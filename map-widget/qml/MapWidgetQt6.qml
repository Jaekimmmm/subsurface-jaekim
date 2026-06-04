// SPDX-License-Identifier: GPL-2.0
//
// MapWidget implementation for Qt 6 (also works with Qt 5.15+)
// This file is copied to MapWidget.qml during CMake configuration when building with Qt 6
// Qt 6 requires explicit Input Handlers (WheelHandler, DragHandler, PinchHandler, TapHandler)
// for mouse/touch interaction as MapGestureArea was removed
//
import QtQuick 2.15
import QtLocation 5.3
import QtPositioning 5.3
import org.subsurfacedivelog.mobile 1.0

Item {
	id: rootItem
	property alias mapHelper: mapHelper
	property alias map: map

	signal selectedDivesChanged(var list)

	MapWidgetHelper {
		id: mapHelper
		map: map
		editMode: false
		onSelectedDivesChanged: rootItem.selectedDivesChanged(list)
		onEditModeChanged: editMessage.isVisible = editMode === true ? 1 : 0
		onCoordinatesChanged: {}
		Component.onCompleted: {
			map.plugin = Qt.createQmlObject(pluginObject, rootItem)
			map.mapType = { "STREET": map.supportedMapTypes[0], "SATELLITE": map.supportedMapTypes[1] }
			map.activeMapType = map.mapType.SATELLITE
		}
	}

	Map {
		id: map
		anchors.fill: parent
		// AI-generated (Claude): initial view — Jeju area, zoom 8
		center: QtPositioning.coordinate(33.35607909161026, 126.53013192706959)
		zoomLevel: 8.0

		property var startCentroid

		property var mapType
		// AI-generated (Claude): default startup target (Jeju)
		readonly property var defaultCenter: QtPositioning.coordinate(33.35607909161026, 126.53013192706959)
		readonly property real defaultZoomIn: 18.0
		// AI-generated (Claude): first-time single-site landing zoom
		readonly property real singleSiteZoom: 14.0
		readonly property real defaultZoomOut: 1.0
		readonly property real textVisibleZoom: 11.0
		readonly property real zoomStep: 2.0
		property var newCenter: defaultCenter
		property real newZoom: 1.0
		property real newZoomOut: 1.0
		property var clickCoord: QtPositioning.coordinate(0, 0)
		property bool isReady: false
		// AI-generated (Claude): debug tracking — who set the current zoom
		property string zoomSource: "init: Jeju (33.356, 126.530), zoom=8"
		// AI-generated (Claude): padding ratio used by centerOnRectangle (fit-all)
		// 0.25 = 25% empty margin between the outermost marker and the viewport edge
		readonly property real fitPaddingRatio: 0.25

		Component.onCompleted: isReady = true
		onZoomLevelChanged: {
			if (isReady)
				mapHelper.calculateSmallCircleRadius(map.center)
		}

		// AI-generated (Claude): dashed GPS1->GPS2 segments via many short
		// MapPolylines (Qt's MapPolyline has no native dash pattern).
		MapItemView {
			id: trackDashesView
			model: mapHelper.diveTrackDashes
			delegate: MapPolyline {
				line.width: 3
				line.color: "#ff3030"
				path: model.pathCoords.path
			}
		}

		// AI-generated (Claude): GPS2 (exit) markers with checkered-flag icon
		// to visually distinguish from the entry pin.
		MapItemView {
			id: trackExitView
			model: mapHelper.diveTrackExits
			delegate: MapQuickItem {
				coordinate: model.coord
				anchorPoint.x: exitImg.width * 0.5
				anchorPoint.y: exitImg.height
				z: 2
				sourceItem: Image {
					id: exitImg
					source: "qrc:///dive-location-marker-exit-icon"
				}
			}
		}

		MapItemView {
			id: mapItemView
			model: mapHelper.model
			delegate: MapQuickItem {
				id: mapItem
				anchorPoint.x: 0
				anchorPoint.y: mapItemImage.height
				coordinate:  model.coordinate
				z: model.z
				sourceItem: Image {
					id: mapItemImage
					source: model.pixmap
					SequentialAnimation {
						id: mapItemImageAnimation
						PropertyAnimation { target: mapItemImage; property: "scale"; from: 1.0; to: 0.7; duration: 120 }
						PropertyAnimation { target: mapItemImage; property: "scale"; from: 0.7; to: 1.0; duration: 80 }
					}
					MouseArea {
						drag.target: (mapHelper.editMode && model.isSelected) ? mapItem : undefined
						anchors.fill: parent
						// AI-generated (Claude): allow clicks even in edit
						// (dive-site) mode so map markers can drive the dive-
						// site list selection.
						onClicked: {
							if (model.divesite)
								mapHelper.selectedLocationChanged(model.divesite)
						}
						onDoubleClicked: map.doubleClickHandler(mapItem.coordinate)
						onReleased: {
							if (mapHelper.editMode && model.isSelected) {
								mapHelper.updateCurrentDiveSiteCoordinatesFromMap(model.divesite, mapItem.coordinate)
							}
						}
					}
					Item {
						// Text with a duplicate for shadow. DropShadow as layer effect is kind of slow here.
						y: mapItemImage.y + mapItemImage.height
						visible: map.zoomLevel >= map.textVisibleZoom
						Text {
							id: mapItemTextShadow
							x: mapItemText.x + 2; y: mapItemText.y + 2
							text: mapItemText.text
							font.pointSize: mapItemText.font.pointSize
							color: "black"
						}
						Text {
							id: mapItemText
							text: model.name
							font.pointSize: 11.0
							color: model.isSelected ? "white" : "lightgrey"
						}
					}
				}
			}
		}

		SequentialAnimation {
			id: mapAnimationZoomIn
			NumberAnimation {
				target: map; property: "zoomLevel"; to: map.newZoomOut; duration: Math.abs(map.newZoomOut - map.zoomLevel) * 200
			}
			ParallelAnimation {
				CoordinateAnimation { target: map; property: "center"; to: map.newCenter; duration: 2000; easing.type: Easing.OutCubic }
				NumberAnimation {
					target: map; property: "zoomLevel"; to: map.newZoom; duration: 2000
				}
			}
		}

		ParallelAnimation {
			id: mapAnimationClick
			CoordinateAnimation { target: map; property: "center"; to: map.newCenter; duration: 500	}
			NumberAnimation { target: map; property: "zoomLevel"; to: map.newZoom; duration: 500 }
		}

		// Qt 6 uses Input Handlers for map interaction (required for mouse/touch to work)
		// these also work with Qt 5.15
		WheelHandler {
			id: wheel
			onActiveChanged: if (active) { map.stopZoomAnimations(); map.zoomSource = "wheel (rotationScale=1/120)" }
			acceptedDevices: Qt.platform.pluginName === "cocoa" || Qt.platform.pluginName === "wayland"
				? PointerDevice.Mouse | PointerDevice.TouchPad
				: PointerDevice.Mouse
			rotationScale: 1/120 // this gets us reasonably smooth zooming in my testing
			property: "zoomLevel"
		}
		DragHandler {
			id: drag
			target: null
			onActiveChanged: if (active) map.stopZoomAnimations()
			onTranslationChanged: function(delta) { map.pan(-delta.x, -delta.y) }
		}
		PinchHandler {
			id: pinch
			target: null
			onActiveChanged: if (active) {
				map.stopZoomAnimations()
				map.startCentroid = map.toCoordinate(pinch.centroid.position, false)
				map.zoomSource = "pinch (log2(scale))"
			}
			onScaleChanged: function(delta) {
				map.zoomLevel += Math.log2(delta)
				map.alignCoordinateToPoint(map.startCentroid, pinch.centroid.position)
			}
			grabPermissions: PointerHandler.TakeOverForbidden
		}
		TapHandler {
			acceptedButtons: Qt.LeftButton
			onDoubleTapped: function(eventPoint, button) {
				map.doubleClickHandler(map.toCoordinate(eventPoint.position))
			}
		}

		function doubleClickHandler(coord) {
			newCenter = coord
			newZoom = zoomLevel + zoomStep
			if (newZoom > maximumZoomLevel)
				newZoom = maximumZoomLevel
			zoomSource = "doubleClick: zoomLevel+zoomStep(" + zoomStep + ") clamp<=maximumZoomLevel(" + maximumZoomLevel + ")"
			mapAnimationClick.restart()
		}

		function pointIsVisible(pt) {
			return !isNaN(pt.x)
		}

		function coordIsValid(coord) {
			if (coord == null || isNaN(coord.latitude) || isNaN(coord.longitude) ||
			    (coord.latitude === 0.0 && coord.longitude === 0.0))
				return false;
			return true;
		}

		function stopZoomAnimations() {
			mapAnimationZoomIn.stop()
		}

		// AI-generated (Claude): single-site centring.
		// preserveZoom=true → just pan, keep current zoom (transition between
		// two single-site selections). Otherwise zoom to singleSiteZoom (14).
		function centerOnCoordinate(coord, preserveZoom) {
			stopZoomAnimations()
			if (!coordIsValid(coord)) {
				console.warn("MapWidget.qml: centerOnCoordinate(): !coordIsValid()")
				return
			}
			var zoomStored = zoomLevel
			newCenter = coord
			if (preserveZoom === true) {
				newZoom = zoomStored
				zoomSource = "centerOnCoordinate (single→single): zoom kept @ " +
				             zoomStored.toFixed(2)
			} else {
				newZoom = singleSiteZoom
				zoomSource = "centerOnCoordinate (single site, first): newZoom=singleSiteZoom(" +
				             singleSiteZoom.toFixed(2) + ")"
			}
			newZoomOut = zoomStored
			mapAnimationZoomIn.restart()
		}

		// AI-generated (Claude): pick the zoom level so the limiting axis (width
		// or height, whichever is tighter relative to its bbox extent) hits the
		// requested padding ratio. Comparing only diagonals would crop markers
		// on the perpendicular axis when bbox aspect differs from the viewport.
		function centerOnRectangle(topLeft, bottomRight, centerRect) {
			stopZoomAnimations()
			if (newCenter.latitude === 0.0 && newCenter.longitude === 0.0) {
				return
			}
			var centerStored = QtPositioning.coordinate(center.latitude, center.longitude)
			var zoomStored = zoomLevel
			newCenter = centerRect

			// Viewport width/height in metres at the current zoom.
			var midY = height * 0.5, midX = width * 0.5
			var viewportW = toCoordinate(Qt.point(0.0, midY)).distanceTo(toCoordinate(Qt.point(width, midY)))
			var viewportH = toCoordinate(Qt.point(midX, 0.0)).distanceTo(toCoordinate(Qt.point(midX, height)))

			// Bbox width/height in metres along its midline.
			var midLat = (topLeft.latitude + bottomRight.latitude) * 0.5
			var midLon = (topLeft.longitude + bottomRight.longitude) * 0.5
			var rectW = QtPositioning.coordinate(midLat, topLeft.longitude)
			              .distanceTo(QtPositioning.coordinate(midLat, bottomRight.longitude))
			var rectH = QtPositioning.coordinate(topLeft.latitude, midLon)
			              .distanceTo(QtPositioning.coordinate(bottomRight.latitude, midLon))

			if (viewportW > 0 && viewportH > 0 && rectW > 0 && rectH > 0) {
				// At target zoom, we want both:
				//   viewportW(target) >= rectW * (1 + 2*pad)
				//   viewportH(target) >= rectH * (1 + 2*pad)
				// Each axis halves per +1 zoom. The smaller of the two ratios
				// drives the final zoom (otherwise the other axis is cropped).
				var pad = 1.0 + 2.0 * fitPaddingRatio
				var ratioX = viewportW / (rectW * pad)
				var ratioY = viewportH / (rectH * pad)
				var limit = Math.min(ratioX, ratioY)
				newZoom = zoomStored + Math.log2(limit)
			} else {
				newZoom = defaultZoomIn
			}
			if (newZoom > defaultZoomIn) newZoom = defaultZoomIn
			if (newZoom < minimumZoomLevel) newZoom = minimumZoomLevel

			// Skip the intermediate zoom-out step in the animation: zoom and pan
			// together straight to the target.
			newZoomOut = zoomStored

			center = centerStored
			zoomSource = "centerOnRectangle: rectW=" + Math.round(rectW) +
			             "m rectH=" + Math.round(rectH) +
			             "m -> newZoom=" + newZoom.toFixed(2) +
			             " (padRatio=" + fitPaddingRatio + ", clamp[" +
			             minimumZoomLevel.toFixed(1) + "," + defaultZoomIn + "])"
			mapAnimationZoomIn.restart()
		}

		function deselectMapLocation() {
			stopZoomAnimations()
		}
	}

	Rectangle {
		id: editMessage
		radius: padding
		color: "#b08000"
		border.color: "white"
		x: (map.width - width) * 0.5; y: padding
		width: editMessageText.width + padding * 2.0
		height: editMessageText.height + padding * 2.0
		visible: false
		opacity: 0.0
		property int isVisible: -1
		property real padding: 10.0
		onOpacityChanged: visible = opacity != 0.0
		states: [
			State { when: editMessage.isVisible === 1; PropertyChanges { target: editMessage; opacity: 1.0 }},
			State { when: editMessage.isVisible === 0; PropertyChanges { target: editMessage; opacity: 0.0 }}
		]
		transitions: Transition { NumberAnimation { properties: "opacity"; easing.type: Easing.InOutQuad }}
		Text {
			id: editMessageText
			y: editMessage.padding; x: editMessage.padding
			verticalAlignment: Text.AlignVCenter
			color: "white"
			font.pointSize: 11.0
			text: qsTr("Drag the selected dive location")
		}
	}

	// AI-generated (Claude): debug overlay showing current zoom and who set it
	Rectangle {
		id: zoomDebugBox
		x: 60; y: 10
		width: zoomDebugText.width + 16
		height: zoomDebugText.height + 10
		color: "#cc000000"
		radius: 4
		Text {
			id: zoomDebugText
			x: 8; y: 5
			color: "white"
			font.pointSize: 10
			text: "zoom = " + map.zoomLevel.toFixed(3) +
			      "  [min " + map.minimumZoomLevel.toFixed(1) +
			      ", max " + map.maximumZoomLevel.toFixed(1) + "]" +
			      "\nsource: " + map.zoomSource
		}
	}

	Image {
		id: toggleImage
		x: 10; y: x
		width: 40
		height: 40
		source: "qrc:///map-style-" + (map.activeMapType === map.mapType.SATELLITE ? "map" : "photo") + "-icon"
		SequentialAnimation {
			id: toggleImageAnimation
			PropertyAnimation { target: toggleImage; property: "scale"; from: 1.0; to: 0.8; duration: 120 }
			PropertyAnimation { target: toggleImage; property: "scale"; from: 0.8; to: 1.0; duration: 80 }
		}
		MouseArea {
			anchors.fill: parent
			onClicked: {
				map.activeMapType = map.activeMapType === map.mapType.SATELLITE ? map.mapType.STREET : map.mapType.SATELLITE
				toggleImageAnimation.restart()
			}
		}
	}

	Image {
		id: imageZoomIn
		x: 10 + (toggleImage.width - imageZoomIn.width) * 0.5; y: toggleImage.y + toggleImage.height + 10
		width: 20
		height: 20
		source: "qrc:///zoom-in-icon"
		SequentialAnimation {
			id: imageZoomInAnimation
			PropertyAnimation { target: imageZoomIn; property: "scale"; from: 1.0; to: 0.8; duration: 120 }
			PropertyAnimation { target: imageZoomIn; property: "scale"; from: 0.8; to: 1.0; duration: 80 }
		}
		MouseArea {
			anchors.fill: parent
			onClicked: {
				map.stopZoomAnimations()
				map.newCenter = map.center
				map.newZoom = map.zoomLevel + map.zoomStep
				if (map.newZoom > map.maximumZoomLevel)
					map.newZoom = map.maximumZoomLevel
				map.zoomSource = "[+] button: zoomLevel+zoomStep(" + map.zoomStep +
				                 ") clamp<=maximumZoomLevel(" + map.maximumZoomLevel + ")"
				mapAnimationClick.restart()
				imageZoomInAnimation.restart()
			}
		}
	}

	Image {
		id: imageZoomOut
		x: imageZoomIn.x; y: imageZoomIn.y + imageZoomIn.height + 10
		source: "qrc:///zoom-out-icon"
		width: 20
		height: 20
		SequentialAnimation {
			id: imageZoomOutAnimation
			PropertyAnimation { target: imageZoomOut; property: "scale"; from: 1.0; to: 0.8; duration: 120 }
			PropertyAnimation { target: imageZoomOut; property: "scale"; from: 0.8; to: 1.0; duration: 80 }
		}
		MouseArea {
			anchors.fill: parent
			onClicked: {
				map.stopZoomAnimations()
				map.newCenter = map.center
				map.newZoom = map.zoomLevel - map.zoomStep
				map.zoomSource = "[-] button: zoomLevel-zoomStep(" + map.zoomStep + ")"
				mapAnimationClick.restart()
				imageZoomOutAnimation.restart()
			}
		}
	}

	/*
	 * open coordinates in google maps while attempting to roughly preserve
	 * the zoom level. the mapping between the QML map zoom level and the
	 * Google Maps zoom level is done via exponential regression:
	 *     y = a * exp(b * x)
	 *
	 * data set:
	 *     qml (x)            gmaps (y in meters)
	 *     21                 257
	 *     15.313216749178913 3260
	 *     12.553216749178931 20436
	 *     11.11321674917894  52883
	 *     9.313216749178952  202114
	 *     7.51321674917896   737136
	 *     5.593216749178958  2495529
	 *     4.153216749178957  3895765
	 *     1.753216749178955  18999949
	 */
	function openLocationInGoogleMaps(latitude, longitude) {
		var loc = latitude + "," + longitude
		var poi = latitude + "+" + longitude
		var x = map.zoomLevel
		var a = 53864950.831693
		var b = -0.60455861606547030630
		var zoom = Math.floor(a * Math.exp(b * x))
		var url = "https://www.google.com/maps/place/" + poi + "/@" + loc + "," + zoom + "m/data=!3m1!1e3!4m2!3m1!1s0x0:0x0"
		Qt.openUrlExternally(url)
		console.log("openLocationInGoogleMaps() map.zoomLevel: " + x + ", url: " + url)
	}

	MapWidgetContextMenu {
		id: contextMenu
		y: 10; x: map.width - y
		onActionSelected: {
			switch (action) {
			case contextMenu.actions.OPEN_LOCATION_IN_GOOGLE_MAPS:
				openLocationInGoogleMaps(map.center.latitude, map.center.longitude)
				break
			case contextMenu.actions.COPY_LOCATION_DECIMAL:
				mapHelper.copyToClipboardCoordinates(map.center, false)
				break
			case contextMenu.actions.COPY_LOCATION_SEXAGESIMAL:
				mapHelper.copyToClipboardCoordinates(map.center, true)
				break
			case contextMenu.actions.SELECT_VISIBLE_LOCATIONS:
				mapHelper.selectVisibleLocations()
				break
			}
		}
	}
}
