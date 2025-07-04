#pragma once

class Notifier;

class IObserver
{
public:
	IObserver(Notifier & notifier);

	virtual ~IObserver();

	virtual void OnReadyToPlayVideo() = 0;
	virtual void OnDownloadProgressChanged() = 0;

private:
	Notifier & m_notifier;
};
