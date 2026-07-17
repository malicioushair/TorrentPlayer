#include "SubtitleContentProcessor.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <QRegularExpression>

#include "SubtitleArchiveExtractor.h"
#include "SubtitleParser.h"

namespace TorrentPlayer::Subtitles {
namespace {

bool IsZipArchive(const QByteArray & content)
{
	return content.size() >= 4
		&& content.at(0) == 'P'
		&& content.at(1) == 'K'
		&& static_cast<unsigned char>(content.at(2)) == 0x03
		&& static_cast<unsigned char>(content.at(3)) == 0x04;
}

std::optional<std::pair<int, int>> ParseSeasonEpisode(const QString & text)
{
	static const QRegularExpression seasonEpisodeRegex(
		QStringLiteral(R"((?:^|[^\d])[Ss](\d{1,2})[Ee](\d{1,2})(?:[^\d]|$))"));
	static const QRegularExpression alternativeRegex(
		QStringLiteral(R"((?:^|[^\d])(\d{1,2})[xX](\d{1,2})(?:[^\d]|$))"));

	for (const auto & regex : { seasonEpisodeRegex, alternativeRegex })
	{
		const auto match = regex.match(text);
		if (match.hasMatch())
			return std::make_pair(match.captured(1).toInt(), match.captured(2).toInt());
	}

	return std::nullopt;
}

int FormatPriority(SubtitleFormat format)
{
	switch (format)
	{
		case SubtitleFormat::Srt:
			return 0;
		case SubtitleFormat::Vtt:
			return 1;
		case SubtitleFormat::Ass:
		case SubtitleFormat::Ssa:
			return 2;
		case SubtitleFormat::Unknown:
			return 3;
	}

	return 3;
}

std::vector<const SubtitlePayload *> SelectPayloads(
	const std::vector<SubtitlePayload> & payloads,
	const VideoSearchMetadata & metadata)
{
	if (payloads.empty())
		return {};

	std::vector<const SubtitlePayload *> matches;
	if (payloads.size() > 1 && metadata.season && metadata.episode)
	{
		for (const auto & payload : payloads)
		{
			if (const auto seasonEpisode = ParseSeasonEpisode(payload.fileName))
				if (seasonEpisode->first == *metadata.season && seasonEpisode->second == *metadata.episode)
					matches.push_back(&payload);
		}
	}

	if (matches.empty() && !(payloads.size() > 1 && metadata.season && metadata.episode))
		for (const auto & payload : payloads)
			matches.push_back(&payload);
	std::ranges::stable_sort(matches, {}, [](const SubtitlePayload * payload) {
		return FormatPriority(payload->format);
	});
	return matches;
}

}

struct SubtitleContentProcessor::Impl
{
	SubtitleArchiveExtractor archiveExtractor;
	SubtitleParser parser;

	std::vector<SubtitlePayload> Expand(
		const SubtitlePayload & payload,
		bool & wasArchive,
		QString & errorDescription) const
	{
		const auto archive = IsZipArchive(payload.content);
		wasArchive = archive;
		if (archive)
			return archiveExtractor.Extract(payload, errorDescription);
		return { payload };
	}

	std::optional<ProcessedSubtitle> Process(
		const std::vector<SubtitlePayload> & payloads,
		const VideoSearchMetadata & metadata,
		QString & errorDescription) const
	{
		const auto candidates = SelectPayloads(payloads, metadata);
		if (candidates.empty())
		{
			errorDescription = metadata.season && metadata.episode
								 ? QStringLiteral("The subtitle archive contains no entry for the current episode.")
								 : QStringLiteral("No supported subtitle file was found.");
			return std::nullopt;
		}

		QString parseError;
		for (const auto * candidate : candidates)
		{
			auto selectedPayload = *candidate;
			if (!IsSubtitleFormat(selectedPayload.format))
				selectedPayload.format = SubtitleFormatFromFileName(selectedPayload.fileName);
			if (!IsSubtitleFormat(selectedPayload.format))
				selectedPayload.format = SubtitleFormat::Srt;

			auto document = parser.Parse(selectedPayload, &parseError);
			if (document)
				return ProcessedSubtitle { std::move(*document), std::move(selectedPayload) };
		}

		errorDescription = parseError;
		return std::nullopt;
	}
};

SubtitleContentProcessor::SubtitleContentProcessor()
	: m_impl(std::make_unique<Impl>())
{
}

SubtitleContentProcessor::~SubtitleContentProcessor() = default;

std::optional<ProcessedSubtitle> SubtitleContentProcessor::Process(
	const SubtitlePayload & payload,
	const VideoSearchMetadata & metadata,
	QString & errorDescription) const
{
	[[maybe_unused]] auto wasArchive = false; // @TODO: get rid of the redundant bool
	const auto payloads = m_impl->Expand(payload, wasArchive, errorDescription);
	return m_impl->Process(payloads, metadata, errorDescription);
}

std::optional<ProcessedSubtitle> SubtitleContentProcessor::Process(
	const std::vector<SubtitlePayload> & payloads,
	const VideoSearchMetadata & metadata,
	QString & errorDescription) const
{
	return m_impl->Process(payloads, metadata, errorDescription);
}

std::vector<SubtitlePayload> SubtitleContentProcessor::Expand(
	const SubtitlePayload & payload,
	bool & wasArchive,
	QString & errorDescription) const
{
	return m_impl->Expand(payload, wasArchive, errorDescription);
}

} // namespace TorrentPlayer::Subtitles
