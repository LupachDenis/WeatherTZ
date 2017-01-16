import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import WeatherStorage 1.0

Popup {
	id: searchCity
	modal: true
	focus: true
	closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
	
	property int selectedCityId : 0
	
	function setCurrentCityId(){
		var currentCityName = WeatherStorage.cityNameToDisplay;
		if (currentCityName == "???")
			cityNameField.text = ""
		else
			cityNameField.text = currentCityName
	}
	
	onClosed: {		
		cityNamePopup.close()
	}
	
	Connections{
		target : WeatherStorage
		onSuggestionChanged: {
			citySuggestionListModel.clear()
			var variants = WeatherStorage.suggestion
			for (var i=0;i<variants.length;++i){
				var variant_ = variants[i];
				citySuggestionListModel.append({cityId:variant_.id, cityName:variant_.name, cityCountry:variant_.country} )
			}
			cityNamePopup.open()
		}
	}
	
	function openPopup(){
		WeatherStorage.suggestionMask = cityNameField.text
		cityNamePopup.open()
	}
	
	function closePopup(){
		if (selectedCityId == 0)
			WeatherStorage.cityName = cityNameField.text
		else
			WeatherStorage.cityId = selectedCityId
		
		searchCity.close()
	}
	
	ColumnLayout{
		
		// ComboxBox would be better, but: https://bugreports.qt.io/browse/QTBUG-53876
		TextField{
			id: cityNameField
			selectByMouse: true
			
			onTextChanged: {
				selectedCityId = 0
				openPopup()
			}
		}		
		
		Popup{
			id : cityNamePopup
			y: cityNameField.height + cityNameField.y
			x: cityNameField.x
			width: cityNameField.width
			height: citySuggestionListView.listViewItemHeight * citySuggestionListModel.count
			padding: 0
			closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
			
			ListView{
				id : citySuggestionListView
				anchors.fill: parent
				anchors.margins: 0						
				model: ListModel{
					id : citySuggestionListModel
				}
				
				readonly property int listViewItemHeight : 40
				readonly property int listViewItemX : 10
				
				Layout.fillHeight: true
				Layout.fillWidth: true
				
				height: parent.height
				width: cityNameField.width
				
				delegate: Component{
					id: listViewItemDelegate
					Rectangle{
						border.width: 1
						width: cityNameField.width
						height: citySuggestionListView.listViewItemHeight
						Text {
							text: cityName + "," + cityCountry
							anchors.verticalCenter: parent.verticalCenter
							x: 10
						}
						MouseArea{
							anchors.fill: parent
							onClicked: {
								var selectedCityId_ = cityId
								var selectedCityName_ = cityName + "," + cityCountry
								cityNameField.text = selectedCityName_
								selectedCityId = selectedCityId_
								cityNamePopup.close()
								closePopup()
							}
						}
					}
				}
			}
		}
		
		RowLayout{
			anchors.horizontalCenter: parent.horizontalCenter
			Button{
				text: qsTr("OK")
				anchors.horizontalCenter: parent.horizontalCenter
				onClicked: {
					closePopup()
				}
			}
		}
	}
	
}
