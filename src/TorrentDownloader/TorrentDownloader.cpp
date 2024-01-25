#include "TorrentDownloader.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/write_resume_data.hpp>

class TorrentDownloader::Impl
{
public:
	Impl()
	{
		m_settings.set_int(lt::settings_pack::alert_mask,
			lt::alert_category::error | lt::alert_category::storage | lt::alert_category::status);
		m_session = std::make_unique<lt::session>(m_settings);
	}

	void AddMagnetLink(const std::string & magnetUrl)
	{
		std::ifstream resumeStream(m_resumeFile, std::ios_base::binary);
		std::vector<char> resumeData(std::istreambuf_iterator<char>(resumeStream), {});

		auto magnetParams = lt::parse_magnet_uri(magnetUrl);
		if (!resumeData.empty())
		{
			auto atp = lt::read_resume_data(resumeData);
			if (atp.info_hashes == magnetParams.info_hashes)
				magnetParams = std::move(atp);
		}
		magnetParams.save_path = "."; // save in current dir
		m_session->async_add_torrent(std::move(magnetParams));
	}

	void DownloadTorrent()
	{
		using namespace std::chrono;
		auto lastSaveResume = steady_clock::now();
		bool isDone = false;

		while (!isDone)
		{
			std::vector<lt::alert *> alerts;
			m_session->pop_alerts(&alerts);

			for (const auto * alert : alerts)
				handleAlert(alert, isDone);

			std::this_thread::sleep_for(milliseconds(200));
			m_session->post_torrent_updates();

			if (steady_clock::now() - lastSaveResume > seconds(30))
			{
				m_torrentHandle.save_resume_data(lt::torrent_handle::only_if_modified | lt::torrent_handle::save_info_dict);
				lastSaveResume = steady_clock::now();
			}
		}

		std::cout << "\nDone, shutting down\n";
	}

	void handleAlert(const lt::alert * alert, bool & is_done)
	{
		if (auto * add_torrent_alert = lt::alert_cast<lt::add_torrent_alert>(alert))
			m_torrentHandle = add_torrent_alert->handle;

		if (lt::alert_cast<lt::torrent_finished_alert>(alert) || lt::alert_cast<lt::torrent_error_alert>(alert))
		{
			is_done = true;
			m_torrentHandle.save_resume_data(lt::torrent_handle::only_if_modified | lt::torrent_handle::save_info_dict);
		}

		if (auto * save_resume_data_alert = lt::alert_cast<lt::save_resume_data_alert>(alert))
		{
			std::ofstream of(m_resumeFile, std::ios_base::binary);
			const auto buffer = write_resume_data_buf(save_resume_data_alert->params);
			of.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		}

		if (auto * state_update_alert = lt::alert_cast<lt::state_update_alert>(alert))
		{
			if (!state_update_alert->status.empty())
			{
				const lt::torrent_status & status = state_update_alert->status.front();
				std::string torrent_name = m_torrentHandle.is_valid() && m_torrentHandle.torrent_file() ? m_torrentHandle.torrent_file()->name() : "<unknown>";

				std::cout << "\r" << torrent_name << ": "
						  << getTorrentStateName(status.state) << ' '
						  << (status.download_payload_rate / 1000) << " kB/s "
						  << (status.total_done / 1000) << " kB ("
						  << (status.progress_ppm / 10000) << "%) downloaded ("
						  << status.num_peers << " peers)\x1b[K";
				std::cout.flush();
			}
		}
	}

	const char * getTorrentStateName(lt::torrent_status::state_t state) const
	{
		switch (state)
		{
			case lt::torrent_status::checking_files:
				return "checking";
			case lt::torrent_status::downloading_metadata:
				return "dl metadata";
			case lt::torrent_status::downloading:
				return "downloading";
			case lt::torrent_status::finished:
				return "finished";
			case lt::torrent_status::seeding:
				return "seeding";
			case lt::torrent_status::checking_resume_data:
				return "checking resume";
			default:
				return "<unknown>";
		}
	};

private:
	std::unique_ptr<lt::session> m_session;
	lt::settings_pack m_settings;
	lt::torrent_handle m_torrentHandle;
	std::string m_resumeFile = ".resume_file";
};

TorrentDownloader::TorrentDownloader()
	: m_impl(std::make_unique<Impl>())
{
}

TorrentDownloader::~TorrentDownloader() = default;

void TorrentDownloader::DownloadWithMagnet(const std::string & magnet_url)
{
	try
	{
		m_impl->AddMagnetLink(magnet_url);
		m_impl->DownloadTorrent();
	}
	catch (const std::exception & e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
}
