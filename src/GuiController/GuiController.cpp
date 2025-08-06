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
	QSettings settings;
};

GuiController::GuiController(Notifier & notifier, QObject * parent)
	: QObject(parent)
	, IObserver(notifier)
	, m_downloader(notifier)
	, m_impl(std::make_unique<Impl>())
{
	m_engine.rootContext()->setContextProperty("guiController", this);
	m_engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
	if (m_engine.rootObjects().isEmpty())
	{
		LOG(ERROR) << "Failed to load QML file: qrc:/main.qml";
		return;
	}
}

GuiController::~GuiController()
{
}

void GuiController::DownloadWithTorrentFile(const QUrl & filePath)
{
	m_downloader.DownlloadWithTorrentFile(filePath.toLocalFile().toStdString());
}

QUrl TorrentPlayer::GuiController::GetVideoFile()
{
	const auto videoFile = m_downloader.GetVideoFile();
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

int TorrentPlayer::GuiController::GetDownloadProgress()
{
	return m_downloader.GetDownloadProgress();
}

QString TorrentPlayer::GuiController::GetSavePath()
{
	const auto defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
	const auto storedPath = m_impl->settings.value(PATH).toString();
	return storedPath.isEmpty() ? defaultPath : storedPath;
}

void TorrentPlayer::GuiController::SetSavePath(const QString & path)
{
	m_impl->settings.setValue(PATH, path);
	emit savePathChanged();
}
