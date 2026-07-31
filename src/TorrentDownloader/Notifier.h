#pragma once

#include <memory>
#include <mutex>
#include <vector>

class ITorrentDownloaderObserver;

class Notifier
{
public:
	Notifier();
	virtual ~Notifier();

	void RegisterObserver(ITorrentDownloaderObserver * observer);
	void UnregisterObserver(ITorrentDownloaderObserver * observer);
	void OnVideoFileUpdated();
	void OnDownloadProgressChanged();
	void CannotPlayVideo();
	void DownloadStarted();
	void DownloadFinished();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};