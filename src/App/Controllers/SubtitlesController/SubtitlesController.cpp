#include "SubtitlesController.h"

#include <string>

#include <QSettings>
#include <QUrl>

#include "App/Controllers/UserDataStorage/UserDataStorage.h"
#include "App/SubtitleLoader/SubtitleLoader.h"

namespace {
constexpr auto RECENT_IMDB_ID = "recentImdbId";
}

struct SubtitlesController::Impl
{
	explicit Impl(SubtitlesController & controller)
		: controller(controller)
		, userData(TorrentPlayer::CredentialStore::ReadUserData())
		, imdbId(settings.value(RECENT_IMDB_ID).toString())
		, subtitleLoader(&controller)
	{
		QObject::connect(&subtitleLoader, &SubtitleLoader::showErrorMessage, &controller, &SubtitlesController::showErrorMessage);
		QObject::connect(&subtitleLoader, &SubtitleLoader::activeSubtitleTrackChanged, &controller, &SubtitlesController::activeSubtitleTrackChanged);
		QObject::connect(&subtitleLoader, &SubtitleLoader::subtitleTracksChanged, &controller, &SubtitlesController::subtitleTracksChanged);
		QObject::connect(&subtitleLoader, &SubtitleLoader::currentSubtitleTextChanged, &controller, &SubtitlesController::currentSubtitleTextChanged);
	}

	SubtitlesController & controller;
	QVariantMap userData;
	QSettings settings;
	QString imdbId;
	std::string videoFile {};
	SubtitleLoader subtitleLoader;
};

SubtitlesController::SubtitlesController(QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(*this))
{
}

SubtitlesController::~SubtitlesController() = default;

void SubtitlesController::SetVideoFile(const std::string & videoFile)
{
	m_impl->videoFile = videoFile;
	m_impl->subtitleLoader.SetVideoFile(QString::fromStdString(videoFile));
}

void SubtitlesController::DownloadSubtitles(const QString & language)
{
	m_impl->subtitleLoader.DownloadSubtitles(language);
}

void SubtitlesController::SetPlaybackPosition(qint64 positionMs)
{
	m_impl->subtitleLoader.SetPlaybackPosition(positionMs);
}

int SubtitlesController::GetActiveSubtitleTrack() const
{
	return m_impl->subtitleLoader.GetActiveSubtitleTrack();
}

void SubtitlesController::SetActiveSubtitleTrack(int trackIndex)
{
	m_impl->subtitleLoader.SetActiveSubtitleTrack(trackIndex);
}

QStringList SubtitlesController::GetSubtitleTracks() const
{
	return m_impl->subtitleLoader.GetSubtitleTracks();
}

QString SubtitlesController::GetCurrentSubtitleText() const
{
	return m_impl->subtitleLoader.GetCurrentSubtitleText();
}

QString SubtitlesController::GetImdbId() const
{
	return m_impl->imdbId;
}

void SubtitlesController::SetImdbId(const QString & imdbId)
{
	if (m_impl->imdbId == imdbId)
		return;

	m_impl->imdbId = imdbId;
	m_impl->settings.setValue(RECENT_IMDB_ID, imdbId);
	m_impl->subtitleLoader.SetImdbId(imdbId);
	emit imdbIdChanged();
}

QVariantMap SubtitlesController::GetUserData() const
{
	return m_impl->userData;
}

void SubtitlesController::SetUserData(const QVariantMap & userData)
{
	TorrentPlayer::CredentialStore::StoreUserData(userData);
	m_impl->userData = userData;
	m_impl->subtitleLoader.SetUserData(userData);
	emit userDataChanged();
}
