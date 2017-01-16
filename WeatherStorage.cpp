#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <QNetworkReply>
#include <QCoreApplication>

#include "WeatherStorage.h"

#define ZERO_KELVIN 273.15

CityData::CityData(const CityData& other) : QObject(nullptr)
{
	m_id = other.m_id;
	m_name = other.m_name;
	m_country = other.m_country;
	m_coord = other.m_coord;
	
}

QString WeatherData::time() const{
	return QLocale::system().toString(m_dateTime.time(),QLocale::ShortFormat);
}

void WeatherData::setIcon(const QString& base){ 
	if (base.isEmpty())
		m_icon = QString();
	else
		m_icon = QStringLiteral("qrc:/weathericons/") + base + QStringLiteral(".png");
	emit iconChanged();
}

WeatherStorage::WeatherStorage(QObject *parent) : QObject(parent)
{
	m_futureWatcher = new QFutureWatcher<CitiesContainers>(this);
	connect(m_futureWatcher, &QFutureWatcherBase::finished, this, &WeatherStorage::onLoadCityDataFinished);
	connect(m_futureWatcher, &QFutureWatcherBase::finished, m_futureWatcher, &QObject::deleteLater);
	
	m_futureWatcher->setFuture( QtConcurrent::run(&WeatherStorage::loadCityData) );
	
	m_currentWeather = new WeatherData(this);
	for (int i=0;i<forecastItemsCount();++i)
		m_forecastWeather.append(new WeatherData(this));
	
	m_networkManager = new QNetworkAccessManager(this);
	
	m_networkSession = new QNetworkSession(QNetworkConfigurationManager().defaultConfiguration(), this);
	connect(m_networkSession, &QNetworkSession::opened, this, &WeatherStorage::onNetworkSessionOpened);
	if (m_networkSession->isOpen())
		onNetworkSessionOpened();
	else
		m_networkSession->open();
	
	m_refreshTimer = new QTimer(this);
	m_refreshTimer->setInterval(refreshIntervalMs);
	connect(m_refreshTimer, &QTimer::timeout, this, &WeatherStorage::refreshWeatherAndForecast);
	m_refreshTimer->start();
}

WeatherStorage::~WeatherStorage()
{
	if (m_futureWatcher){
		m_futureWatcher->waitForFinished();
		m_citiesById = m_futureWatcher->result().byId;
	}
	
	m_suggestionList.clear();

	for (CityData* city : m_citiesById)	
		delete city;
	m_citiesById.clear();
	m_citiesByName.clear();
}

void WeatherStorage::setCityId(int val){
	setCityIdInternal(val);
	setHasValidWeather(false);
	setHasValidForecast(false);
    m_preferredSource = CityIdSource;
	refreshWeatherAndForecast();
}

void WeatherStorage::setCityIdInternal(int val){
	m_cityId = val;
	m_cityName = QString();
	
    if (m_preferredSource != GpsSource){
        if (CityData* cityData = m_citiesById.value(val)){
            setCoordinatesInternal(cityData->m_coord);
        }else{
            setCoordinatesInternal(QPointF());
        }
    }
	
	emit cityNameToDisplayChanged();
}

void WeatherStorage::setCityName(const QString& val){
	setCityNameInternal(val);
	setHasValidWeather(false);
	setHasValidForecast(false);
    m_preferredSource = CityNameSource;
    refreshWeatherAndForecast();
}

void WeatherStorage::setCityNameInternal(const QString& val){
	m_cityId = 0;
	m_cityName = val;	
	emit cityNameToDisplayChanged();
}

void WeatherStorage::setCoordinates(const QPointF& val){
	setCoordinatesInternal(val);
	if (QDateTime::currentMSecsSinceEpoch() - m_lastGpsUpdated < refreshIntervalMs)
		return;
	
	m_lastGpsUpdated = QDateTime::currentMSecsSinceEpoch();
    setHasValidWeather(false);
	setHasValidForecast(false);
    m_preferredSource = GpsSource;
    refreshWeatherAndForecast();
}

void WeatherStorage::setCoordinatesInternal(const QPointF& val){
	m_coordinates = val;
	emit coordinatesChanged();
}

