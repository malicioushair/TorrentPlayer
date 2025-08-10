#pragma once

#include <memory>
#include <string>

#include "TorrentDownloader/Notifier.h"

class TorrentDownloader
{
public:
	TorrentDownloader(Notifier & notifier);
	~TorrentDownloader();

	void DownloadWithMagnet(const std::string & magnet_url, const std::string & savePath);
	void DownloadWithTorrentFile(const std::string & torrentPath, const std::string & savePath);
	std::string GetVideoFile() const;
	int GetDownloadProgress() const;

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};
