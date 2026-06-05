// SPDX-License-Identifier: GPL-2.0
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QVector>

#include "qmlmapwidgethelper.h"
#include "core/dive.h"
#include "core/divecomputer.h"
#include "core/extradata.h"
#include "core/divefilter.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/pref.h"
#include "core/qthelper.h"
#include "core/range.h"
#include "core/string-format.h"
#include "qt-models/maplocationmodel.h"
#include "qt-models/divelocationmodel.h"
#ifndef SUBSURFACE_MOBILE
#include "desktop-widgets/mapwidget.h"
#endif

#define SMALL_CIRCLE_RADIUS_PX            26.0

MapWidgetHelper::MapWidgetHelper(QObject *parent) : QObject(parent)
{
	m_mapLocationModel = new MapLocationModel(this);
	m_smallCircleRadius = SMALL_CIRCLE_RADIUS_PX;
	m_map = nullptr;
	m_editMode = false;
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &MapWidgetHelper::diveSiteChanged);
}

QGeoCoordinate MapWidgetHelper::getCoordinates(struct dive_site *ds)
{
	if (!ds || !ds->has_gps_location())
		return QGeoCoordinate(0.0, 0.0);
	return QGeoCoordinate(ds->location.lat.udeg * 0.000001, ds->location.lon.udeg * 0.000001);
}

void MapWidgetHelper::centerOnDiveSite(struct dive_site *ds)
{
	if (!ds || !ds->has_gps_location()) {
		// dive site with no GPS
		m_mapLocationModel->setSelected(ds);
		QMetaObject::invokeMethod(m_map, "deselectMapLocation");
	} else {
		// dive site with GPS
		m_mapLocationModel->setSelected(ds);
		QGeoCoordinate dsCoord (ds->location.lat.udeg * 0.000001, ds->location.lon.udeg * 0.000001);
		QMetaObject::invokeMethod(m_map, "centerOnCoordinate", Q_ARG(QVariant, QVariant::fromValue(dsCoord)));
	}
}

void MapWidgetHelper::setSelected(const std::vector<dive_site *> divesites)
{
	m_mapLocationModel->setSelected(std::move(divesites));
	m_mapLocationModel->selectionChanged();
	updateEditMode();
	// AI-generated (Claude)
	updateDiveTracks();
}

// AI-generated (Claude): collect entry (GPS1) / exit (GPS2) coordinates from
// the dive computers of the currently selected dives, and synthesize a fake
// dashed line by chopping each segment into N short polylines (Qt's
// MapPolyline does not support a native dash pattern).
void MapWidgetHelper::updateDiveTracks()
{
	QVector<QGeoCoordinate> exits;
	QVector<QVector<QGeoCoordinate>> dashes;
	constexpr int DASH_COUNT = 10; // 10 dashes + 9 gaps; pattern starts and ends with a dash

	for (const auto &d: divelog.dives) {
		if (!d->selected)
			continue;
		location_t entry{}, exitLoc{};
		bool hasEntry = false, hasExit = false;
		for (const struct divecomputer &dc: d->dcs) {
			for (const auto &data: dc.extra_data) {
				if (data.key == "GPS1" && !hasEntry) {
					location_t tmp{};
					parse_location(data.value.c_str(), &tmp);
					if (has_location(&tmp)) { entry = tmp; hasEntry = true; }
				} else if (data.key == "GPS2" && !hasExit) {
					location_t tmp{};
					parse_location(data.value.c_str(), &tmp);
					if (has_location(&tmp)) { exitLoc = tmp; hasExit = true; }
				}
			}
			if (hasEntry && hasExit)
				break;
		}
		if (!hasEntry || !hasExit)
			continue;
		if (entry == exitLoc)
			continue;

		const double eLat = entry.lat.udeg   * 0.000001;
		const double eLon = entry.lon.udeg   * 0.000001;
		const double xLat = exitLoc.lat.udeg * 0.000001;
		const double xLon = exitLoc.lon.udeg * 0.000001;
		exits.append(QGeoCoordinate(xLat, xLon));

		constexpr int slots = 2 * DASH_COUNT - 1; // 19: even=dash, odd=gap; last slot is a dash
		for (int i = 0; i < DASH_COUNT; ++i) {
			const double t0 = static_cast<double>(2 * i)     / slots;
			const double t1 = static_cast<double>(2 * i + 1) / slots;
			QVector<QGeoCoordinate> seg{
				QGeoCoordinate(eLat + (xLat - eLat) * t0, eLon + (xLon - eLon) * t0),
				QGeoCoordinate(eLat + (xLat - eLat) * t1, eLon + (xLon - eLon) * t1)
			};
			dashes.append(std::move(seg));
		}
	}

	m_diveTrackExits.reset(std::move(exits));
	m_diveTrackDashes.reset(std::move(dashes));
}

