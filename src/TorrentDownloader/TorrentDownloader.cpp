#include "TorrentDownloader.h"

#include <chrono>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/write_resume_data.hpp>

#include "glog/logging.h"

#include "Notifier.h"

namespace {

constexpr auto operator"" _MiB(unsigned long long x)
{
	return x * 1024 * 1024;
}

inline void prioritizeFileTail(const lt::torrent_handle & torrentHandle, lt::file_index_t fileIndex, std::int64_t tailBytes)
{
	const auto torrentInfo = torrentHandle.torrent_file();
	if (!torrentInfo)
		return;

	const auto & fileStorage = torrentInfo->files();
	const auto fileSize = fileStorage.file_size(fileIndex);
	if (fileSize <= 0)
		return;

	// how much of the end we want (clamped to file size)
	tailBytes = std::max<std::int64_t>(1, std::min(tailBytes, fileSize));

	const auto pieceLength = torrentInfo->piece_length();
	const auto fileOffset = fileStorage.file_offset(fileIndex); // file start within the torrent
	const auto firstPieceIdx = lt::piece_index_t((fileOffset) / pieceLength);
	const auto lastPieceIdx = lt::piece_index_t((fileOffset + fileSize - 1) / pieceLength);

	// which piece index contains the first byte of the "tail" region
	const auto tailStartOffset = fileOffset + (fileSize - tailBytes);
	const auto tailFirstPieceIdx = lt::piece_index_t(tailStartOffset / pieceLength);

	// 1) Start with low priority everywhere
	std::vector<lt::download_priority_t> prios(torrentInfo->num_pieces(), lt::download_priority_t { 1 });
	torrentHandle.prioritize_pieces(prios);

	// 2) Raise priority for the tail range
	for (lt::piece_index_t pieceIdx = tailFirstPieceIdx; pieceIdx <= lastPieceIdx; ++pieceIdx)
		torrentHandle.piece_priority(pieceIdx, libtorrent::top_priority);

	// 3) Add deadlines so the tail is requested right away, scheduling from the very last piece backwards.
	auto order = 0;
	for (lt::piece_index_t pieceIdx = lastPieceIdx; pieceIdx >= tailFirstPieceIdx; --pieceIdx)
	{
		// 0 ms, 100 ms, 200 ms ... to keep an order without spamming
		torrentHandle.set_piece_deadline(static_cast<int>(static_cast<int>(pieceIdx)), order * 100, lt::torrent_handle::alert_when_available);
		++order;
		if (pieceIdx == lt::piece_index_t(0))
			break; // protect from wrap-around
	}
}

}

