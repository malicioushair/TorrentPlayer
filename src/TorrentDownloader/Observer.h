#pragma once

class Notifier;

class IObserver
{
public:
	IObserver(Notifier & notifier);

	virtual ~IObserver();

	virtual void OnVideoFileUpdated() = 0;
	virtual void OnDownloadProgressChanged() = 0;
	virtual void OnCannotPlayVideo() = 0;

private:
	Notifier & m_notifier;
};