void MapWidgetHelper::centerOnSelectedDiveSite()
{
	std::vector<struct dive_site *> selDS = m_mapLocationModel->selectedDs();

	if (selDS.empty()) {
		// no selected dives with GPS coordinates
		QMetaObject::invokeMethod(m_map, "deselectMapLocation");
		return;
	}

	// find the most top-left and bottom-right dive sites on the map coordinate system.
	qreal minLat = 0.0, minLon = 0.0, maxLat = 0.0, maxLon = 0.0;
	int count = 0;
	for(struct dive_site *dss: selDS) {
		if (!has_location(&dss->location))
			continue;
		qreal lat = dss->location.lat.udeg * 0.000001;
		qreal lon = dss->location.lon.udeg * 0.000001;
		if (++count == 1) {
			minLat = maxLat = lat;
			minLon = maxLon = lon;
			continue;
		}
		if (lat < minLat)
			minLat = lat;
		else if (lat > maxLat)
			maxLat = lat;
		if (lon < minLon)
			minLon = lon;
		else if (lon > maxLon)
			maxLon = lon;
	}

	// Pass coordinates to QML, either as a point or as a rectangle.
	// If we didn't find any coordinates, do nothing.
	if (count == 1) {
		QGeoCoordinate dsCoord (selDS[0]->location.lat.udeg * 0.000001, selDS[0]->location.lon.udeg * 0.000001);
		// AI-generated (Claude): preserve zoom only when transitioning from
		// another single-site selection.
		const bool preserveZoom = m_prevSingleSite;
		QMetaObject::invokeMethod(m_map, "centerOnCoordinate",
		                          Q_ARG(QVariant, QVariant::fromValue(dsCoord)),
		                          Q_ARG(QVariant, preserveZoom));
		m_prevSingleSite = true;
	} else if (count > 1) {
		QGeoCoordinate coordTopLeft(minLat, minLon);
		QGeoCoordinate coordBottomRight(maxLat, maxLon);
		QGeoCoordinate coordCenter(minLat + (maxLat - minLat) * 0.5, minLon + (maxLon - minLon) * 0.5);
		QMetaObject::invokeMethod(m_map, "centerOnRectangle",
					  Q_ARG(QVariant, QVariant::fromValue(coordTopLeft)),
					  Q_ARG(QVariant, QVariant::fromValue(coordBottomRight)),
					  Q_ARG(QVariant, QVariant::fromValue(coordCenter)));
		m_prevSingleSite = false;
	}
}

void MapWidgetHelper::updateEditMode()
{
#ifndef SUBSURFACE_MOBILE
	// The filter being set to dive site is the signal that we are in dive site edit mode.
	// This is the case when either the dive site edit tab or the dive site list tab are active.
	bool old = m_editMode;
	m_editMode = DiveFilter::instance()->diveSiteMode();
	if (old != m_editMode)
		emit editModeChanged();
#endif
}

void MapWidgetHelper::reloadMapLocations()
{
	updateEditMode();
	m_mapLocationModel->reload(m_map);
	// AI-generated (Claude)
	updateDiveTracks();
}

