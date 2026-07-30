#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <QStringList>
#include <QtCore/qtmetamacros.h>

#include "TorrentDownloader/Observer.h"

namespace TorrentPlayer {
class GuiController
	: public QObject
	, public IObserver
{
	Q_OBJECT
	Q_DISABLE_COPY(GuiController)

public:
	Q_PROPERTY(QUrl videoFile READ GetVideoFile NOTIFY videoFileUpdated)
	Q_PROPERTY(int downloadProgress READ GetDownloadProgress NOTIFY downloadProgressChanged)
	Q_PROPERTY(QString savePath READ GetSavePath WRITE SetSavePath NOTIFY savePathChanged)
	Q_PROPERTY(bool frostedGlassEnabled READ GetFrostedGlassEnabled WRITE SetFrostedGlassEnabled NOTIFY frostedGlassEnabledChanged)
	Q_PROPERTY(QStringList audioTracks READ GetAudioTracks NOTIFY audioTracksChanged)
	Q_PROPERTY(int activeAudioTrack READ GetActiveAudioTrack WRITE SetActiveAudioTrack NOTIFY activeAudioTrackChanged)

signals:
	void videoFileUpdated();
	void downloadProgressChanged();
	void savePathChanged();
	void frostedGlassEnabledChanged();
	void audioTracksChanged();
	void activeAudioTrackChanged();
	void showErrorMessage(const QString & text, const QString & description);

public:
	GuiController(Notifier & notifier, QObject * parent = nullptr);
	~GuiController();

	Q_INVOKABLE bool IsDebug();
	Q_INVOKABLE void BumpHotReloadToken();
	Q_INVOKABLE QUrl GetVideoFile() const;
	Q_INVOKABLE void AddFile(const QUrl & filePath);

	void DownloadWithTorrentFile(const QUrl & filePath);

public: // IObserver
	void OnVideoFileUpdated() override;
	void OnDownloadProgressChanged() override;
	void OnCannotPlayVideo() override;

private:
	int GetDownloadProgress() const;
	QString GetSavePath() const;
	void SetSavePath(const QString & path);
	bool GetFrostedGlassEnabled() const;
	void SetFrostedGlassEnabled(bool enabled);
	QStringList GetAudioTracks() const;
	int GetActiveAudioTrack() const;
	void SetActiveAudioTrack(int trackIndex);
	void LoadAudioTracks(const QUrl & filePath);
	void UpdateAudioTracks();
	void UpdateSubtitlesVideoFile();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
} // namespace TorrentPlayer