QString WeatherStorage::cityNameToDisplay()const{
	if (m_cityId){
		if (CityData* cd = m_citiesById.value(m_cityId))
			return cd->m_name + QStringLiteral(",") + cd->m_country;
		return QString();
	}
	if (!m_cityName.isEmpty())
		return m_cityName;
	return QStringLiteral("???");
}

void WeatherStorage::onLoadCityDataFinished(){
	m_citiesById = m_futureWatcher->result().byId;
	m_citiesByName = m_futureWatcher->result().byName;
	m_futureWatcher = nullptr;
	updateSuggestion();	
}

WeatherStorage::CitiesContainers WeatherStorage::loadCityData(){
	
	QFile file(QStringLiteral(":/citylist"));
	file.open(QIODevice::ReadOnly);
	const QByteArray json = qUncompress( file.readAll() );
	
	CitiesContainers cities;
	
	int startIndex = 0;
	while(startIndex < json.size()){
#ifdef QT_DEBUG
//		if (cities.size() >= 1000)
//			break;
#endif
		
		const char* startBuffer = json.constData() + startIndex;		
		const char* finded = (const char*)memchr(startBuffer, '\n', json.size() - startIndex);
		if (!finded)
			break;
		startIndex = finded - json.constData() + 1;
		if (startIndex < json.size() && json.constData()[startIndex] == '\r')
			++startIndex;		
		
		int findedSize = finded - startBuffer;
		
		QByteArray source(startBuffer, findedSize );
	
		QJsonDocument jdoc = QJsonDocument::fromJson(source);
		if (jdoc.isEmpty())
			break;
		
		QJsonObject jcity = jdoc.object();
		int id = jcity.value(QStringLiteral("_id")).toInt();
		QString name = jcity.value(QStringLiteral("name")).toString();
		QString country = jcity.value(QStringLiteral("country")).toString();
		QJsonObject jcoord = jcity.value(QStringLiteral("coord")).toObject();
		QPointF coord(jcoord.value(QStringLiteral("lon")).toDouble(),jcoord.value(QStringLiteral("lat")).toDouble());
		
		CityData* cityData = new CityData();
		cityData->m_id = id;
		cityData->m_name = name;
		cityData->m_country = country;
		cityData->m_coord = coord;
		
		cities.byId.insert(id, cityData);
		cities.byName.insert( (name + QStringLiteral(",") + country).toLower(), cityData );
	}
	return cities;
}

void WeatherStorage::updateSuggestion()
{
	static const int maxSuggestionsCount = 7;
	
	QList<CityData*> newList;
	
	if (!m_suggestionMask.isEmpty()){	
		QString lowercase = m_suggestionMask.toLower();	
		CitiesByNameContainer::const_iterator it = m_citiesByName.lowerBound(lowercase);
		
		do{
			if (maxSuggestionsCount >= 0 && newList.size() >= maxSuggestionsCount)
				break;
			
			if (it == m_citiesByName.end())
				break;
			QString keyValue = it.key();
			if (keyValue.mid(0,lowercase.size()) != lowercase)
				break;
			newList << *it;
			++it;
			
		}while(true);
	}
	
	if (newList == m_suggestionList)
		return;
	
	m_suggestionList = newList;
	emit suggestionChanged();
}

void WeatherStorage::setSuggestionMask(const QString& val){ 
	if (val == m_suggestionMask)
		return;
	m_suggestionMask = val; 
	updateSuggestion();
}

QQmlListProperty<CityData> WeatherStorage::suggestion(){
	return QQmlListProperty<CityData>(this,m_suggestionList);
}

double WeatherStorage::temperatureAxisMin() const
{
	QList<WeatherData*>::const_iterator it = std::min_element( m_forecastWeather.constBegin(), m_forecastWeather.constEnd(), 
															   [](WeatherData* first, WeatherData* second)->bool{return first->temperature() < second->temperature();} 
			);
	return std::floor((*it)->temperature());
}

double WeatherStorage::temperatureAxisMax() const
{
	QList<WeatherData*>::const_iterator it = std::max_element( m_forecastWeather.constBegin(), m_forecastWeather.constEnd(), 
															   [](WeatherData* first, WeatherData* second)->bool{return first->temperature() < second->temperature();} 
			);
	return std::ceil((*it)->temperature());
}

