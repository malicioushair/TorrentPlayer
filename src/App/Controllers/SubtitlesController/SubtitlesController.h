#pragma once

#include <QtCore/qtmetamacros.h>
#include <memory>

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "App/Models/SubtitleLanguageModel.h"

class SubtitlesController
	: public QObject
{
	Q_OBJECT
	QML_ELEMENT
	QML_SINGLETON

	Q_PROPERTY(int activeSubtitleTrack READ GetActiveSubtitleTrack WRITE SetActiveSubtitleTrack NOTIFY activeSubtitleTrackChanged)
	Q_PROPERTY(QStringList subtitleTracks READ GetSubtitleTracks NOTIFY subtitleTracksChanged)
	Q_PROPERTY(QString currentSubtitleText READ GetCurrentSubtitleText NOTIFY currentSubtitleTextChanged)
	Q_PROPERTY(qint64 subtitleOffset READ GetSubtitleOffset NOTIFY subtitleOffsetChanged)
	Q_PROPERTY(QString imdbId READ GetImdbId WRITE SetImdbId NOTIFY imdbIdChanged)
	Q_PROPERTY(QVariantMap userData READ GetUserData WRITE SetUserData NOTIFY userDataChanged)
	Q_PROPERTY(bool subdlConfigured READ GetSubdlConfigured NOTIFY userDataChanged)
	Q_PROPERTY(bool openSubtitlesConfigured READ GetOpenSubtitlesConfigured NOTIFY userDataChanged)
	Q_PROPERTY(bool autoFind READ GetAutoFind WRITE SetAutoFind NOTIFY autoFindChanged)
	Q_PROPERTY(QString preferredLanguage READ GetPreferredLanguage WRITE SetPreferredLanguage NOTIFY prefferedLanguageChanged)
	Q_PROPERTY(SubtitleLanguageModel * subtitleLanguages READ GetSubtitleLanguages CONSTANT)
	Q_PROPERTY(int fontSize READ GetFontSize WRITE SetFontSize NOTIFY fontSizeChanged)

public:
	SubtitlesController(QObject * parent = nullptr);
	~SubtitlesController();

	Q_INVOKABLE void DownloadSubtitles(const QString & language = {});
	Q_INVOKABLE void SetPlaybackPosition(qint64 positionMs);
	Q_INVOKABLE void IncreaseOffset();
	Q_INVOKABLE void DecreaseOffset();
	Q_INVOKABLE int IndexOfSubtitleLanguage(const QString & code) const;

	void SetVideoFile(const std::string & videoFile);

signals:
	void activeSubtitleTrackChanged();
	void subtitleTracksChanged();
	void currentSubtitleTextChanged();
	void subtitleOffsetChanged();
	void subtitleDownloadSucceeded();
	void imdbIdChanged();
	void userDataChanged();
	void showErrorMessage(const QString & text, const QString & description);
	void autoFindChanged();
	void prefferedLanguageChanged();
	void fontSizeChanged();

private:
	int GetActiveSubtitleTrack() const;
	void SetActiveSubtitleTrack(int trackIndex);
	QStringList GetSubtitleTracks() const;
	QString GetCurrentSubtitleText() const;
	qint64 GetSubtitleOffset() const;
	QString GetImdbId() const;
	void SetImdbId(const QString & imdbId);
	QVariantMap GetUserData() const;
	void SetUserData(const QVariantMap & userData);
	bool GetSubdlConfigured() const;
	bool GetOpenSubtitlesConfigured() const;
	bool GetAutoFind() const;
	void SetAutoFind(bool value);
	QString GetPreferredLanguage() const;
	void SetPreferredLanguage(const QString & lang);
	SubtitleLanguageModel * GetSubtitleLanguages();
	int GetFontSize() const;
	void SetFontSize(int fontSize);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
