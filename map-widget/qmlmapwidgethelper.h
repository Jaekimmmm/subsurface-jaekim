// SPDX-License-Identifier: GPL-2.0
#ifndef QMLMAPWIDGETHELPER_H
#define QMLMAPWIDGETHELPER_H

#include "core/units.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include <QObject>
#include <QGeoCoordinate>

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QGeoServiceProviderFactoryGooglemaps)
#endif

#include "qt-models/maplocationmodel.h"
#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QGeoPath>
#include <QVector>
#include <QList>
class MapLocation;
struct dive_site;

// AI-generated (Claude): simple list models for the GPS2 markers and the
// GPS1->GPS2 dashed-segment polylines. MapItemView requires a real model
// (QAbstractListModel) in Qt6 for delegate bindings to refresh reliably.
class DiveTrackExitModel : public QAbstractListModel {
	Q_OBJECT
public:
	enum Roles { CoordRole = Qt::UserRole + 1 };
	int rowCount(const QModelIndex & = QModelIndex()) const override { return m_coords.size(); }
	QVariant data(const QModelIndex &idx, int role) const override
	{
		if (idx.row() < 0 || idx.row() >= m_coords.size() || role != CoordRole)
			return {};
		return QVariant::fromValue(m_coords[idx.row()]);
	}
	QHash<int, QByteArray> roleNames() const override
	{
		return { { CoordRole, "coord" } };
	}
	void reset(QVector<QGeoCoordinate> coords)
	{
		beginResetModel();
		m_coords = std::move(coords);
		endResetModel();
	}
private:
	QVector<QGeoCoordinate> m_coords;
};

class DiveTrackDashModel : public QAbstractListModel {
	Q_OBJECT
public:
	enum Roles { PathRole = Qt::UserRole + 1 };
	int rowCount(const QModelIndex & = QModelIndex()) const override { return m_paths.size(); }
	QVariant data(const QModelIndex &idx, int role) const override
	{
		if (idx.row() < 0 || idx.row() >= m_paths.size() || role != PathRole)
			return {};
		// MapPolyline.path expects a QGeoPath; auto-convert from QList<QGeoCoordinate>.
		QList<QGeoCoordinate> coords(m_paths[idx.row()].begin(), m_paths[idx.row()].end());
		return QVariant::fromValue(QGeoPath(coords));
	}
	QHash<int, QByteArray> roleNames() const override
	{
		return { { PathRole, "pathCoords" } };
	}
	void reset(QVector<QVector<QGeoCoordinate>> paths)
	{
		beginResetModel();
		m_paths = std::move(paths);
		endResetModel();
	}
private:
	QVector<QVector<QGeoCoordinate>> m_paths;
};

class MapWidgetHelper : public QObject {

	Q_OBJECT
	Q_PROPERTY(QObject *map MEMBER m_map)
	Q_PROPERTY(MapLocationModel *model MEMBER m_mapLocationModel NOTIFY modelChanged)
	Q_PROPERTY(bool editMode MEMBER m_editMode NOTIFY editModeChanged)
	Q_PROPERTY(QString pluginObject READ pluginObject NOTIFY pluginObjectChanged)
	// AI-generated (Claude): GPS2 (exit) markers and dashed entry->exit segments
	// for the currently selected dives that recorded both GPS1 and GPS2.
	Q_PROPERTY(DiveTrackExitModel *diveTrackExits READ diveTrackExits CONSTANT)
	Q_PROPERTY(DiveTrackDashModel *diveTrackDashes READ diveTrackDashes CONSTANT)

public:
	explicit MapWidgetHelper(QObject *parent = NULL);

	void centerOnSelectedDiveSite();
	Q_INVOKABLE QGeoCoordinate getCoordinates(struct dive_site *ds);
	Q_INVOKABLE void centerOnDiveSite(struct dive_site *ds);
	Q_INVOKABLE void reloadMapLocations();
	// AI-generated (Claude): fit the viewport to all dive sites with GPS
	Q_INVOKABLE void centerOnAllSites();
	Q_INVOKABLE void copyToClipboardCoordinates(QGeoCoordinate coord, bool formatTraditional);
	Q_INVOKABLE void calculateSmallCircleRadius(QGeoCoordinate coord);
	Q_INVOKABLE void updateCurrentDiveSiteCoordinatesFromMap(struct dive_site *ds, QGeoCoordinate coord);
	Q_INVOKABLE void selectVisibleLocations();
	Q_INVOKABLE void selectedLocationChanged(struct dive_site *ds);
	void setSelected(const std::vector<dive_site *> divesites);
	QString pluginObject();
	bool editMode() const;
	// AI-generated (Claude)
	DiveTrackExitModel *diveTrackExits() { return &m_diveTrackExits; }
	DiveTrackDashModel *diveTrackDashes() { return &m_diveTrackDashes; }

private:
	void updateEditMode();
	// AI-generated (Claude): rebuild GPS1/GPS2 track data from current selection
	void updateDiveTracks();
	QObject *m_map;
	MapLocationModel *m_mapLocationModel;
	qreal m_smallCircleRadius;
	bool m_editMode;
	// AI-generated (Claude): "previous selection was a single site" — used to
	// decide whether centerOnCoordinate should preserve the user's zoom.
	bool m_prevSingleSite = false;
	// AI-generated (Claude)
	DiveTrackExitModel m_diveTrackExits;
	DiveTrackDashModel m_diveTrackDashes;

private slots:
	void diveSiteChanged(struct dive_site *ds, int field);

signals:
	void modelChanged();
	void editModeChanged();
	void selectedDivesChanged(const QList<int> &list);
	void coordinatesChanged(struct dive_site *ds, const location_t &);
	void pluginObjectChanged();
	// AI-generated (Claude): emitted on map-marker click while in dive-site
	// mode. Carries all sites within the small-circle radius of the clicked
	// marker (so a zoomed-out click picks up nearby sites too).
	void selectedDiveSitesFromMap(const QVector<dive_site *> &sites);
};

#endif
