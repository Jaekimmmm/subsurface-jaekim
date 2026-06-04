// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELOCATIONMODEL_H
#define DIVELOCATIONMODEL_H

#include <QAbstractTableModel>
#include <QAbstractItemModel>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include "core/units.h"
// AI-generated (Claude)
#include "core/taxonomy.h"
#include <memory>
#include <vector>

struct dive;
struct dive_trip;

class LocationInformationModel : public QAbstractTableModel {
	Q_OBJECT
public:
	// Common columns, roles and accessor function for all dive-site models.
	// Thus, different views can connect to different models.
	enum Columns { EDIT, REMOVE, NAME, DESCRIPTION, NUM_DIVES, LOCATION, NOTES, DIVESITE, TAXONOMY, COLUMNS };
	enum Roles { DIVESITE_ROLE = Qt::UserRole + 1 };
	static QVariant getDiveSiteData(const struct dive_site &ds, int column, int role);

	LocationInformationModel(QObject *obj = 0);
	static LocationInformationModel *instance();
	int columnCount(const QModelIndex &parent) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index = QModelIndex(), int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

public slots:
	void update();
private slots:
	void diveSiteDiveCountChanged(struct dive_site *ds);
	void diveSiteAdded(struct dive_site *ds, int idx);
	void diveSiteDeleted(struct dive_site *ds, int idx);
	void diveSiteChanged(struct dive_site *ds, int field);
	void diveSiteDivesChanged(struct dive_site *ds);
};

class DiveSiteSortedModel : public QSortFilterProxyModel {
	Q_OBJECT
private:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &source_parent) const override;
	bool lessThan(const QModelIndex &i1, const QModelIndex &i2) const override;
	QString fullText;
#ifndef SUBSURFACE_MOBILE
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
#endif // SUBSURFACE_MOBILE
public:
	DiveSiteSortedModel(QObject *parent = nullptr);
	QStringList allSiteNames() const;
	void setFilter(const QString &text);
	struct dive_site *getDiveSite(const QModelIndex &idx);
};

// To access only divesites at the given GPS coordinates with the exception of a given dive site
class GPSLocationInformationModel : public QSortFilterProxyModel {
	Q_OBJECT
private:
	const struct dive_site *ignoreDs;
	location_t location;
	int64_t distance;
	bool filterAcceptsRow(int sourceRow, const QModelIndex &source_parent) const override;
public:
	GPSLocationInformationModel(QObject *parent = nullptr);
	void set(const struct dive_site *ignoreDs, const location_t &);
	void setCoordinates(const location_t &);
	void setDistance(int64_t dist); // Distance from coordinates in mm
};

// AI-generated (Claude): Tree view of dive sites grouped by the user-enabled
// taxonomy categories from prefs.geocoding.category[]. The leaves are
// dive_site* and the internal nodes are taxonomy values. The tree is
// rebuilt on any divesite signal or prefs change — simple but adequate for
// the typical dive-site count (hundreds, not millions).
class DiveSiteTreeModel : public QAbstractItemModel {
	Q_OBJECT
public:
	enum Columns { NAME, NUM_DIVES, COLUMNS };
	enum Roles { DIVESITE_ROLE = Qt::UserRole + 1 };

	DiveSiteTreeModel(QObject *parent = nullptr);
	~DiveSiteTreeModel() override;

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	struct dive_site *getDiveSite(const QModelIndex &idx) const;
	int hierarchyDepth() const { return static_cast<int>(levels.size()); }

public slots:
	void rebuild();
	// AI-generated (Claude): coalesce many close-together triggers into a
	// single rebuild on the next event-loop tick.
	void scheduleRebuild();

private:
	struct Node {
		Node *parent = nullptr;
		int rowInParent = 0;
		// For group nodes: text + child collection; for leaves: ds is set
		QString label;
		struct dive_site *ds = nullptr;
		int diveCount = 0; // aggregated for groups, ds->dives.size() for leaves
		std::vector<std::unique_ptr<Node>> children;
	};
	std::unique_ptr<Node> root;
	std::vector<enum taxonomy_category> levels;
	bool rebuildPending = false; // AI-generated (Claude)

	Node *nodeOf(const QModelIndex &idx) const;
};

class GeoReferencingOptionsModel : public QStringListModel {
	Q_OBJECT
public:
	static GeoReferencingOptionsModel *instance();
private:
	GeoReferencingOptionsModel(QObject *parent = 0);
};

#endif
