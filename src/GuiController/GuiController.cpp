#include "GuiController.h"
#include "TorrentDownloader/Observer.h"

#include <QQmlContext>
#include <QTimer>

#include "glog/logging.h"

using namespace TorrentPlayer;

GuiController::GuiController(Notifier & notifier, QObject * parent)
	: QObject(parent)
	, IObserver(notifier)
	, m_downloader(notifier)
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
	return QUrl::fromLocalFile(QString::fromStdString(m_downloader.GetVideoFile()));
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