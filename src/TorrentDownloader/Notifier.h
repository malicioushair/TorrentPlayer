#pragma once

#include <mutex>
#include <vector>

class IObserver;

class Notifier
{
public:
	Notifier();
	virtual ~Notifier();

	void RegisterObserver(IObserver * observer);
	void UnregisterObserver(IObserver * observer);
	void OnReadyToPlayVideo();
	void OnDownloadProgressChanged();
	void CannotPlayVideo();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};