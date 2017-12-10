import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import WeatherStorage 1.0
import QtCharts 2.1
import QtGraphicalEffects 1.0

ApplicationWindow {
	id: applicationWindow
    visible: true
    width: 800
    height: 480
    title: qsTr("Weather Forecast")
    
	Component.onCompleted: {
		//WeatherStorage.cityName = "Perm"
	}
	
    property int currentCityId
	property bool cityNameChangedNotByUser : false	

	property int defaultFontPointSize: 15
	
	function toMsecsSinceEpoch(date) {
		var msecs = date.getTime();
		return msecs;
	}
	
	function fillChart(){
		
		temperatureSpline.clear()
		
		if (WeatherStorage.hasValidForecast){
			xAxis.min = WeatherStorage.forecastWeather[0].dateTime
			xAxis.max = WeatherStorage.forecastWeather[WeatherStorage.forecastItemsCount()-1].dateTime
			
			yAxis.min = WeatherStorage.temperatureAxisMin()
			yAxis.max = WeatherStorage.temperatureAxisMax()
			
			yAxis.tickCount = yAxis.max - yAxis.min + 1
			for (var i=0;i<WeatherStorage.forecastItemsCount();++i){
				var forecast = WeatherStorage.forecastWeather[i]
				temperatureSpline.append(forecast.dateTime,forecast.temperature)
			}
		}
	}
	
	LinearGradient{
		anchors.fill: parent
		start: Qt.point(0,0)
		end: Qt.point(0, parent.height)
		gradient: Gradient{
			GradientStop{position: 0.0; color : "#FFFFff"}
			GradientStop{position: 1.0; color : "#ccccff"}
		}
	}
	
	
	Connections{
		target : WeatherStorage
		onHasValidForecastChanged:{
			fillChart()
		}
	}
	
	SearchCity{
		id: searchCity
		
		x: (parent.width - searchCity.width)/2
		y: (parent.height - searchCity.height)/4
	}
	
	ColumnLayout{
		id: leftLayout
		anchors.left: parent.left		
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.margins: 10
		width: 200	
		
		BorderRectangle{
			border.color: "lightGrey"
		}
		
		RowLayout{
			id: cityNameLayout
			anchors.top: parent.top
			anchors.left: parent.left
			anchors.right: parent.right
						
			Text{
				id: cityNameText				
				anchors.verticalCenter: parent.verticalCenter
				anchors.left: parent.left
				anchors.right: changeCityButton.left
				font.pointSize: defaultFontPointSize
				text: WeatherStorage.cityNameToDisplay
				
				BorderRectangle{
					border.color: "lightGrey"
				}
			}
			
			ToolButton{
				id: changeCityButton
				anchors.top: parent.top
				anchors.right: parent.right
				height: cityNameText.height
				width: height
				
				Image{
					source: "qrc:/system-search.png"
					anchors.fill: parent
				}
				onClicked: {
					searchCity.setCurrentCityId()
					searchCity.open()
				}
				
				BorderRectangle{}
			}
			BorderRectangle{}
		}
		
		Text{
			id: cityCoordinates
			anchors.top: cityNameLayout.bottom
			anchors.left: parent.left
			visible: WeatherStorage.coordinates.x != 0.0 && WeatherStorage.coordinates.y != 0.0
			text: qsTr("Geo coords: [") + WeatherStorage.coordinates.y.toFixed(2) + ", " + WeatherStorage.coordinates.x.toFixed(2) + "]"
		}
	
		RowLayout{			
			id: currentWeatherIconTempLayout
			anchors.top: cityCoordinates.bottom
			anchors.left: parent.left
			anchors.right: parent.right
			visible: WeatherStorage.hasValidWeather
			
			Image{
				id: weatherImage
				source: WeatherStorage.currentWeather.icon
				height: 50
				width: 50
				anchors.left: parent.left
				
				BorderRectangle{}
			}
			
			Text{
				id: weatherTemperature
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.horizontalCenterOffset: weatherImage.width/2
				anchors.verticalCenter: parent.verticalCenter
				text: WeatherStorage.currentWeather.temperatureText
				font.pointSize: defaultFontPointSize
				
				BorderRectangle{}
			}
			
			
			BorderRectangle{}
		}

		Text{
			id : currentWeatherDescriptionText
			anchors.top: currentWeatherIconTempLayout.bottom
			anchors.left: parent.left
			font.pointSize: defaultFontPointSize
			
			visible: WeatherStorage.hasValidWeather
			text: WeatherStorage.currentWeather.description
			
			BorderRectangle{}
		}
		
		Text{
			id : getWeatherTimeText
			anchors.top: currentWeatherDescriptionText.bottom
			anchors.left: parent.left
			anchors.topMargin: 5 
			visible: WeatherStorage.hasValidWeather
			
			text: WeatherStorage.getWeatherTime
			
			BorderRectangle{}
		}
		
		ToolButton{
			id: refreshButton
			text: qsTr("Refresh")
			anchors.bottom: parent.bottom
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.margins: 5
			background: Rectangle {
					   color: "transparent"
				   }			
			
			BorderRectangle{
				border.color: "black"
			}
			
			onClicked: {
				WeatherStorage.refreshWeatherAndForecast();
			}
		}
		
		BorderRectangle{}
	}
	
	Item{
		id : forecastTmpItem
		anchors.left: leftLayout.right
		anchors.top: parent.top		
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		anchors.margins: 10
		
		
		BorderRectangle{
			border.color: "lightGrey"
		}
		
		ChartView{
			id: chart
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.top: parent.top
			anchors.bottom: forecastRowLayout.top			
			backgroundColor: "transparent"
					
			SplineSeries{
				id: temperatureSpline					
				name: "Temperature"
				color: "blue"
				
				axisX: DateTimeAxis{
					id: xAxis					
					format: "hh.mm"
					tickCount: WeatherStorage.forecastItemsCount()
					gridVisible: false
				}
				axisY: ValueAxis{
					id: yAxis
					//tickCount: 5
					gridVisible: true
				}
			}
		}
		
		RowLayout{
			id: forecastRowLayout

			anchors.bottom: parent.bottom
			anchors.left: parent.left
			anchors.right: parent.right
			spacing: 0
			
			// unknown bug in Qt5.10 when placing dynamically created items into layout, so this ugly workaround:
			
			ForecastItem{
				weatherData: WeatherStorage.forecastWeather[0]
			}
			ForecastItem{
				weatherData: WeatherStorage.forecastWeather[1]
			}
			ForecastItem{
				weatherData: WeatherStorage.forecastWeather[2]
			}
			ForecastItem{
				weatherData: WeatherStorage.forecastWeather[3]
			}
			ForecastItem{
				weatherData: WeatherStorage.forecastWeather[4]
			}
			
			ForecastItem{
				weatherData: WeatherStorage.forecastWeather[5]
			}
			ForecastItem{
				weatherData: WeatherStorage.forecastWeather[6]
			}
			ForecastItem{
				weatherData: WeatherStorage.forecastWeather[7]
			}
			ForecastItem{
				weatherData: WeatherStorage.forecastWeather[8]
			}
			ForecastItem{
				weatherData: WeatherStorage.forecastWeather[9]
			}
			
			/*
			Component.onCompleted: {
				
				var forecastItemComponent;
				function createForecastItems(){
					forecastItemComponent = Qt.createComponent("ForecastItem.qml")
					if (forecastItemComponent.status == Component.Ready)
						createForecastItemsFinish()
					else
						forecastItemComponent.statusChanged.connect(createForecastItemsFinish);
				}
				
				function createForecastItemsFinish(){
					if (forecastItemComponent.status == Component.Ready){
						for (var i = 0; i < WeatherStorage.forecastItemsCount(); ++i){
							var forecast = forecastItemComponent.createObject(forecastRowLayout,{"weatherData": WeatherStorage.forecastWeather[i]})
							if (forecast == null)
								console.log("can`t create forecast item!")
//							else
//								console.log("forecast item created")
						}
					}else if (component.status == Component.Error){
						console.log("Error loading component:", forecastItemComponent.errorString());
					}
				}
				createForecastItems()
			}
			*/
	
			BorderRectangle{
				border.color: "lightGrey"
			}
			
		}
	}
}
