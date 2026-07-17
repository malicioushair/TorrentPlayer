#include "SubtitleCache.h"

#include <array>
#include <optional>
#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace TorrentPlayer::Subtitles {
namespace {

QString DefaultCacheDirectory()
{
	const auto appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	return QDir(appData).absoluteFilePath(QStringLiteral("subtitles"));
}

QString SafeFileNameComponent(QString value)
{
	static const auto invalidCharacters = QStringLiteral("<>:\"/\\|?*");
	for (const auto character : invalidCharacters)
		value.replace(character, QLatin1Char('_'));
	for (auto index = 0; index < value.size(); ++index)
		if (value[index].unicode() < 0x20)
			value[index] = QLatin1Char('_');

	value = value.trimmed();
	while (value.endsWith(QLatin1Char('.')) || value.endsWith(QLatin1Char(' ')))
		value.chop(1);
	return value.isEmpty() ? QStringLiteral("subtitle") : value.left(120);
}

QString CacheStem(const QString & videoPath, const QString & language, SubtitleProviderId provider)
{
	const QFileInfo videoInfo(videoPath);
	return QStringLiteral("%1--%2--%3").arg(SafeFileNameComponent(QStringLiteral("%1.%2").arg(videoInfo.completeBaseName(), videoInfo.suffix())), SafeFileNameComponent(language.trimmed().toLower()), SafeFileNameComponent(SubtitleProviderName(provider).toLower()));
}

QString CachePath(
	const QString & cacheDirectory,
	const QString & videoPath,
	const QString & language,
	SubtitleProviderId provider,
	SubtitleFormat format)
{
	return QDir(cacheDirectory).absoluteFilePath(QStringLiteral("%1.%2").arg(CacheStem(videoPath, language, provider), SubtitleFormatExtension(format)));
}

QString ArchiveMarker(const QString & language, SubtitleProviderId provider)
{
	return QStringLiteral("--%1--%2").arg(SafeFileNameComponent(language.trimmed().toLower()), SafeFileNameComponent(SubtitleProviderName(provider).toLower()));
}

QString ArchiveCachePath(
	const QString & cacheDirectory,
	const QString & fileName,
	const QString & language,
	SubtitleProviderId provider,
	SubtitleFormat format)
{
	const auto referenceName = SafeFileNameComponent(QFileInfo(fileName).completeBaseName());
	return QDir(cacheDirectory).absoluteFilePath(QStringLiteral("%1%2.%3").arg(referenceName, ArchiveMarker(language, provider), SubtitleFormatExtension(format)));
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

QString SeriesKey(const QString & fileName)
{
	auto baseName = QFileInfo(fileName).completeBaseName();
	static const QRegularExpression episodeRegex(
		QStringLiteral(R"((?:^|[^\d])(?:[Ss]\d{1,2}[Ee]\d{1,2}|\d{1,2}[xX]\d{1,2})(?:[^\d]|$))"));
	if (const auto match = episodeRegex.match(baseName); match.hasMatch())
		baseName.truncate(match.capturedStart());

	baseName = baseName.toLower();
	baseName.remove(QRegularExpression(QStringLiteral(R"([^\p{L}\p{N}])")));
	return baseName;
}

bool MatchesVideoFile(const QString & videoPath, const QString & subtitleFileName)
{
	const auto videoFileName = QFileInfo(videoPath).fileName();
	const auto videoEpisode = ParseSeasonEpisode(videoFileName);
	const auto subtitleEpisode = ParseSeasonEpisode(subtitleFileName);
	if (videoEpisode || subtitleEpisode)
	{
		if (!videoEpisode || !subtitleEpisode || videoEpisode != subtitleEpisode)
			return false;
		return !SeriesKey(videoFileName).isEmpty() && SeriesKey(videoFileName) == SeriesKey(subtitleFileName);
	}

	return QFileInfo(videoFileName).completeBaseName().compare(QFileInfo(subtitleFileName).completeBaseName(), Qt::CaseInsensitive) == 0;
}

constexpr std::array SUPPORTED_FORMATS {
	SubtitleFormat::Srt,
	SubtitleFormat::Vtt,
	SubtitleFormat::Ass,
	SubtitleFormat::Ssa,
};

}

struct SubtitleCache::Impl
{
	explicit Impl(QString cacheDirectory)
		: cacheDirectory(cacheDirectory.isEmpty() ? DefaultCacheDirectory() : std::move(cacheDirectory))
	{
		QDir().mkpath(this->cacheDirectory);
	}

	QString cacheDirectory;

	std::optional<SubtitlePayload> Find(
		const QString & videoPath,
		const QString & language,
		SubtitleProviderId provider) const
	{
		for (const auto format : SUPPORTED_FORMATS)
		{
			const auto path = CachePath(cacheDirectory, videoPath, language, provider, format);
			QFile file(path);
			if (!file.exists() || !file.open(QIODevice::ReadOnly))
				continue;

			return SubtitlePayload {
				provider,
				QStringLiteral("%1 %2.%3")
					.arg(SubtitleProviderName(provider), language.toUpper(), SubtitleFormatExtension(format)),
				format,
				file.readAll(),
			};
		}

		return std::nullopt;
	}

