#pragma once

#include <memory>
#include <vector>

#include <QString>

#include "SubtitleTypes.h"

namespace TorrentPlayer::Subtitles {

class SubtitleArchiveExtractor
{
public:
	SubtitleArchiveExtractor();
	~SubtitleArchiveExtractor();

	std::vector<SubtitlePayload> Extract(
		const SubtitlePayload & archive,
		QString & errorDescription) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace TorrentPlayer::Subtitles
