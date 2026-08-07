#include <cstdlib>
#include <exception>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStandardPaths>

#include "Controllers/GuiController/GuiController.h"
#include "TorrentDownloader/Notifier.h"

#include "glog/logging.h"

void InitLogging(const char * execName)
{
	const auto logDir = QDir(QString::fromStdString(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString() + "/logs"));
	if (!logDir.exists())
	{
		if (!logDir.mkpath(logDir.absolutePath()))
		{
			LOG(ERROR) << "Failed to create log directory: " << logDir.absolutePath().toStdString();
			return;
		}
	}

	FLAGS_log_dir = logDir.absolutePath().toStdString();
	FLAGS_alsologtostderr = true;
	google::InitGoogleLogging(execName);
}

int main(int argc, char * argv[])
{
	try
	{
		QCoreApplication::setOrganizationName("dv");
		QCoreApplication::setApplicationName("TorrentPlayer");
		InitLogging(argv[0]);

		LOG(INFO) << "Starting TorrentPlayer application";

		QGuiApplication app(argc, argv);
		QCommandLineParser commandLineParser;
		commandLineParser.addHelpOption();
		const QCommandLineOption smokeTestOption(
			QStringLiteral("smoke-test"),
			QStringLiteral("Exit after application initialization."));
		commandLineParser.addOption(smokeTestOption);
		commandLineParser.process(app);

		Notifier notifier;
		TorrentPlayer::GuiController guiController(notifier);
		if (commandLineParser.isSet(smokeTestOption))
			return EXIT_SUCCESS;

		return QGuiApplication::exec();
	}
	catch (const std::exception & ex)
	{
		LOG(ERROR) << "Exception caught in main:" << ex.what();
	}
	catch (...)
	{
		LOG(ERROR) << "Unknown exception caught in main.";
	}

	return EXIT_FAILURE;
}
