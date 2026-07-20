#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <QString>

#include "SubtitleTypes.h"

namespace TorrentPlayer::Subtitles {

class SubtitleContentProcessor
{
public:
	SubtitleContentProcessor();
	~SubtitleContentProcessor();

	std::optional<ProcessedSubtitle> Process(
		const SubtitlePayload & payload,
		const VideoSearchMetadata & metadata,
		QString & errorDescription) const;
	std::optional<ProcessedSubtitle> Process(
		const std::vector<SubtitlePayload> & payloads,
		const VideoSearchMetadata & metadata,
		QString & errorDescription) const;
	std::vector<SubtitlePayload> Expand(
		const SubtitlePayload & payload,
		bool & wasArchive,
		QString & errorDescription) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace TorrentPlayer::Subtitles
