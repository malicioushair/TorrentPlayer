#include "SubtitleTypes.h"

#include <QFileInfo>

namespace TorrentPlayer::Subtitles {

QString SubtitleProviderName(SubtitleProviderId provider)
{
	switch (provider)
	{
		case SubtitleProviderId::Subdl:
			return QStringLiteral("SubDL");
		case SubtitleProviderId::OpenSubtitles:
			return QStringLiteral("OpenSubtitles");
	}

	return {};
}

QString SubtitleFormatExtension(SubtitleFormat format)
{
	switch (format)
	{
		case SubtitleFormat::Srt:
			return QStringLiteral("srt");
		case SubtitleFormat::Vtt:
			return QStringLiteral("vtt");
		case SubtitleFormat::Ass:
			return QStringLiteral("ass");
		case SubtitleFormat::Ssa:
			return QStringLiteral("ssa");
		case SubtitleFormat::Unknown:
			return {};
	}

	return {};
}

SubtitleFormat SubtitleFormatFromFileName(const QString & fileName)
{
	const auto extension = QFileInfo(fileName).suffix().toLower();
	if (extension == QStringLiteral("srt"))
		return SubtitleFormat::Srt;
	if (extension == QStringLiteral("vtt"))
		return SubtitleFormat::Vtt;
	if (extension == QStringLiteral("ass"))
		return SubtitleFormat::Ass;
	if (extension == QStringLiteral("ssa"))
		return SubtitleFormat::Ssa;

	return SubtitleFormat::Unknown;
}

bool IsSubtitleFormat(SubtitleFormat format)
{
	return format != SubtitleFormat::Unknown;
}

} // namespace TorrentPlayer::Subtitles
