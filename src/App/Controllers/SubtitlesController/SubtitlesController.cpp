#include "SubtitlesController.h"

#include <string>

#include <QSettings>
#include <QUrl>

#include "App/Controllers/UserDataStorage/UserDataStorage.h"
#include "SubtitleLoader/SubtitleLoader.h"

namespace {
constexpr auto SUBDL_API_KEY = "subDlApiKey";
constexpr auto USERNAME = "openSubtitlesUsername";
constexpr auto PASSWORD = "openSubtitlesPassword";
constexpr auto RECENT_IMDB_ID = "recentImdbId";
constexpr auto SUBTITLE_OFFSET_STEP_MS = 500;
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
	qint64 playbackPositionMs {};
	int subtitleOffsetMs {};
	SubtitleLoader subtitleLoader;

	void UpdatePlaybackPosition()
	{
		subtitleLoader.SetPlaybackPosition(playbackPositionMs - subtitleOffsetMs);
	}
};

SubtitlesController::SubtitlesController(QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(*this))
{
}

SubtitlesController::~SubtitlesController() = default;

void SubtitlesController::SetVideoFile(const std::string & videoFile)
{
	if (m_impl->videoFile != videoFile)
	{
		m_impl->playbackPositionMs = 0;
		if (m_impl->subtitleOffsetMs != 0)
			emit subtitleOffsetChanged();
	}

	m_impl->videoFile = videoFile;
	m_impl->subtitleLoader.SetVideoFile(QString::fromStdString(videoFile));
}

void SubtitlesController::DownloadSubtitles(const QString & language)
{
	m_impl->subtitleLoader.DownloadSubtitles(language);
}

void SubtitlesController::SetPlaybackPosition(qint64 positionMs)
{
	m_impl->playbackPositionMs = positionMs;
	m_impl->UpdatePlaybackPosition();
}

void SubtitlesController::IncreaseOffset()
{
	m_impl->subtitleOffsetMs += SUBTITLE_OFFSET_STEP_MS;
	m_impl->UpdatePlaybackPosition();
	emit subtitleOffsetChanged();
}

void SubtitlesController::DecreaseOffset()
{
	m_impl->subtitleOffsetMs -= SUBTITLE_OFFSET_STEP_MS;
	m_impl->UpdatePlaybackPosition();
	emit subtitleOffsetChanged();
}

bool SubtitlesController::GetSubdlConfigured() const
{
	return !m_impl->userData.value(SUBDL_API_KEY).toString().isEmpty();
}

bool SubtitlesController::GetOpenSubtitlesConfigured() const
{
	const auto username = m_impl->userData.value(USERNAME).toString().trimmed();
	const auto password = m_impl->userData.value(PASSWORD).toString().trimmed();
	return !username.isEmpty() && !password.isEmpty();
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

qint64 SubtitlesController::GetSubtitleOffset() const
{
	return m_impl->subtitleOffsetMs;
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
