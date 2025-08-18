#include "Notifier.h"

#include <functional>
#include <memory>

#include "TorrentDownloader/Observer.h"

namespace {

}

struct Notifier::Impl
{
	std::vector<IObserver *> observers;
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

void Notifier::RegisterObserver(IObserver * observer)
{
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	m_impl->observers.push_back(observer);
}

void Notifier::UnregisterObserver(IObserver * observer)
{
	std::lock_guard<std::mutex> lock(m_impl->mutex);
	m_impl->observers.erase(std::remove(m_impl->observers.begin(), m_impl->observers.end(), observer), m_impl->observers.end());
}

void Notifier::OnReadyToPlayVideo()
{
	m_impl->NotifyAll(&IObserver::OnReadyToPlayVideo);
}

void Notifier::OnDownloadProgressChanged()
{
	m_impl->NotifyAll(&IObserver::OnDownloadProgressChanged);
}

void Notifier::CannotPlayVideo()
{
	m_impl->NotifyAll(&IObserver::OnCannotPlayVideo);
}