// AI-generated (Claude): compute the bounding box of every dive site that has
// GPS coordinates and ask QML to fit the rectangle. Falls back to the single-
// site centering for one site, or to the default zoom for none.
void MapWidgetHelper::centerOnAllSites()
{
	qreal minLat = 0.0, minLon = 0.0, maxLat = 0.0, maxLon = 0.0;
	int count = 0;
	for (const auto &ds: divelog.sites) {
		if (!ds->has_gps_location())
			continue;
		const qreal lat = ds->location.lat.udeg * 0.000001;
		const qreal lon = ds->location.lon.udeg * 0.000001;
		if (++count == 1) {
			minLat = maxLat = lat;
			minLon = maxLon = lon;
			continue;
		}
		if      (lat < minLat) minLat = lat;
		else if (lat > maxLat) maxLat = lat;
		if      (lon < minLon) minLon = lon;
		else if (lon > maxLon) maxLon = lon;
	}
	if (count == 0)
		return;
	if (count == 1) {
		QGeoCoordinate c(minLat, minLon);
		QMetaObject::invokeMethod(m_map, "centerOnCoordinate", Q_ARG(QVariant, QVariant::fromValue(c)));
		return;
	}
	QGeoCoordinate topLeft(maxLat, minLon);
	QGeoCoordinate bottomRight(minLat, maxLon);
	QGeoCoordinate center(minLat + (maxLat - minLat) * 0.5, minLon + (maxLon - minLon) * 0.5);
	QMetaObject::invokeMethod(m_map, "centerOnRectangle",
				  Q_ARG(QVariant, QVariant::fromValue(topLeft)),
				  Q_ARG(QVariant, QVariant::fromValue(bottomRight)),
				  Q_ARG(QVariant, QVariant::fromValue(center)));
}

void MapWidgetHelper::selectedLocationChanged(struct dive_site *ds_in)
{
	if (!ds_in)
		return;
	const MapLocation *location = m_mapLocationModel->getMapLocation(ds_in);
	if (!location)
		return;
	QGeoCoordinate locationCoord = location->coordinate;

#ifndef SUBSURFACE_MOBILE
	// AI-generated (Claude): in dive-site mode the relevant selection is
	// dive_site rows in the list view, not dives. Collect nearby sites and
	// emit a dedicated signal.
	if (DiveFilter::instance()->diveSiteMode()) {
		QVector<dive_site *> nearbySites;
		for (const auto &ds: divelog.sites) {
			if (!ds->has_gps_location())
				continue;
			const qreal lat = ds->location.lat.udeg * 0.000001;
			const qreal lon = ds->location.lon.udeg * 0.000001;
			if (locationCoord.distanceTo(QGeoCoordinate(lat, lon)) < m_smallCircleRadius)
				nearbySites.append(ds.get());
		}
		emit selectedDiveSitesFromMap(nearbySites);
		return;
	}
#endif

	QList<int> selectedDiveIds;
	for (auto [idx, dive]: enumerated_range(divelog.dives)) {
		struct dive_site *ds = dive->dive_site;
		if (!ds || !ds->has_gps_location())
			continue;
#ifndef SUBSURFACE_MOBILE
		const qreal latitude = ds->location.lat.udeg * 0.000001;
		const qreal longitude = ds->location.lon.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		if (locationCoord.distanceTo(dsCoord) < m_smallCircleRadius)
			selectedDiveIds.append(idx);
	}
#else // the mobile version doesn't support multi-dive selection
		if (ds == location->divesite)
			selectedDiveIds.append(dive->id); // use id here instead of index
	}
	int last; // get latest dive chronologically
	if (!selectedDiveIds.isEmpty()) {
		 last = selectedDiveIds.last();
		 selectedDiveIds.clear();
		 selectedDiveIds.append(last);
	}
#endif
	emit selectedDivesChanged(selectedDiveIds);
}

void MapWidgetHelper::selectVisibleLocations()
{
	QList<int> selectedDiveIds;
	for (auto [idx, dive]: enumerated_range(divelog.dives)) {
		struct dive_site *ds = dive->dive_site;
		if (!ds || ds->has_gps_location())
			continue;
		const qreal latitude = ds->location.lat.udeg * 0.000001;
		const qreal longitude = ds->location.lon.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		QPointF point;
		QMetaObject::invokeMethod(m_map, "fromCoordinate", Q_RETURN_ARG(QPointF, point),
		                          Q_ARG(QGeoCoordinate, dsCoord));
		if (!qIsNaN(point.x()))
#ifndef SUBSURFACE_MOBILE // indices on desktop
			selectedDiveIds.append(idx);
	}
#else // use id on mobile instead of index
			selectedDiveIds.append(dive->id);
	}
	int last; // get latest dive chronologically
	if (!selectedDiveIds.isEmpty()) {
		 last = selectedDiveIds.last();
		 selectedDiveIds.clear();
		 selectedDiveIds.append(last);
	}
