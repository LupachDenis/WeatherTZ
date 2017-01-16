import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import WeatherStorage 1.0

// test ApplicationWindow

ApplicationWindow {
	
	visible: true
    width: 640
    height: 480
    title: qsTr("Weather Forecast")
    
    property int currentCityId

	
	Component.onCompleted:{
		citySuggestionListModel.clear()
		var variants = CitiesStorage.suggestion1("text")
		for (var cityVariant in variants){
			citySuggestionListModel.append({cityName:variants[cityVariant]} )
		}
	}
	
	ColumnLayout{
		anchors.fill: parent
		
		
		ListView{
			id : citySuggestionListView
			anchors.fill: parent						
			model: ListModel{
				id : citySuggestionListModel
			}
			
			readonly property int listViewItemHeight : 40
			readonly property int listViewItemWidth : 180
			readonly property int listViewItemX : 10
			
			Layout.fillHeight: true
			Layout.fillWidth: true
			
			height: 200//listViewItemHeight * citySuggestionListModel.count
			width: listViewItemWidth + listViewItemX*2
			
			delegate: Component{
				id: listViewItemDelegate
				Rectangle{
					border.width: 1
					width: citySuggestionListView.listViewItemWidth
					height: citySuggestionListView.listViewItemHeight
					Text {
						text: cityName
						anchors.verticalCenter: parent.verticalCenter
						x: 10
					}
				}
			}
		}
	}
}
