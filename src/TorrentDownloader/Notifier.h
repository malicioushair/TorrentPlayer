#pragma once

#include "TorrentDownloader/Observer.h"

#include <mutex>
#include <vector>

class Notifier
{
public:
	Notifier() = default;
	virtual ~Notifier() = default;

	void RegisterObserver(IObserver * observer)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_observers.push_back(observer);
	}

	void UnregisterObserver(IObserver * observer)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), observer), m_observers.end());
	}

	void OnReadyToPlayVideo()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto observer : m_observers)
			if (observer)
				observer->OnReadyToPlayVideo();
	}

	void OnDownloadProgressChanged()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto observer : m_observers)
			if (observer)
				observer->OnDownloadProgressChanged();
	}

private:
	std::vector<IObserver *> m_observers;
	std::mutex m_mutex;
};