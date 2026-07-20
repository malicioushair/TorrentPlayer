#pragma once

#include <memory>

#include <QString>

#include "SubtitleTypes.h"

namespace TorrentPlayer::Subtitles {

class VideoMetadataResolver
{
public:
	VideoMetadataResolver();
	~VideoMetadataResolver();

	VideoSearchMetadata Resolve(const QString & videoPath, const QString & explicitImdbId) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace TorrentPlayer::Subtitles
