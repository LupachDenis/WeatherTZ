#ifndef CITIESSTORAGE_H
#define CITIESSTORAGE_H

#include <QObject>
#include <QMap>
#include <QPointF>
#include <QTimer>
#include <QtQml/QQmlListProperty>
#include <QFutureWatcher>
#include <QtPositioning/QGeoPositionInfoSource>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkSession;
QT_END_NAMESPACE

class CityData : public QObject{	
	Q_OBJECT
	
	Q_PROPERTY(int id MEMBER m_id)
	Q_PROPERTY(QString name MEMBER m_name)
	Q_PROPERTY(QString country MEMBER m_country)
	Q_PROPERTY(QPointF coord MEMBER m_coord)
	
public:
	explicit CityData(QObject* parent = nullptr) : QObject(parent){}
	CityData(const CityData& other);

	int m_id;
	QString m_name;
	QString m_country;
	QPointF m_coord;
};

Q_DECLARE_METATYPE(CityData)

class WeatherData : public QObject{
	Q_OBJECT	
	
	Q_PROPERTY(QDateTime dateTime READ dateTime NOTIFY dateTimeChanged)
	Q_PROPERTY(QString time READ time NOTIFY dateTimeChanged)
	Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
	Q_PROPERTY(QString icon READ icon NOTIFY iconChanged)
	
	Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
	Q_PROPERTY(QString temperatureText READ temperatureText NOTIFY temperatureChanged)
	
public:
	explicit WeatherData(QObject* parent = nullptr) : QObject(parent){}
	WeatherData(const WeatherData& other) : QObject(nullptr){
		m_dateTime = other.m_dateTime;
		m_description = other.m_description;
		m_icon = other.m_icon;
		m_temperature = other.m_temperature;
	}
	
	QDateTime dateTime()const{return m_dateTime;}
	void setDateTime(const QDateTime& val){
		m_dateTime = val;
		emit dateTimeChanged();
	}
	
	QString time()const;	
	
	QString description()const{		
		return m_description;
	}
	void setDescription(const QString& val){	
		m_description = val;
		emit descriptionChanged();
	}
	
	void setIcon(const QString& base);
	QString icon()const{return m_icon;}
	
	void setTemperature(double val){
		m_temperature = val;
		emit temperatureChanged();
	}
	double temperature()const{return m_temperature;}
	QString temperatureText()const{
		QString numStr = QString::number(m_temperature,'f',1);
		return QString::number(m_temperature,'f',1) + QStringLiteral(" °C");}
signals:
	void dateTimeChanged();
	void descriptionChanged();
	void iconChanged();
	void temperatureChanged();
private:
	QDateTime m_dateTime;
	QString m_description;
	QString m_icon;
	double m_temperature = 0;
};

Q_DECLARE_METATYPE(WeatherData)

class WeatherStorage : public QObject
{
	Q_OBJECT
	
	Q_PROPERTY(int cityId READ cityId WRITE setCityId)
	Q_PROPERTY(QString cityName READ cityName WRITE setCityName)
	Q_PROPERTY(QString cityNameToDisplay READ cityNameToDisplay NOTIFY cityNameToDisplayChanged)

	Q_PROPERTY(QString suggestionMask READ suggestionMask WRITE setSuggestionMask)
	Q_PROPERTY(QQmlListProperty<CityData> suggestion READ suggestion NOTIFY suggestionChanged)
	
	Q_PROPERTY(WeatherData* currentWeather READ currentWeather NOTIFY currentWeatherChanged)
	Q_PROPERTY(QQmlListProperty<WeatherData> forecastWeather READ forecastWeather NOTIFY forecastWeatherChanged)
	
	Q_PROPERTY(QString getWeatherTime READ getWeatherTime WRITE setGetWeatherTime NOTIFY getWeatherTimeChanged)
	
	Q_PROPERTY(bool hasValidWeather READ hasValidWeather WRITE setHasValidWeather NOTIFY hasValidWeatherChanged)
	Q_PROPERTY(bool hasValidForecast READ hasValidForecast WRITE setHasValidForecast NOTIFY hasValidForecastChanged)
	
	Q_PROPERTY(QPointF coordinates READ coordinates WRITE setCoordinates NOTIFY coordinatesChanged)
private:
	using CitiesByNameContainer = QMultiMap<QString, CityData*>;
	using CitiesByIdContainer = QMap<int,CityData*>;
	
