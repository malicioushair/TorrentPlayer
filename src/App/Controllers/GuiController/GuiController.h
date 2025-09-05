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
	Q_PROPERTY(QString savePath READ GetSavePath WRITE SetSavePath NOTIFY savePathChanged)

signals:
	void readyToPlayVideo();
	void downloadProgressChanged();
	void savePathChanged();
	void showErrorMessage(const QString & text, const QString & description);

public:
	GuiController(Notifier & notifier, QObject * parent = nullptr);
	~GuiController();

	Q_INVOKABLE void DownloadWithTorrentFile(const QUrl & filePath);
	Q_INVOKABLE QUrl GetVideoFile() const;
	Q_INVOKABLE bool NeedShowMenuBar() const;

public: // IObserver
	void OnReadyToPlayVideo() override;
	void OnDownloadProgressChanged() override;
	void OnCannotPlayVideo() override;

private:
	int GetDownloadProgress() const;
	QString GetSavePath() const;
	void SetSavePath(const QString & path);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
} // namespace TorrentPlayer