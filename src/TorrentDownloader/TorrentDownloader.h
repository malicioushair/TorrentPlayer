#pragma once

#include <memory>
#include <string>

#include <libtorrent/alert_types.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_status.hpp>

#include "TorrentDownloader/Notifier.h"

class TorrentDownloader
{
public:
	TorrentDownloader(Notifier & notifier);
	~TorrentDownloader();

	void DownloadWithMagnet(const std::string & magnet_url);
	void DownlloadWithTorrentFile(const std::string & torrentPath);
	std::string GetVideoFile();
	int GetDownloadProgress();

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};
