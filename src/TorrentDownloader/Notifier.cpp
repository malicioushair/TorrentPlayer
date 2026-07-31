#include "Notifier.h"

#include <algorithm>
#include <functional>
#include <memory>

#include "TorrentDownloader/ITorrentDownloaderObserver.h"

namespace {

}

struct Notifier::Impl
{
	std::vector<ITorrentDownloaderObserver *> observers;
	std::mutex mutex;

	template <typename MemFn>
	void NotifyAll(MemFn fn)
	{
		std::lock_guard<std::mutex> lock(mutex);
		for (auto * observer : observers)
			if (observer)
				std::invoke(fn, observer);
	}
};

Notifier::Notifier()
	: m_impl(std::make_unique<Impl>())
{
}

Notifier::~Notifier() = default;

void Notifier::RegisterObserver(ITorrentDownloaderObserver * observer)
{
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	m_impl->observers.push_back(observer);
}

void Notifier::UnregisterObserver(ITorrentDownloaderObserver * observer)
{
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	m_impl->observers.erase(std::remove(m_impl->observers.begin(), m_impl->observers.end(), observer), m_impl->observers.end());
}

void Notifier::OnVideoFileUpdated()
{
	m_impl->NotifyAll(&ITorrentDownloaderObserver::OnVideoFileUpdated);
}

void Notifier::OnDownloadProgressChanged()
{
	m_impl->NotifyAll(&ITorrentDownloaderObserver::OnDownloadProgressChanged);
}

void Notifier::CannotPlayVideo()
{
	m_impl->NotifyAll(&ITorrentDownloaderObserver::OnCannotPlayVideo);
}

void Notifier::DownloadStarted()
{
	m_impl->NotifyAll(&ITorrentDownloaderObserver::OnDownloadStarted);
}

void Notifier::DownloadFinished()
{
	m_impl->NotifyAll(&ITorrentDownloaderObserver::OnDownloadFinished);
}