	bool Store(
		const QString & videoPath,
		const QString & language,
		const SubtitlePayload & payload,
		QString & errorDescription) const
	{
		if (!IsSubtitleFormat(payload.format) || payload.content.isEmpty())
		{
			errorDescription = QStringLiteral("The processed subtitle cannot be cached.");
			return false;
		}

		const auto targetPath = CachePath(cacheDirectory, videoPath, language, payload.provider, payload.format);
		QSaveFile file(targetPath);
		if (!file.open(QIODevice::WriteOnly) || file.write(payload.content) != payload.content.size() || !file.commit())
		{
			errorDescription = QStringLiteral("Could not write the subtitle cache file.");
			return false;
		}
		for (const auto format : SUPPORTED_FORMATS)
			if (format != payload.format)
				QFile::remove(CachePath(cacheDirectory, videoPath, language, payload.provider, format));

		return true;
	}

	std::vector<SubtitlePayload> FindArchiveEntries(
		const QString & videoPath,
		const QString & language,
		SubtitleProviderId provider) const
	{
		std::vector<SubtitlePayload> result;
		const auto marker = ArchiveMarker(language, provider);
		const QDir directory(cacheDirectory);
		for (const auto & fileInfo : directory.entryInfoList(QDir::Files | QDir::Readable))
		{
			const auto format = SubtitleFormatFromFileName(fileInfo.fileName());
			if (!IsSubtitleFormat(format))
				continue;

			auto referenceName = fileInfo.completeBaseName();
			if (!referenceName.endsWith(marker))
				continue;
			referenceName.chop(marker.size());
			const auto logicalFileName = QStringLiteral("%1.%2").arg(referenceName, SubtitleFormatExtension(format));
			if (!MatchesVideoFile(videoPath, logicalFileName))
				continue;

			QFile file(fileInfo.absoluteFilePath());
			if (!file.open(QIODevice::ReadOnly))
				continue;
			result.push_back({ provider, logicalFileName, format, file.readAll() });
		}

		return result;
	}

	bool StoreArchiveEntries(
		const QString & language,
		SubtitleProviderId provider,
		const std::vector<SubtitlePayload> & payloads,
		QString & errorDescription) const
	{
		for (const auto & payload : payloads)
		{
			if (!IsSubtitleFormat(payload.format) || payload.content.isEmpty())
			{
				errorDescription = "The subtitle archive contains an invalid entry.";
				return false;
			}

			QSaveFile file(ArchiveCachePath(cacheDirectory, payload.fileName, language, provider, payload.format));
			if (!file.open(QIODevice::WriteOnly) || file.write(payload.content) != payload.content.size() || !file.commit())
			{
				errorDescription = "Could not write a subtitle archive cache file.";
				return false;
			}
		}

		return true;
	}

	void Invalidate(const QString & videoPath, const QString & language, SubtitleProviderId provider) const
	{
		for (const auto format : SUPPORTED_FORMATS)
			QFile::remove(CachePath(cacheDirectory, videoPath, language, provider, format));
	}

	void InvalidateArchiveEntries(
		const QString & language,
		SubtitleProviderId provider,
		const std::vector<SubtitlePayload> & payloads) const
	{
		for (const auto & payload : payloads)
			QFile::remove(ArchiveCachePath(cacheDirectory, payload.fileName, language, provider, payload.format));
	}
};

SubtitleCache::SubtitleCache(const QString & cacheDirectory)
	: m_impl(std::make_unique<Impl>(cacheDirectory))
{
}

SubtitleCache::~SubtitleCache() = default;

std::optional<SubtitlePayload> SubtitleCache::Find(
	const QString & videoPath,
	const QString & language,
	SubtitleProviderId provider) const
{
	return m_impl->Find(videoPath, language, provider);
}

bool SubtitleCache::Store(
	const QString & videoPath,
	const QString & language,
	const SubtitlePayload & payload,
	QString & errorDescription) const
{
	return m_impl->Store(videoPath, language, payload, errorDescription);
}

std::vector<SubtitlePayload> SubtitleCache::FindArchiveEntries(
	const QString & videoPath,
	const QString & language,
	SubtitleProviderId provider) const
{
	return m_impl->FindArchiveEntries(videoPath, language, provider);
}

bool SubtitleCache::StoreArchiveEntries(
	const QString & language,
	SubtitleProviderId provider,
	const std::vector<SubtitlePayload> & payloads,
	QString & errorDescription) const
{
	return m_impl->StoreArchiveEntries(language, provider, payloads, errorDescription);
}

void SubtitleCache::Invalidate(const QString & videoPath, const QString & language, SubtitleProviderId provider) const
{
	m_impl->Invalidate(videoPath, language, provider);
}

void SubtitleCache::InvalidateArchiveEntries(
	const QString & language,
	SubtitleProviderId provider,
	const std::vector<SubtitlePayload> & payloads) const
{
	m_impl->InvalidateArchiveEntries(language, provider, payloads);
}

} // namespace TorrentPlayer::Subtitles
