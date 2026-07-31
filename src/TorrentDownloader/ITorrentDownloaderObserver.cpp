#include "ITorrentDownloaderObserver.h"

#include "Notifier.h"

ITorrentDownloaderObserver::ITorrentDownloaderObserver(Notifier & notifier)
	: m_notifier(notifier)
{
	m_notifier.RegisterObserver(this);
}

ITorrentDownloaderObserver::~ITorrentDownloaderObserver()
{
	m_notifier.UnregisterObserver(this);
}