#endif
	emit selectedDivesChanged(selectedDiveIds);
}

/*
 * Based on a 2D Map widget circle with center "coord" and radius SMALL_CIRCLE_RADIUS_PX,
 * obtain a "small circle" with radius m_smallCircleRadius in meters:
 *     https://en.wikipedia.org/wiki/Circle_of_a_sphere
 *
 * The idea behind this circle is to be able to select multiple nearby dives, when clicking on
 * the map. This code can be in QML, but it is in C++ instead for performance reasons.
 *
 * This can be made faster with an exponential regression [a * exp(b * x)], with a pretty
 * decent R-squared, but it becomes bound to map provider zoom level mappings and the
 * SMALL_CIRCLE_RADIUS_PX value, which makes the code hard to maintain.
 */
void MapWidgetHelper::calculateSmallCircleRadius(QGeoCoordinate coord)
{
	QPointF point;
	QMetaObject::invokeMethod(m_map, "fromCoordinate", Q_RETURN_ARG(QPointF, point),
	                          Q_ARG(QGeoCoordinate, coord));
	QPointF point2(point.x() + SMALL_CIRCLE_RADIUS_PX, point.y());
	QGeoCoordinate coord2;
	QMetaObject::invokeMethod(m_map, "toCoordinate", Q_RETURN_ARG(QGeoCoordinate, coord2),
	                          Q_ARG(QPointF, point2));
	m_smallCircleRadius = coord2.distanceTo(coord);
}

static location_t mk_location(QGeoCoordinate coord)
{
	return create_location(coord.latitude(), coord.longitude());
}

void MapWidgetHelper::copyToClipboardCoordinates(QGeoCoordinate coord, bool formatTraditional)
{
	bool savep = prefs.coordinates_traditional;
	prefs.coordinates_traditional = formatTraditional;
	location_t location = mk_location(coord);
	QApplication::clipboard()->setText(printGPSCoords(&location), QClipboard::Clipboard);

	prefs.coordinates_traditional = savep;
}

void MapWidgetHelper::updateCurrentDiveSiteCoordinatesFromMap(struct dive_site *ds, QGeoCoordinate coord)
{
	MapLocation *loc = m_mapLocationModel->getMapLocation(ds);
	if (loc)
		loc->coordinate = coord;
	location_t location = mk_location(coord);
	emit coordinatesChanged(ds, location);
}

void MapWidgetHelper::diveSiteChanged(struct dive_site *ds, int field)
{
	centerOnDiveSite(ds);
}

bool MapWidgetHelper::editMode() const
{
	return m_editMode;
}

QString MapWidgetHelper::pluginObject()
{
	QString lang = getUiLanguage().replace('_', '-');
	QString cacheFolder = QString::fromStdString(system_default_directory() + "/googlemaps").replace("\\", "/");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	const char *locationImport = "import QtLocation 6.0;";
#else
	const char *locationImport = "import QtLocation 5.3;";
#endif
	return QStringLiteral("import QtQuick 2.0;"
			      "%3"
			      "Plugin {"
			      "    id: mapPlugin;"
			      "    name: 'googlemaps';"
			      "    PluginParameter { name: 'googlemaps.maps.language'; value: '%1' }"
			      "    PluginParameter { name: 'googlemaps.cachefolder'; value: '%2' }"
			      "    Component.onCompleted: {"
			      "        if (availableServiceProviders.indexOf(name) === -1) {"
			      "            console.warn('MapWidget.qml: cannot find a plugin named: ' + name);"
			      "        }"
			      "    }"
			      "}").arg(lang, cacheFolder, QLatin1String(locationImport));
}
