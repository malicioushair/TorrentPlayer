#include "VideoMetadataResolver.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace TorrentPlayer::Subtitles {
namespace {

QString ExtractImdbId(const QString & text)
{
	static const QRegularExpression imdbIdRegex(
		QStringLiteral(R"(tt(\d{7,8}))"),
		QRegularExpression::CaseInsensitiveOption);
	const auto match = imdbIdRegex.match(text);
	return match.hasMatch() ? QStringLiteral("tt%1").arg(match.captured(1)) : QString();
}

QString NormalizeImdbId(const QString & imdbId)
{
	const auto trimmed = imdbId.trimmed();
	if (trimmed.isEmpty())
		return {};

	if (const auto extracted = ExtractImdbId(trimmed); !extracted.isEmpty())
		return extracted;

	static const QRegularExpression bareImdbIdRegex(QStringLiteral(R"(^(\d{7,8})$)"));
	const auto match = bareImdbIdRegex.match(trimmed);
	return match.hasMatch() ? QStringLiteral("tt%1").arg(match.captured(1)) : QString();
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
		if (!match.hasMatch())
			continue;

		const auto season = match.captured(1).toInt();
		const auto episode = match.captured(2).toInt();
		if (season > 0 && episode > 0)
			return std::make_pair(season, episode);
	}

	return std::nullopt;
}

std::optional<std::pair<QString, qint64>> ComputeMovieHash(const QString & videoPath)
{
	QFile file(videoPath);
	if (!file.open(QIODevice::ReadOnly))
		return std::nullopt;

	static constexpr qint64 chunkSize = 64LL * 1024;
	const auto fileSize = file.size();
	if (fileSize < chunkSize * 2)
		return std::nullopt;

	const auto sumChunk = [](const QByteArray & chunk) {
		quint64 sum = 0;
		const auto * data = reinterpret_cast<const unsigned char *>(chunk.constData());
		for (qsizetype offset = 0; offset + 8 <= chunk.size(); offset += 8)
		{
			quint64 value = 0;
			for (auto byte = 0; byte < 8; ++byte)
				value |= static_cast<quint64>(data[offset + byte]) << (8 * byte);
			sum += value;
		}
		return sum;
	};

	const auto head = file.read(chunkSize);
	if (head.size() != chunkSize || !file.seek(fileSize - chunkSize))
		return std::nullopt;
	const auto tail = file.read(chunkSize);
	if (tail.size() != chunkSize)
		return std::nullopt;

	const auto hash = static_cast<quint64>(fileSize) + sumChunk(head) + sumChunk(tail);
	return std::make_pair(QStringLiteral("%1").arg(hash, 16, 16, QLatin1Char('0')), fileSize);
}

}

struct VideoMetadataResolver::Impl
{
	VideoSearchMetadata Resolve(const QString & videoPath, const QString & explicitImdbId) const
	{
		VideoSearchMetadata result;
		const auto fileName = QFileInfo(videoPath).fileName();
		result.imdbId = NormalizeImdbId(explicitImdbId);
		if (result.imdbId.isEmpty())
			result.imdbId = ExtractImdbId(fileName);

		if (const auto seasonEpisode = ParseSeasonEpisode(fileName))
		{
			result.season = seasonEpisode->first;
			result.episode = seasonEpisode->second;
		}

		if (const auto movieHash = ComputeMovieHash(videoPath))
		{
			result.movieHash = movieHash->first;
			result.movieFileSize = movieHash->second;
		}

		return result;
	}
};

VideoMetadataResolver::VideoMetadataResolver()
	: m_impl(std::make_unique<Impl>())
{
}

VideoMetadataResolver::~VideoMetadataResolver() = default;

VideoSearchMetadata VideoMetadataResolver::Resolve(const QString & videoPath, const QString & explicitImdbId) const
{
	return m_impl->Resolve(videoPath, explicitImdbId);
}

} // namespace TorrentPlayer::Subtitles
