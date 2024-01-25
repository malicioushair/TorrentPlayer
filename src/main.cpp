#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <stdlib.h>

int main(int argc, char * argv[])
{
	QGuiApplication app(argc, argv);
	QQmlApplicationEngine engine;
	engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
	if (engine.rootObjects().isEmpty())
		return EXIT_FAILURE;
	return QGuiApplication::exec();
}