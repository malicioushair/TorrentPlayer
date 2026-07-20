#pragma once

#include <memory>
#include <optional>

#include <QString>

#include "SubtitleTypes.h"

namespace TorrentPlayer::Subtitles {

class SubtitleParser
{
public:
	SubtitleParser();
	~SubtitleParser();

	std::optional<SubtitleDocument> Parse(const SubtitlePayload & payload, QString * errorDescription = nullptr) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace TorrentPlayer::Subtitles
