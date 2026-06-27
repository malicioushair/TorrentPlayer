#include "GuiController.h"
#include "TorrentDownloader/Observer.h"

#include <QDateTime>
#include <QFileInfo>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QMimeDatabase>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>
#include <QStringLiteral>
#include <QUrlQuery>

#include <memory>
#include <string>
#include <utility>

#include "glog/logging.h"

using namespace TorrentPlayer;

namespace {
constexpr auto PATH = "PATH";

class HotReloadUrlInterceptor
	: public QQmlAbstractUrlInterceptor
{
public:
	explicit HotReloadUrlInterceptor(std::string token = {})
		: m_token(std::move(token))
	{
	}

	void SetToken(const std::string & token)
	{
		m_token = token;
	}

	QUrl intercept(const QUrl & url, DataType type) override
	{
		if (true
			&& type != QQmlAbstractUrlInterceptor::QmlFile
			&& type != QQmlAbstractUrlInterceptor::JavaScriptFile
			&& type != QQmlAbstractUrlInterceptor::QmldirFile)
			return url;

		if (m_token.empty())
			return url;

		const auto scheme = url.scheme();
		if (scheme != QStringLiteral("file") && scheme != QStringLiteral("qrc"))
			return url;

		QUrl result(url);
		QUrlQuery query(result);
		query.removeAllQueryItems("r");
		query.addQueryItem("r", QString::fromStdString(m_token));
		result.setQuery(query);

		return result;
	}

private:
	std::string m_token;
};

bool IsVideoFile(const QUrl & url)
{
	if (!url.isLocalFile())
		return false;
	const QFileInfo fileInfo(url.toLocalFile());
	if (!fileInfo.isFile())
		return false;
	const QMimeDatabase mimeDatabase;
	const QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo);
	return mimeType.name().startsWith("video/");
}

QString GetAudioTrackLabel(const QMediaMetaData & track, int index)
{
	QStringList details;
	const auto title = track.stringValue(QMediaMetaData::Title);
	const auto language = track.stringValue(QMediaMetaData::Language);
	const auto codec = track.stringValue(QMediaMetaData::AudioCodec);

	if (!title.isEmpty())
		details.push_back(title);
	if (!language.isEmpty())
		details.push_back(language);
	if (!codec.isEmpty())
		details.push_back(codec);

	if (details.empty())
		return QStringLiteral("Track %1").arg(index + 1);

	return QStringLiteral("Track %1: %2").arg(index + 1).arg(details.join(QStringLiteral(" / ")));
}
}

struct GuiController::Impl
{
	Impl(Notifier & notifier)
		: downloader(notifier)
	{
	}

	QQmlApplicationEngine engine;
	TorrentDownloader downloader;
	QMediaPlayer mediaInfoReader;
	QSettings settings;
	std::string videoFile {};
	QStringList audioTracks;
	int activeAudioTrack { -1 };
	std::unique_ptr<HotReloadUrlInterceptor> interceptor { std::make_unique<HotReloadUrlInterceptor>() };

	void LoadQml()
	{
		engine.
#ifndef NDEBUG
			load(MAIN_QML)
#else
			loadFromModule("TorrentPlayer", "Main")
#endif
			;
	}
};

GuiController::GuiController(Notifier & notifier, QObject * parent)
	: QObject(parent)
	, IObserver(notifier)
	, m_impl(std::make_unique<Impl>(notifier))
{
	m_impl->engine.rootContext()->setContextProperty("guiController", this);
	m_impl->engine.addImportPath("qrc:/qt/qml");
	m_impl->engine.addUrlInterceptor(m_impl->interceptor.get());
	connect(&m_impl->mediaInfoReader, &QMediaPlayer::tracksChanged, this, &GuiController::UpdateAudioTracks);
	m_impl->LoadQml();

	if (m_impl->engine.rootObjects().isEmpty())
	{
		LOG(ERROR) << "Failed to load QML";
		throw std::runtime_error("Failed to load QML");
	}
}

GuiController::~GuiController() = default;

bool GuiController::IsDebug()
{
	return
#ifndef NDEBUG
		true
#else
		false
#endif
		;
}

