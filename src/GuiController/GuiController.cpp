#include "GuiController.h"
#include "TorrentDownloader/Observer.h"

#include <QFileInfo>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>

#include "glog/logging.h"

constexpr auto PATH = "PATH";

using namespace TorrentPlayer;

struct GuiController::Impl
{
	Impl(Notifier & notifier)
		: downloader(notifier)
	{
	}

	QQmlApplicationEngine engine;
	TorrentDownloader downloader;
	QSettings settings;
};

GuiController::GuiController(Notifier & notifier, QObject * parent)
	: QObject(parent)
	, IObserver(notifier)
	, m_impl(std::make_unique<Impl>(notifier))
{
	m_impl->engine.rootContext()->setContextProperty("guiController", this);
	m_impl->engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
	if (m_impl->engine.rootObjects().isEmpty())
	{
		LOG(ERROR) << "Failed to load QML file: qrc:/main.qml";
		return;
	}
}

GuiController::~GuiController() = default;

void GuiController::DownloadWithTorrentFile(const QUrl & filePath)
{
	const auto savePath = m_impl->settings.value(PATH).toString().toStdString();
	const auto defaultSavePath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation).toStdString();
	m_impl->downloader.DownloadWithTorrentFile(filePath.toLocalFile().toStdString(), !savePath.empty() ? savePath : defaultSavePath);
}

QUrl TorrentPlayer::GuiController::GetVideoFile() const
{
	const auto videoFile = m_impl->downloader.GetVideoFile();
	if (videoFile.empty() || QFileInfo(QString::fromStdString(videoFile)).isDir())
		return {};

	return QUrl::fromLocalFile(QString::fromStdString(videoFile));
}

void TorrentPlayer::GuiController::OnReadyToPlayVideo()
{
	emit readyToPlayVideo();
}

void TorrentPlayer::GuiController::OnDownloadProgressChanged()
{
	emit downloadProgressChanged();
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
