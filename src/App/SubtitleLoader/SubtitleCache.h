#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <QString>

#include "SubtitleTypes.h"

namespace TorrentPlayer::Subtitles {

class SubtitleCache
{
public:
	explicit SubtitleCache(const QString & cacheDirectory = {});
	~SubtitleCache();

	std::optional<SubtitlePayload> Find(
		const QString & videoPath,
		const QString & language,
		SubtitleProviderId provider) const;
	bool Store(
		const QString & videoPath,
		const QString & language,
		const SubtitlePayload & payload,
		QString & errorDescription) const;
	std::vector<SubtitlePayload> FindArchiveEntries(
		const QString & videoPath,
		const QString & language,
		SubtitleProviderId provider) const;
	bool StoreArchiveEntries(
		const QString & language,
		SubtitleProviderId provider,
		const std::vector<SubtitlePayload> & payloads,
		QString & errorDescription) const;
	void Invalidate(const QString & videoPath, const QString & language, SubtitleProviderId provider) const;
	void InvalidateArchiveEntries(
		const QString & language,
		SubtitleProviderId provider,
		const std::vector<SubtitlePayload> & payloads) const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace TorrentPlayer::Subtitles
