#pragma once

#include "TorrentDownloader/TorrentDownloader.h"

#include <QObject>
#include <QQmlApplicationEngine>

#include "TorrentDownloader/Observer.h"

namespace TorrentPlayer {
class GuiController
	: public QObject
	, public IObserver
{
	Q_OBJECT
	Q_DISABLE_COPY(GuiController)

	Q_PROPERTY(QUrl videoFile READ GetVideoFile NOTIFY readyToPlayVideo)
	Q_PROPERTY(int downloadProgress READ GetDownloadProgress NOTIFY downloadProgressChanged)

signals:
	void readyToPlayVideo();
	void downloadProgressChanged();

public:
	GuiController(Notifier & notifier, QObject * parent = nullptr);
	~GuiController();

	Q_INVOKABLE void DownloadWithTorrentFile(const QUrl & filePath);
	Q_INVOKABLE QUrl GetVideoFile();

public: // IObserver
	void OnReadyToPlayVideo() override;
	void OnDownloadProgressChanged() override;

private:
	int GetDownloadProgress();

private:
	QQmlApplicationEngine m_engine;
	TorrentDownloader m_downloader;
};
} // namespace TorrentPlayer