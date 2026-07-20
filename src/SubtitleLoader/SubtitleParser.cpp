#include "SubtitleParser.h"

#include <algorithm>

#include <QRegularExpression>
#include <QStringConverter>
#include <QStringDecoder>
#include <QStringList>

namespace TorrentPlayer::Subtitles {
namespace {

constexpr auto MAX_SUBTITLE_BYTES = 10 * 1024 * 1024;

QString DecodeSubtitle(const QByteArray & content)
{
	if (content.startsWith("\xEF\xBB\xBF"))
		return QString::fromUtf8(content.sliced(3));

	if (content.size() >= 2)
	{
		const auto first = static_cast<unsigned char>(content.at(0));
		const auto second = static_cast<unsigned char>(content.at(1));
		if (first == 0xFF && second == 0xFE)
		{
			QStringDecoder decoder(QStringConverter::Utf16LE);
			return decoder.decode(content.sliced(2));
		}
		if (first == 0xFE && second == 0xFF)
		{
			QStringDecoder decoder(QStringConverter::Utf16BE);
			return decoder.decode(content.sliced(2));
		}
	}

	QStringDecoder utf8Decoder(QStringConverter::Utf8);
	const auto utf8Text = utf8Decoder.decode(content);
	if (!utf8Decoder.hasError())
		return utf8Text;

	return QString::fromLatin1(content);
}

qint64 ParseTimestamp(const QString & timestamp)
{
	static const QRegularExpression timestampRegex(
		QStringLiteral(R"(^\s*(?:(\d+):)?(\d{1,2}):(\d{2})[,.](\d{1,3})(?:\s|$))"));
	const auto match = timestampRegex.match(timestamp);
	if (!match.hasMatch())
		return -1;

	const auto hours = match.captured(1).isEmpty() ? 0 : match.captured(1).toLongLong();
	const auto minutes = match.captured(2).toLongLong();
	const auto seconds = match.captured(3).toLongLong();
	if (minutes >= 60 || seconds >= 60)
		return -1;

	auto millisecondsText = match.captured(4);
	while (millisecondsText.size() < 3)
		millisecondsText += QLatin1Char('0');

	return ((hours * 60 + minutes) * 60 + seconds) * 1000 + millisecondsText.left(3).toLongLong();
}

QString CleanSubtitleText(QString text)
{
	text.replace(QRegularExpression(QStringLiteral(R"(<[^>]*>)")), QString());
	text.replace(QRegularExpression(QStringLiteral(R"(\{[^}]*\})")), QString());
	text.replace(QStringLiteral("\\N"), QStringLiteral("\n"));
	text.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
	text.replace(QStringLiteral("\\h"), QStringLiteral(" "));
	return text.trimmed();
}

std::vector<SubtitleCue> ParseAss(const QString & content)
{
	std::vector<SubtitleCue> cues;
	auto normalized = content;
	normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
	normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));

	for (const auto & rawLine : normalized.split(QLatin1Char('\n')))
	{
		const auto line = rawLine.trimmed();
		if (!line.startsWith(QStringLiteral("Dialogue:"), Qt::CaseInsensitive))
			continue;

		const auto fields = line.mid(QStringLiteral("Dialogue:").size()).trimmed().split(QLatin1Char(','), Qt::KeepEmptyParts);
		if (fields.size() < 10)
			continue;

		const auto startMs = ParseTimestamp(fields[1]);
		const auto endMs = ParseTimestamp(fields[2]);
		const auto text = CleanSubtitleText(fields.mid(9).join(QLatin1Char(',')));
		if (startMs >= 0 && endMs > startMs && !text.isEmpty())
			cues.push_back({ startMs, endMs, text });
	}

	return cues;
}

std::vector<SubtitleCue> ParseSrtOrVtt(const QString & content)
{
	std::vector<SubtitleCue> cues;
	auto normalized = content;
	normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
	normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
	if (!normalized.isEmpty() && normalized.front().unicode() == 0xFEFF)
		normalized.remove(0, 1);
	if (normalized.startsWith(QStringLiteral("WEBVTT")))
	{
		const auto headerEnd = normalized.indexOf(QLatin1Char('\n'));
		normalized.remove(0, headerEnd < 0 ? normalized.size() : headerEnd + 1);
	}

	const auto blocks = normalized.split(QRegularExpression(QStringLiteral(R"(\n\s*\n)")), Qt::SkipEmptyParts);
	for (const auto & block : blocks)
	{
		const auto lines = block.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
		auto timeLineIndex = -1;
		for (auto index = 0; index < lines.size(); ++index)
		{
			if (lines[index].contains(QStringLiteral("-->")))
			{
				timeLineIndex = index;
				break;
			}
		}

		if (timeLineIndex < 0)
			continue;

		const auto times = lines[timeLineIndex].split(QStringLiteral("-->"));
		if (times.size() != 2)
			continue;

		const auto startMs = ParseTimestamp(times[0]);
		const auto endMs = ParseTimestamp(times[1]);
		if (startMs < 0 || endMs <= startMs)
			continue;

		const auto text = CleanSubtitleText(lines.mid(timeLineIndex + 1).join(QLatin1Char('\n')));
		if (!text.isEmpty())
			cues.push_back({ startMs, endMs, text });
	}

	return cues;
}

}

struct SubtitleParser::Impl
{
	std::optional<SubtitleDocument> Parse(const SubtitlePayload & payload, QString * errorDescription) const
	{
		if (payload.content.isEmpty() || payload.content.size() > MAX_SUBTITLE_BYTES)
		{
			if (errorDescription)
				*errorDescription = payload.content.isEmpty()
					? QStringLiteral("The subtitle file is empty.")
					: QStringLiteral("The subtitle file is too large.");
			return std::nullopt;
		}

		auto format = payload.format;
		if (!IsSubtitleFormat(format))
			format = SubtitleFormatFromFileName(payload.fileName);
		if (!IsSubtitleFormat(format))
			format = SubtitleFormat::Srt;

		const auto text = DecodeSubtitle(payload.content);
		auto cues = format == SubtitleFormat::Ass || format == SubtitleFormat::Ssa
			? ParseAss(text)
			: ParseSrtOrVtt(text);
		if (cues.empty())
		{
			if (errorDescription)
				*errorDescription = QStringLiteral("The subtitle format is unsupported or contains no valid cues.");
			return std::nullopt;
		}

		std::ranges::sort(cues, {}, &SubtitleCue::startMs);
		return SubtitleDocument { payload.fileName, format, std::move(cues) };
	}
};

SubtitleParser::SubtitleParser()
	: m_impl(std::make_unique<Impl>())
{
}

SubtitleParser::~SubtitleParser() = default;

std::optional<SubtitleDocument> SubtitleParser::Parse(const SubtitlePayload & payload, QString * errorDescription) const
{
	return m_impl->Parse(payload, errorDescription);
}

} // namespace TorrentPlayer::Subtitles
