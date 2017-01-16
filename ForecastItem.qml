import QtQuick 2.0
import QtQuick.Layouts 1.3
import WeatherStorage 1.0
import QtQuick.Controls 2.0

ColumnLayout{
	id: columnLayout
	
	property int forecastFontPointSize: 10	
	property WeatherData weatherData: undefined
	
	Image{
		height: 50
		width: 50
		source: weatherData.icon
	}
	Text{
		anchors.horizontalCenter: parent.horizontalCenter
		text: WeatherStorage.hasValidForecast ? weatherData.temperatureText : ""
		font.pointSize: forecastFontPointSize
	}
	Text{
		anchors.horizontalCenter: parent.horizontalCenter
		text: WeatherStorage.hasValidForecast ? weatherData.time : ""		
	}

}
	