QQmlListProperty<WeatherData> WeatherStorage::forecastWeather()
{
	return QQmlListProperty<WeatherData>(this,m_forecastWeather);
}

void WeatherStorage::onNetworkSessionOpened(){
    m_geoPositionSource = QGeoPositionInfoSource::createDefaultSource(this);
    if (!m_geoPositionSource)
        return;
    connect(m_geoPositionSource, &QGeoPositionInfoSource::positionUpdated, this, &WeatherStorage::onPositionUpdated);
    connect(m_geoPositionSource, static_cast<void(QGeoPositionInfoSource::*)(QGeoPositionInfoSource::Error)>(&QGeoPositionInfoSource::error), this, &WeatherStorage::onPositionError);
    m_geoPositionSource->startUpdates();
}

void WeatherStorage::onPositionUpdated(QGeoPositionInfo gpsPos){

    if (m_preferredSource == UnknownSource)
        m_preferredSource = GpsSource;
    if (m_preferredSource != GpsSource)
        return;

    QGeoCoordinate gpsCoord = gpsPos.coordinate();
    QPointF newCoords(gpsCoord.longitude(),gpsCoord.latitude());
    setCoordinates(newCoords);
}

void WeatherStorage::onPositionError(QGeoPositionInfoSource::Error e){
    qDebug().noquote() << e;
}

void WeatherStorage::refreshWeatherAndForecast(){
	m_refreshTimer->stop();
	m_refreshTimer->start();
	refreshWeatherOrForecast(false);
}

