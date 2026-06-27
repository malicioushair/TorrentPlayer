#include "GuiController.h"
#include "TorrentDownloader/Observer.h"

#include <QDateTime>
#include <QFileInfo>
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
}

struct GuiController::Impl
{
	Impl(Notifier & notifier)
		: downloader(notifier)
	{
	}

	QQmlApplicationEngine engine;
	TorrentDownloader downloader;
	QSettings settings;
	std::string videoFile {};
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
		m_impl->videoFile = filePath.path().toStdString();
	else
		emit showErrorMessage("Unknown file.", "Only video or torrent files are accepted.");
}

void TorrentPlayer::GuiController::OnReadyToPlayVideo()
{
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
