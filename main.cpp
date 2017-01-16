#include <QApplication>
#include <QQmlApplicationEngine>

#include "WeatherStorage.h"

int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication app(argc, argv);
	
	qmlRegisterSingletonType<WeatherStorage>("WeatherStorage",1,0,"WeatherStorage",
											[](QQmlEngine *, QJSEngine *)->QObject*{ return new WeatherStorage();}  
	);
	qmlRegisterType<CityData>("WeatherStorage",1,0,"CityData");
	qmlRegisterType<WeatherData>("WeatherStorage",1,0,"WeatherData");
	
	qRegisterMetaType<CityData>();
	qRegisterMetaType<WeatherData>();
	
	QQmlApplicationEngine engine;
	engine.load(QUrl(QLatin1String("qrc:/main.qml")));
	
	return app.exec();
}