void GuiController::BumpHotReloadToken()
{
	m_impl->interceptor->SetToken(QString::number(QDateTime::currentSecsSinceEpoch()).toStdString());
	m_impl->engine.clearComponentCache();
}

void GuiController::DownloadWithTorrentFile(const QUrl & filePath)
{
	const auto savePath = m_impl->settings.value(PATH).toString().toStdString();
	const auto defaultSavePath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation).toStdString();
	m_impl->downloader.DownloadWithTorrentFile(filePath.toLocalFile().toStdString(), !savePath.empty() ? savePath : defaultSavePath);
	m_impl->videoFile = m_impl->downloader.GetVideoFile();
	LoadAudioTracks(GetVideoFile());
}

QUrl TorrentPlayer::GuiController::GetVideoFile() const
{
	if (m_impl->videoFile.empty() || QFileInfo(QString::fromStdString(m_impl->videoFile)).isDir())
		return {};

	return QUrl::fromLocalFile(QString::fromStdString(m_impl->videoFile));
}

void TorrentPlayer::GuiController::AddFile(const QUrl & filePath)
{
	const QFileInfo fileInfo(filePath.toLocalFile());
	if (fileInfo.suffix() == "torrent")
		DownloadWithTorrentFile(filePath);
	else if (IsVideoFile(filePath))
	{
		m_impl->videoFile = filePath.path().toStdString();
		LoadAudioTracks(filePath);
	}
	else
		emit showErrorMessage("Unknown file.", "Only video or torrent files are accepted.");
}

void TorrentPlayer::GuiController::OnReadyToPlayVideo()
{
	m_impl->videoFile = m_impl->downloader.GetVideoFile();
	LoadAudioTracks(GetVideoFile());
	emit readyToPlayVideo();
}

void TorrentPlayer::GuiController::OnDownloadProgressChanged()
{
	emit downloadProgressChanged();
}

void TorrentPlayer::GuiController::OnCannotPlayVideo()
{
	emit showErrorMessage(
		tr("Moov atom was not found."),
		tr("Moov atom was not found at the beginning of the video. Wait until the video is downloaded."));
}

int TorrentPlayer::GuiController::GetDownloadProgress() const
{
	return m_impl->downloader.GetDownloadProgress();
}

QString TorrentPlayer::GuiController::GetSavePath() const
{
	if (const auto storedPath = m_impl->settings.value(PATH).toString(); storedPath.isEmpty())
		return storedPath;

	return QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
}

void TorrentPlayer::GuiController::SetSavePath(const QString & path)
{
	m_impl->settings.setValue(PATH, path);
	emit savePathChanged();
}

QStringList TorrentPlayer::GuiController::GetAudioTracks() const
{
	return m_impl->audioTracks;
}

int TorrentPlayer::GuiController::GetActiveAudioTrack() const
{
	return m_impl->activeAudioTrack;
}

void TorrentPlayer::GuiController::SetActiveAudioTrack(int trackIndex)
{
	if (m_impl->activeAudioTrack == trackIndex)
		return;

	m_impl->activeAudioTrack = trackIndex;
	emit activeAudioTrackChanged();
}

void TorrentPlayer::GuiController::LoadAudioTracks(const QUrl & filePath)
{
	if (filePath.isEmpty())
		return;

	if (!m_impl->audioTracks.empty())
	{
		m_impl->audioTracks.clear();
		emit audioTracksChanged();
	}

	SetActiveAudioTrack(-1);
	m_impl->mediaInfoReader.setSource(filePath);
}

void TorrentPlayer::GuiController::UpdateAudioTracks()
{
	QStringList audioTracks;
	const auto mediaTracks = m_impl->mediaInfoReader.audioTracks();
	for (int i = 0; i < mediaTracks.size(); ++i)
		audioTracks.push_back(GetAudioTrackLabel(mediaTracks[i], i));

	if (m_impl->audioTracks != audioTracks)
	{
		m_impl->audioTracks = audioTracks;
		emit audioTracksChanged();
	}

	if (audioTracks.empty())
		SetActiveAudioTrack(-1);
	else if (m_impl->activeAudioTrack < 0 || m_impl->activeAudioTrack >= audioTracks.size())
		SetActiveAudioTrack(0);
}