	struct CitiesContainers{
		CitiesByIdContainer byId;
		CitiesByNameContainer byName;
	};
	
    enum PreferredSource{
        UnknownSource = 0,
        GpsSource,
        CityIdSource,
        CityNameSource
    };

	static const int refreshIntervalMs = 10*60*1000; // 10 min
public:
	explicit WeatherStorage(QObject *parent = nullptr);
	~WeatherStorage();
	
	bool hasValidWeather()const{return m_hasValidWeather;}
	void setHasValidWeather(bool val){
		m_hasValidWeather = val;
		emit hasValidWeatherChanged();
	}
	
	bool hasValidForecast()const{return m_hasValidForecast;}
	void setHasValidForecast(bool val){
		m_hasValidForecast = val;
		emit hasValidForecastChanged();
	}
	
	int cityId()const{return m_cityId;}
	void setCityId(int val);
protected:
	void setCityIdInternal(int val); // without sending data request
public:
	
	QString cityName()const{return m_cityName;}
	void setCityName(const QString& val);
protected:
	void setCityNameInternal(const QString& val); // without sending data request
public:
	
	QPointF coordinates()const{return m_coordinates;}
	void setCoordinates(const QPointF& val);
protected:
	void setCoordinatesInternal(const QPointF &val); // without sending data request
public:	
	
	QString suggestionMask()const{return m_suggestionMask;}
	void setSuggestionMask(const QString& val);	
	
	QString cityNameToDisplay()const;
	
	QString getWeatherTime()const{return m_getWeatherTime;}
	void setGetWeatherTime(const QString& time){
		m_getWeatherTime = tr("get at ") + time;
		emit getWeatherTimeChanged();
	}

	void updateSuggestion();
	
	QQmlListProperty<CityData> suggestion();
	
	Q_INVOKABLE static QString appKey(){return QStringLiteral("6868571b061d81b7a8ea993ef165a68f");}
	Q_INVOKABLE static int forecastItemsCount(){return 10;}
	
	Q_INVOKABLE double temperatureAxisMin()const;
	Q_INVOKABLE double temperatureAxisMax()const;
	
	WeatherData* currentWeather()const{return m_currentWeather;}
	
	QQmlListProperty<WeatherData> forecastWeather();
signals:
	void suggestionChanged();
	void cityNameToDisplayChanged();
	void coordinatesChanged();
	
	void hasValidWeatherChanged();
	void hasValidForecastChanged();
	
	void currentWeatherChanged();
	void forecastWeatherChanged();
	
	void getWeatherTimeChanged();	
public slots:
	Q_INVOKABLE	void refreshWeatherAndForecast();
	
private:
	void refreshWeatherOrForecast(bool isForecast);
	
	static CitiesContainers loadCityData();
	
	void parseWeatherJson(const QJsonObject& json, WeatherData* weatherData );	
private: // slots
	
	void onLoadCityDataFinished();
	void onNetworkSessionOpened();
    void onPositionUpdated(QGeoPositionInfo gpsPos);
    void onPositionError(QGeoPositionInfoSource::Error e);
	
	void onWeatherReply();
	void onForecastReply();	
private:	
	int m_cityId = 0;
	QString m_cityName;
	QPointF m_coordinates;
    PreferredSource m_preferredSource = UnknownSource;
	
	bool m_hasValidWeather = false;
	bool m_hasValidForecast = false;
	
	QString m_getWeatherTime;
	
	QString m_suggestionMask;
	CitiesByIdContainer m_citiesById;
	CitiesByNameContainer m_citiesByName;
	
	bool m_currentWeatherValid = false;	
	WeatherData* m_currentWeather;
	QList<WeatherData*>	m_forecastWeather;
	
	QNetworkAccessManager* m_networkManager;
	QNetworkSession* m_networkSession;
	
	QFutureWatcher<CitiesContainers>* m_futureWatcher;
	QList<CityData*> m_suggestionList;

    QGeoPositionInfoSource* m_geoPositionSource = nullptr;
	
	QTimer* m_refreshTimer;
	qint64 m_lastGpsUpdated = 0;
};

#endif // CITIESSTORAGE_H