class TorrentDownloader::Impl
	: public Notifier
{
public:
	Impl(Notifier & notifier)
		: m_notifier(notifier)
	{
		m_settings.set_int(lt::settings_pack::alert_mask, lt::alert_category::error | lt::alert_category::storage | lt::alert_category::status);
		m_session = std::make_unique<lt::session>(m_settings);
	}

	int GetDownloadProgress()
	{
		return m_downloadProgress;
	}

	void AddMagnetLink(const std::string & magnetUrl, const std::string & savePath)
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
		magnetParams.save_path = savePath;
		m_session->async_add_torrent(std::move(magnetParams));
	}

	void AddTorrentFile(const std::string & torrentPath, const std::string & savePath)
	{
		// 1.  Load (optional) fast-resume data
		std::ifstream resumeStream(m_resumeFile, std::ios_base::binary);
		std::vector<char> resumeData { std::istreambuf_iterator<char>(resumeStream), {} };

		// 2.  Parse the .torrent
		lt::error_code ec;
		m_torrentInfo = std::make_shared<lt::torrent_info>(torrentPath, ec);
		if (ec)
			throw std::runtime_error("Invalid torrent file: " + ec.message());

		lt::add_torrent_params atp;
		atp.ti = m_torrentInfo; // the metadata

		// 3.  Merge resume data if it matches this torrent
		if (!resumeData.empty())
		{
			auto r = lt::read_resume_data(resumeData, ec);
			if (!ec && r.info_hashes == atp.ti->info_hashes())
				atp = std::move(r); // re-use stored priorities, etc.
			atp.ti = m_torrentInfo; // make sure metadata is set
		}

		// 4.  Hand it to the session (asynchronous)
		atp.save_path = savePath;
		m_session->async_add_torrent(std::move(atp));
	}

	bool HasMoov(const size_t chunkSize)
	{
		const auto filePath = GetVideoFile();
		std::ifstream in(filePath, std::ios::binary);
		if (!in)
			return false;

		in.seekg(0, std::ios::end);
		const auto fileSize = static_cast<std::uint64_t>(in.tellg());
		if (fileSize == 0)
			return false;

		const auto availableChunkSize = std::min<std::size_t>(chunkSize, static_cast<std::size_t>(fileSize));
		std::vector<char> buffer(chunkSize);

		const auto readHas = [&](std::uint64_t start, std::size_t n, std::string_view needle) {
			in.seekg(static_cast<std::streamoff>(start), std::ios::beg);
			in.read(buffer.data(), static_cast<std::streamsize>(n));
			const auto got = in.gcount();
			if (got == 0)
				return false;
			std::string_view sv(buffer.data(), got);
			return sv.find(needle) != std::string_view::npos;
		};

		static constexpr auto MOOV = "moov";
		const auto beginHasMoov = readHas(0, chunkSize, MOOV);
		const auto endHasMoov = readHas(fileSize - chunkSize, chunkSize, MOOV);

		return beginHasMoov || endHasMoov;
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
				HandleAlert(alert, isDone);

			std::this_thread::sleep_for(milliseconds(200));
			m_session->post_torrent_updates();

			if (steady_clock::now() - lastSaveResume > seconds(30))
			{
				m_torrentHandle.save_resume_data(lt::torrent_handle::only_if_modified | lt::torrent_handle::save_info_dict);
				m_torrentHandle.set_flags(
					lt::torrent_flags::sequential_download,
					lt::torrent_flags::sequential_download);

				static constexpr auto chunkSize = 16_MiB;
				if (!m_hasMoov)
					m_hasMoov = HasMoov(chunkSize);
				if (!m_isDownloadComplete && m_torrentHandle.status().total_wanted_done >= chunkSize && !m_hasMoov)
					prioritizeFileTail(m_torrentHandle, libtorrent::file_index_t { 0 }, chunkSize);

				lastSaveResume = steady_clock::now();
			}
		}

		LOG(INFO) << "Torrent download completed. Video file: " << GetVideoFile();
		m_isDownloadComplete = true;
	}

	void HandleAlert(const lt::alert * alert, bool & is_done)
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

				VLOG(1) << "\r" << torrent_name << ": "
						<< getTorrentStateName(status.state) << ' '
						<< (status.download_payload_rate / 1000) << " kB/s "
						<< (status.total_done / 1000) << " kB ("
						<< (status.progress_ppm / 10000) << "%) downloaded ("
						<< status.num_peers << " peers)\n";

				if (status.progress_ppm >= 100000) // 10% progress
				{
					// Notify observers that the video is ready to play only once
					if (!m_isVideoReady)
					{
						m_isVideoReady = true;
						m_notifier.OnReadyToPlayVideo();
					}
				}

				UpdateDownloadProgress();
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

	std::string GetVideoFile()
	{
		if (!m_torrentHandle.is_valid() || !m_torrentHandle.torrent_file())
			return {};

		return m_torrentHandle.status().save_path + std::filesystem::path::preferred_separator + m_torrentHandle.torrent_file()->files().begin_deprecated()->filename().to_string();
	}

private:
	void UpdateDownloadProgress()
	{
		m_downloadProgress = m_torrentHandle.status().progress_ppm / 10000;
		m_notifier.OnDownloadProgressChanged();
	}

private:
	std::unique_ptr<lt::session> m_session;
	lt::settings_pack m_settings;
	lt::torrent_handle m_torrentHandle;
	Notifier & m_notifier;
	std::string m_resumeFile { ".resume_file" };
	bool m_isVideoReady { false };
	int m_downloadProgress { 0 };
	std::shared_ptr<lt::torrent_info> m_torrentInfo;
	bool m_isDownloadComplete { false };
	bool m_hasMoov { false };
};

TorrentDownloader::TorrentDownloader(Notifier & notifier)
	: m_impl(std::make_unique<Impl>(notifier))
{
}

TorrentDownloader::~TorrentDownloader() = default;

void TorrentDownloader::DownloadWithMagnet(const std::string & magnet_url, const std::string & savePath)
{
	try
	{
		m_impl->AddMagnetLink(magnet_url, savePath);
		m_impl->DownloadTorrent();
	}
	catch (const std::exception & e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

void TorrentDownloader::DownloadWithTorrentFile(const std::string & torrentPath, const std::string & savePath)
{
	try
	{
		m_impl->AddTorrentFile(torrentPath, savePath);
		std::thread([this] {
			m_impl->DownloadTorrent();
		}).detach();
	}
	catch (const std::exception & e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

std::string TorrentDownloader::GetVideoFile() const
{
	return m_impl->GetVideoFile();
}

int TorrentDownloader::GetDownloadProgress() const
{
	return m_impl->GetDownloadProgress();
}
