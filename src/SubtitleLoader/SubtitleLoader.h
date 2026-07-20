#pragma once

#include <memory>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QtGlobal>

class SubtitleLoader
	: public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(SubtitleLoader)

public:
	explicit SubtitleLoader(QObject * parent = nullptr);
	~SubtitleLoader();

	void SetVideoFile(const QString & videoPath);
	void SetUserData(const QVariantMap & userData);
	void SetImdbId(const QString & imdbId);
	void DownloadSubtitles(const QString & language);
	void SetPlaybackPosition(qint64 positionMs);
	void SetActiveSubtitleTrack(int trackIndex);

	int GetActiveSubtitleTrack() const;
	QStringList GetSubtitleTracks() const;
	QString GetCurrentSubtitleText() const;

signals:
	void subtitleTracksChanged();
	void activeSubtitleTrackChanged();
	void currentSubtitleTextChanged();
	void showErrorMessage(const QString & text, const QString & description);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