void WeatherStorage::refreshWeatherOrForecast(bool isForecast){
    if (m_preferredSource == UnknownSource)
        return;

	QUrlQuery query;
	query.addQueryItem(QStringLiteral("APPID"), appKey());
	query.addQueryItem(QStringLiteral("mode"), QStringLiteral("json"));
	query.addQueryItem(QStringLiteral("cnt"), QString::number(forecastItemsCount()));

    if (m_preferredSource == CityIdSource){
		query.addQueryItem(QStringLiteral("id"), QString::number(m_cityId));
    }else if (m_preferredSource == CityNameSource){
		query.addQueryItem(QStringLiteral("q"), m_cityName);
    }else if (m_preferredSource == GpsSource){
        query.addQueryItem(QStringLiteral("lat"), QString::number(m_coordinates.y(),'f',2));
        query.addQueryItem(QStringLiteral("lon"), QString::number(m_coordinates.x(),'f',2));
    }

	QString urlStr = isForecast ? 
				QStringLiteral("http://api.openweathermap.org/data/2.5/forecast") :
				QStringLiteral("http://api.openweathermap.org/data/2.5/weather");
	
	QUrl url(urlStr);
	url.setQuery(query);
	
    qDebug().noquote() << url.toString();

	QNetworkReply* reply = m_networkManager->get(QNetworkRequest(url));
	connect(reply, &QNetworkReply::finished, this, isForecast ? &WeatherStorage::onForecastReply : &WeatherStorage::onWeatherReply);
	connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void WeatherStorage::parseWeatherJson(const QJsonObject &json, WeatherData *weatherData)
{
	QJsonValue jvalue = json.value(QStringLiteral("weather"));
	if (jvalue.isArray()){
		QJsonArray weatherArray = jvalue.toArray();
		if (weatherArray.size() >= 1){
			QJsonValue jweatherValue = weatherArray.at(0);
			if (jweatherValue.isObject()){
				QJsonObject jweather = jweatherValue.toObject();
				
				weatherData->setDescription(jweather.value(QStringLiteral("description")).toString());
				weatherData->setIcon(jweather.value(QStringLiteral("icon")).toString());
			}
		}		
	}
	
	jvalue = json.value(QStringLiteral("dt"));
	if (jvalue.isDouble()){
		qint64 msecs = (qint64)jvalue.toDouble()*1000;
		weatherData->setDateTime(QDateTime::fromMSecsSinceEpoch(msecs));
	}
	
	jvalue = json.value(QStringLiteral("main"));
	if (jvalue.isObject()){
		QJsonObject jmain = jvalue.toObject();

		weatherData->setTemperature( jmain.value(QStringLiteral("temp")).toDouble() - ZERO_KELVIN);
	}
}

void WeatherStorage::onWeatherReply()
{
	QNetworkReply *networkReply = qobject_cast<QNetworkReply*>(sender());
    if (!networkReply)
        return;	
	
	if (networkReply->error()){
		qDebug().noquote() << networkReply->errorString();
		return;
	}
	
	QByteArray replyText = networkReply->readAll();	
	
	QJsonDocument document = QJsonDocument::fromJson(replyText);
	if (!document.isObject()){
		qDebug().noquote() << replyText;
		return;
	}
	
	//qDebug().noquote() << document.toJson();
	
	QJsonObject jobject = document.object();
	parseWeatherJson(jobject, m_currentWeather);
	
	// check response cityId and cityName if needs:	
	bool idFinded = false;
	QJsonValue jv = jobject.value(QStringLiteral("id"));
	if (jv.isDouble()){
		int newid = jv.toInt();		
		if (newid == m_cityId){
			idFinded = true;
		}else if (m_citiesById.contains(newid)){
			setCityIdInternal(newid);
			idFinded = true;
		}
	}
    if (idFinded && m_preferredSource == CityNameSource)
        m_preferredSource = CityIdSource;

	if (!idFinded){		
		jv = jobject.value(QStringLiteral("name"));
		if (jv.isString()){
			QString name = jv.toString();
			
			jv = jobject.value(QStringLiteral("sys"));
			if (jv.isObject()){
				QJsonObject jsys = jv.toObject();
				jv = jsys.value(QStringLiteral("country"));
				if (jv.isString())
					name += QStringLiteral(",") + jv.toString();	
			}
			setCityNameInternal(name);
        }
	}

    if (!idFinded && m_preferredSource != GpsSource){
        jv = jobject.value(QStringLiteral("coord"));
        if (jv.isObject()){
            QJsonObject jcoord = jv.toObject();
            setCoordinatesInternal(QPointF(jcoord.value(QStringLiteral("lon")).toDouble(),jcoord.value(QStringLiteral("lat")).toDouble()));
        }
    }

	currentWeatherChanged();
	
	// let`s refresh forecast now
	refreshWeatherOrForecast(true);
	
    setGetWeatherTime(QLocale::system().toString(QDateTime::currentDateTime(),QLocale::ShortFormat));
	setHasValidWeather(true);
}

void WeatherStorage::onForecastReply(){
	QNetworkReply *networkReply = qobject_cast<QNetworkReply*>(sender());
    if (!networkReply)
        return;	
	
	if (networkReply->error()){
		qDebug().noquote() << networkReply->errorString();
		return;
	}
	
	QByteArray replyText = networkReply->readAll();	
	//qDebug().noquote() << replyText;
	
	
	QJsonDocument document = QJsonDocument::fromJson(replyText);
	if (!document.isObject()){
		qDebug().noquote() << replyText;
		return;
	}

//#ifdef QT_DEBUG
//	do
//	{
//		QFile file(QCoreApplication::instance()->applicationDirPath() + "/forecastRespond.json");
//		if (!file.open(QIODevice::WriteOnly))
//			break;
//		file.write(document.toJson());
//	}
//	while(false);
//#endif
	
	//qDebug().noquote() << document.toJson();
	
	QJsonObject jobject = document.object();
	QJsonValue jv = jobject.value(QStringLiteral("list"));
	if (jv.isArray()){
		QJsonArray jarray = jv.toArray();
				
		//qDebug().noquote() << "forecasts count : " << jarray.size();
		
		for ( int i=0;i<jarray.size() && i<forecastItemsCount();++i ){
			QJsonValue jlistValue = jarray.at(i);
			if (!jlistValue.isObject())
				continue;
			QJsonObject jlistObj = jlistValue.toObject();
			WeatherData* weatherData = m_forecastWeather.at(i);
			
			parseWeatherJson(jlistObj, weatherData);
		}
	}
	
	setHasValidForecast(true);
}
