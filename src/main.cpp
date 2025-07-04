#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "GuiController/GuiController.h"
#include "TorrentDownloader/Notifier.h"

int main(int argc, char * argv[])
{
	QGuiApplication app(argc, argv);
	Notifier notifier;
	TorrentPlayer::GuiController guiController(notifier);
	return QGuiApplication::exec();
}
