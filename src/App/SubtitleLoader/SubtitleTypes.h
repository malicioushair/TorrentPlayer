#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <QByteArray>
#include <QString>
#include <QtGlobal>

namespace TorrentPlayer::Subtitles {

enum class SubtitleProviderId
{
	Subdl,
	OpenSubtitles,
};

enum class SubtitleFormat
{
	Unknown,
	Srt,
	Vtt,
	Ass,
	Ssa,
};

struct SubtitleCue
{
	qint64 startMs {};
	qint64 endMs {};
	QString text;
};

struct SubtitleDocument
{
	QString label;
	SubtitleFormat format { SubtitleFormat::Unknown };
	std::vector<SubtitleCue> cues;
};

struct VideoSearchMetadata
{
	QString imdbId;
	std::optional<int> season;
	std::optional<int> episode;
	QString movieHash;
	qint64 movieFileSize {};
};

struct SubtitleRequestContext
{
	quint64 requestId {};
	QString videoPath;
	QString language;
	VideoSearchMetadata metadata;
};

struct SubtitlePayload
{
	SubtitleProviderId provider { SubtitleProviderId::Subdl };
	QString fileName;
	SubtitleFormat format { SubtitleFormat::Unknown };
	QByteArray content;
};

struct ProcessedSubtitle
{
	SubtitleDocument document;
	SubtitlePayload cachePayload;
};

enum class ProviderRequestStatus
{
	Success,
	NotFound,
	Failed,
};

struct SubtitleProviderResult
{
	ProviderRequestStatus status { ProviderRequestStatus::Failed };
	SubtitleProviderId provider { SubtitleProviderId::Subdl };
	SubtitlePayload payload;
	QString errorDescription;
};

struct SubtitleRequestOutcome
{
	bool success {};
	SubtitleRequestContext context;
	SubtitleProviderId provider { SubtitleProviderId::Subdl };
	SubtitlePayload payload;
	QString errorTitle;
	QString errorDescription;
};

QString SubtitleProviderName(SubtitleProviderId provider);
QString SubtitleFormatExtension(SubtitleFormat format);
SubtitleFormat SubtitleFormatFromFileName(const QString & fileName);
bool IsSubtitleFormat(SubtitleFormat format);

} // namespace TorrentPlayer::Subtitles
