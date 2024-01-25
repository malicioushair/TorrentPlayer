#pragma once

#include <memory>
#include <string>

#include <libtorrent/alert_types.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_status.hpp>

class TorrentDownloader
{
public:
	TorrentDownloader();
	~TorrentDownloader();

	void DownloadWithMagnet(const std::string & magnet_url);

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};
