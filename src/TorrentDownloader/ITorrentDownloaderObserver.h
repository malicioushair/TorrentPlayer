#pragma once

class Notifier;

class ITorrentDownloaderObserver
{
public:
	ITorrentDownloaderObserver(Notifier & notifier);

	virtual ~ITorrentDownloaderObserver();

	virtual void OnVideoFileUpdated() = 0;
	virtual void OnDownloadProgressChanged() = 0;
	virtual void OnCannotPlayVideo() = 0;
	virtual void OnDownloadStarted() = 0;
	virtual void OnDownloadFinished() = 0;

private:
	Notifier & m_notifier;
};
