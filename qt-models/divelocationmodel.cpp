// SPDX-License-Identifier: GPL-2.0
#include "core/units.h"
#include "qt-models/divelocationmodel.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/errorhelper.h"
#include "core/divesite.h"
#include "core/divelog.h"
#include "core/metrics.h"
#ifndef SUBSURFACE_MOBILE
#include "cleanertablemodel.h" // for trashIcon() and editIcon()
#include "commands/command.h"
#endif
#include <QLineEdit>
#include <QIcon>
#include <core/gettextfromc.h>
// AI-generated (Claude)
#include "core/settings/qPrefGeocoding.h"
#include <algorithm>
#include <functional>

LocationInformationModel *LocationInformationModel::instance()
{
	static LocationInformationModel *self = new LocationInformationModel();
	return self;
}

LocationInformationModel::LocationInformationModel(QObject *obj) : QAbstractTableModel(obj)
{
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDiveCountChanged, this, &LocationInformationModel::diveSiteDiveCountChanged);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteAdded, this, &LocationInformationModel::diveSiteAdded);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDeleted, this, &LocationInformationModel::diveSiteDeleted);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &LocationInformationModel::diveSiteChanged);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDivesChanged, this, &LocationInformationModel::diveSiteDivesChanged);
	connect(&diveListNotifier, &DiveListNotifier::dataReset, this, &LocationInformationModel::update);
}

int LocationInformationModel::columnCount(const QModelIndex &) const
{
	return COLUMNS;
}

int LocationInformationModel::rowCount(const QModelIndex &) const
{
	return (int)divelog.sites.size();
}

QVariant LocationInformationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Vertical)
		return QVariant();

	switch (role) {
	case Qt::TextAlignmentRole:
		return int(Qt::AlignLeft | Qt::AlignVCenter);
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::InitialSortOrderRole:
		// By default, sort number of dives descending, everything else ascending.
		return section == NUM_DIVES ? Qt::DescendingOrder : Qt::AscendingOrder;
	case Qt::DisplayRole:
	case Qt::ToolTipRole:
		switch (section) {
		case NAME:
			return tr("Name");
		case DESCRIPTION:
			return tr("Description");
		case NUM_DIVES:
			return tr("# of dives");
		// AI-generated (Claude)
		case TAXONOMY:
			return tr("Tags");
		}
		break;
	}

	return QVariant();
}

Qt::ItemFlags LocationInformationModel::flags(const QModelIndex &index) const
{
	switch (index.column()) {
	case REMOVE:
		return Qt::ItemIsEnabled;
	case NAME:
	case DESCRIPTION:
		return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
	}
	return QAbstractItemModel::flags(index);
}

QVariant LocationInformationModel::getDiveSiteData(const struct dive_site &ds, int column, int role)
{
	switch(role) {
	case Qt::EditRole:
	case Qt::DisplayRole:
		switch(column) {
		case DIVESITE: return QVariant::fromValue<dive_site *>((dive_site *)&ds); // Not nice: casting away const
		case NAME: return QString::fromStdString(ds.name);
		case NUM_DIVES: return static_cast<int>(ds.dives.size());
		case LOCATION: return "TODO";
		case DESCRIPTION: return QString::fromStdString(ds.description);
		case NOTES: return QString::fromStdString(ds.notes);
		// AI-generated (Claude): join the user-enabled taxonomy values with " / "
		case TAXONOMY: {
			QString out;
			for (enum taxonomy_category cat: taxonomy_active_categories()) {
				std::string v = taxonomy_get_value(ds.taxonomy, cat);
				if (v.empty())
					continue;
				if (!out.isEmpty())
					out += " / ";
				out += QString::fromStdString(v);
			}
			return out;
		}
		}
	break;
	case Qt::ToolTipRole:
		switch(column) {
		case EDIT: return tr("Click here to edit the divesite.");
		case REMOVE: return tr("Clicking here will remove this divesite.");
		}
	break;
	case Qt::DecorationRole:
		switch(column) {
#ifndef SUBSURFACE_MOBILE
		case EDIT: return editIcon();
		case REMOVE: return trashIcon();
#endif
		case NAME: return ds.has_gps_location() ? QIcon(":geotag-icon") : QVariant();
		}
	break;
	case DIVESITE_ROLE:
		return QVariant::fromValue<dive_site *>((dive_site *)&ds); // Not nice: casting away const
	}
	return QVariant();
}

QVariant LocationInformationModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= (int)divelog.sites.size())
		return QVariant();

	const auto &ds = (divelog.sites)[index.row()].get();
	return getDiveSiteData(*ds, index.column(), role);
}

void LocationInformationModel::update()
{
	beginResetModel();
	endResetModel();
}

void LocationInformationModel::diveSiteDiveCountChanged(dive_site *ds)
{
	size_t idx = divelog.sites.get_idx(ds);
	if (idx != std::string::npos)
		dataChanged(createIndex(idx, NUM_DIVES), createIndex(idx, NUM_DIVES));
}

void LocationInformationModel::diveSiteAdded(struct dive_site *, int idx)
{
	if (idx < 0)
		return;
	beginInsertRows(QModelIndex(), idx, idx);
	// Row has already been added by Undo-Command.
	endInsertRows();
}

void LocationInformationModel::diveSiteDeleted(struct dive_site *, int idx)
{
	if (idx < 0)
		return;
	beginRemoveRows(QModelIndex(), idx, idx);
	// Row has already been added by Undo-Command.
	endRemoveRows();
}

void LocationInformationModel::diveSiteChanged(struct dive_site *ds, int field)
{
	size_t idx = divelog.sites.get_idx(ds);
	if (idx == std::string::npos)
		return;
	dataChanged(createIndex(idx, field), createIndex(idx, field));
}

void LocationInformationModel::diveSiteDivesChanged(struct dive_site *ds)
{
	size_t idx = divelog.sites.get_idx(ds);
	if (idx == std::string::npos)
		return;
	dataChanged(createIndex(idx, NUM_DIVES), createIndex(idx, NUM_DIVES));
}

bool DiveSiteSortedModel::filterAcceptsRow(int sourceRow, const QModelIndex &source_parent) const
{
	if (fullText.isEmpty())
		return true;

	if (sourceRow < 0 || sourceRow > (int)divelog.sites.size())
		return false;
	const auto &ds = (divelog.sites)[sourceRow];
	QString text = QString::fromStdString(ds->name + ds->description + ds->notes);
	return text.contains(fullText, Qt::CaseInsensitive);
}

bool DiveSiteSortedModel::lessThan(const QModelIndex &i1, const QModelIndex &i2) const
{
	// The source indices correspond to indices in the global dive site table.
	// Let's access them directly without going via the source model.
	// Kind of dirty, but less effort.

	// Be careful to respect proper ordering when sites are invalid.
	bool valid1 = i1.row() >= 0 && i1.row() < (int)divelog.sites.size();
	bool valid2 = i2.row() >= 0 && i2.row() < (int)divelog.sites.size();
	if (!valid1 || !valid2)
		return valid1 < valid2;

	const auto &ds1 = (divelog.sites)[i1.row()];
	const auto &ds2 = (divelog.sites)[i2.row()];
	switch (i1.column()) {
	case LocationInformationModel::NAME:
	default:
		return QString::localeAwareCompare(QString::fromStdString(ds1->name), QString::fromStdString(ds2->name)) < 0; // TODO: avoid copy
	case LocationInformationModel::DESCRIPTION: {
		int cmp = QString::localeAwareCompare(QString::fromStdString(ds1->description), QString::fromStdString(ds2->description)); // TODO: avoid copy
		return cmp != 0 ? cmp < 0 :
		       QString::localeAwareCompare(QString::fromStdString(ds1->name), QString::fromStdString(ds2->name)) < 0; // TODO: avoid copy
	}
	// AI-generated (Claude): sort the Tags column by the joined taxonomy string
	case LocationInformationModel::TAXONOMY: {
		auto joined = [](const taxonomy_data &t) {
			QString s;
			for (enum taxonomy_category cat: taxonomy_active_categories()) {
				std::string v = taxonomy_get_value(t, cat);
				if (v.empty()) continue;
				if (!s.isEmpty()) s += " / ";
				s += QString::fromStdString(v);
			}
			return s;
		};
		int cmp = QString::localeAwareCompare(joined(ds1->taxonomy), joined(ds2->taxonomy));
		return cmp != 0 ? cmp < 0 :
		       QString::localeAwareCompare(QString::fromStdString(ds1->name), QString::fromStdString(ds2->name)) < 0;
	}
	case LocationInformationModel::NUM_DIVES: {
		int cmp = static_cast<int>(ds1->dives.size()) - static_cast<int>(ds2->dives.size());
		// Since by default nr dives is descending, invert sort direction of names, such that
		// the names are listed as ascending.
		return cmp != 0 ? cmp < 0 :
		       QString::localeAwareCompare(QString::fromStdString(ds1->name), QString::fromStdString(ds2->name)) < 0; // TODO: avoid copy
	}
	}
}

DiveSiteSortedModel::DiveSiteSortedModel(QObject *parent) : QSortFilterProxyModel(parent)
{
	setSourceModel(LocationInformationModel::instance());
}

QStringList DiveSiteSortedModel::allSiteNames() const
{
	QStringList locationNames;
	int num = rowCount();
	for (int i = 0; i < num; i++) {
		int idx = mapToSource(index(i, 0)).row();
		// This shouldn't happen, but if model and core get out of sync,
		// (more precisely: the core has more sites than the model is aware of),
		// we might get an invalid index.
		if (idx < 0 || idx > (int)divelog.sites.size()) {
			report_info("DiveSiteSortedModel::allSiteNames(): invalid index");
			continue;
		}
		locationNames << QString::fromStdString((divelog.sites)[idx]->name);
	}
	return locationNames;
}

struct dive_site *DiveSiteSortedModel::getDiveSite(const QModelIndex &idx_source)
{
	auto idx = mapToSource(idx_source).row();
	return idx >= 0 && idx < (int)divelog.sites.size() ? (divelog.sites)[idx].get() : NULL;
}

#ifndef SUBSURFACE_MOBILE
bool DiveSiteSortedModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	struct dive_site *ds = getDiveSite(index);
	if (!ds || value.isNull())
		return false;
	switch (index.column()) {
	case LocationInformationModel::NAME:
		Command::editDiveSiteName(ds, value.toString());
		return true;
	case LocationInformationModel::DESCRIPTION:
		Command::editDiveSiteDescription(ds, value.toString());
		return true;
	default:
		return false;
	}
}

#endif // SUBSURFACE_MOBILE

void DiveSiteSortedModel::setFilter(const QString &text)
{
	fullText = text.trimmed();
	invalidateFilter();
}

// AI-generated (Claude): DiveSiteTreeModel implementation. Signals route
// through scheduleRebuild() so a bulk operation that emits N change signals
// triggers exactly one tree rebuild on the following event-loop tick.
DiveSiteTreeModel::DiveSiteTreeModel(QObject *parent) : QAbstractItemModel(parent)
{
	rebuild();
	connect(&diveListNotifier, &DiveListNotifier::diveSiteAdded,   this, &DiveSiteTreeModel::scheduleRebuild);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDeleted, this, &DiveSiteTreeModel::scheduleRebuild);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &DiveSiteTreeModel::scheduleRebuild);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDivesChanged,    this, &DiveSiteTreeModel::scheduleRebuild);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDiveCountChanged, this, &DiveSiteTreeModel::scheduleRebuild);
	connect(&diveListNotifier, &DiveListNotifier::dataReset,       this, &DiveSiteTreeModel::scheduleRebuild);
	auto onPrefsChanged = [this](taxonomy_category) { scheduleRebuild(); };
	connect(qPrefGeocoding::instance(), &qPrefGeocoding::first_taxonomy_categoryChanged,  this, onPrefsChanged);
	connect(qPrefGeocoding::instance(), &qPrefGeocoding::second_taxonomy_categoryChanged, this, onPrefsChanged);
	connect(qPrefGeocoding::instance(), &qPrefGeocoding::third_taxonomy_categoryChanged,  this, onPrefsChanged);
	connect(qPrefGeocoding::instance(), &qPrefGeocoding::fourth_taxonomy_categoryChanged, this, onPrefsChanged);
	connect(qPrefGeocoding::instance(), &qPrefGeocoding::fifth_taxonomy_categoryChanged,  this, onPrefsChanged);
}

// AI-generated (Claude)
void DiveSiteTreeModel::scheduleRebuild()
{
	if (rebuildPending)
		return;
	rebuildPending = true;
	QMetaObject::invokeMethod(this, [this]() {
		rebuildPending = false;
		rebuild();
	}, Qt::QueuedConnection);
}

DiveSiteTreeModel::~DiveSiteTreeModel() = default;

void DiveSiteTreeModel::rebuild()
{
	beginResetModel();
	levels = taxonomy_active_categories();
	root = std::make_unique<Node>();

	// Bucketize each site into the tree by walking the level chain.
	for (const auto &ds: divelog.sites) {
		Node *cur = root.get();
		for (enum taxonomy_category cat: levels) {
			std::string raw = taxonomy_get_value(ds->taxonomy, cat);
			QString label = raw.empty()
				? QObject::tr("(unspecified)")
				: QString::fromStdString(raw);
			auto it = std::find_if(cur->children.begin(), cur->children.end(),
					       [&label](const std::unique_ptr<Node> &n) {
						       return n->ds == nullptr && n->label == label;
					       });
			Node *next;
			if (it == cur->children.end()) {
				auto n = std::make_unique<Node>();
				n->parent = cur;
				n->rowInParent = static_cast<int>(cur->children.size());
				n->label = label;
				next = n.get();
				cur->children.push_back(std::move(n));
			} else {
				next = it->get();
			}
			cur = next;
		}
		// Append the leaf (the site itself)
		auto leaf = std::make_unique<Node>();
		leaf->parent = cur;
		leaf->rowInParent = static_cast<int>(cur->children.size());
		leaf->label = QString::fromStdString(ds->name);
		leaf->ds = ds.get();
		leaf->diveCount = static_cast<int>(ds->dives.size());
		cur->children.push_back(std::move(leaf));
	}

	// Sort children alphabetically at every level, then aggregate dive counts.
	std::function<void(Node *)> finalize = [&](Node *n) {
		std::sort(n->children.begin(), n->children.end(),
			  [](const std::unique_ptr<Node> &a, const std::unique_ptr<Node> &b) {
				  return QString::localeAwareCompare(a->label, b->label) < 0;
			  });
		for (size_t i = 0; i < n->children.size(); ++i)
			n->children[i]->rowInParent = static_cast<int>(i);
		int sum = 0;
		for (auto &c: n->children) {
			finalize(c.get());
			sum += c->diveCount;
		}
		if (!n->children.empty())
			n->diveCount = sum;
	};
	finalize(root.get());

	endResetModel();
}

DiveSiteTreeModel::Node *DiveSiteTreeModel::nodeOf(const QModelIndex &idx) const
{
	return idx.isValid() ? static_cast<Node *>(idx.internalPointer()) : root.get();
}

QModelIndex DiveSiteTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!root || row < 0 || column < 0 || column >= COLUMNS)
		return QModelIndex();
	Node *p = nodeOf(parent);
	if (!p || row >= static_cast<int>(p->children.size()))
		return QModelIndex();
	return createIndex(row, column, p->children[row].get());
}

QModelIndex DiveSiteTreeModel::parent(const QModelIndex &idx) const
{
	if (!idx.isValid())
		return QModelIndex();
	Node *n = static_cast<Node *>(idx.internalPointer());
	if (!n || !n->parent || n->parent == root.get())
		return QModelIndex();
	return createIndex(n->parent->rowInParent, 0, n->parent);
}

int DiveSiteTreeModel::rowCount(const QModelIndex &parent) const
{
	if (!root)
		return 0;
	if (parent.column() > 0)
		return 0;
	Node *p = nodeOf(parent);
	return p ? static_cast<int>(p->children.size()) : 0;
}

int DiveSiteTreeModel::columnCount(const QModelIndex &) const
{
	return COLUMNS;
}

QVariant DiveSiteTreeModel::data(const QModelIndex &idx, int role) const
{
	if (!idx.isValid())
		return QVariant();
	Node *n = static_cast<Node *>(idx.internalPointer());
	if (!n)
		return QVariant();
	if (role == DIVESITE_ROLE)
		return QVariant::fromValue<dive_site *>(n->ds);
	if (role == Qt::DecorationRole && idx.column() == NAME && n->ds && n->ds->has_gps_location())
		return QIcon(":geotag-icon");
	if (role == Qt::FontRole)
		return defaultModelFont();
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole)
		return QVariant();
	switch (idx.column()) {
	case NAME:
		return n->label;
	case NUM_DIVES:
		return n->diveCount;
	}
	return QVariant();
}

QVariant DiveSiteTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return QVariant();
	switch (section) {
	case NAME: return tr("Name");
	case NUM_DIVES: return tr("# of dives");
	}
	return QVariant();
}

struct dive_site *DiveSiteTreeModel::getDiveSite(const QModelIndex &idx) const
{
	if (!idx.isValid())
		return nullptr;
	Node *n = static_cast<Node *>(idx.internalPointer());
	return n ? n->ds : nullptr;
}

GeoReferencingOptionsModel *GeoReferencingOptionsModel::instance()
{
	static GeoReferencingOptionsModel *self = new GeoReferencingOptionsModel();
	return self;
}

GeoReferencingOptionsModel::GeoReferencingOptionsModel(QObject *parent) : QStringListModel(parent)
{
	QStringList list;
	int i;
	for (i = 0; i < TC_NR_CATEGORIES; i++)
		list << gettextFromC::tr(taxonomy_category_names[i]);
	setStringList(list);
}

bool GPSLocationInformationModel::filterAcceptsRow(int sourceRow, const QModelIndex &parent) const
{
	if (!has_location(&location))
		return false;

	struct dive_site *ds = sourceModel()->index(sourceRow, LocationInformationModel::DIVESITE, parent).data().value<dive_site *>();
	if (!ds || ds == ignoreDs || !has_location(&ds->location))
		return false;

	return distance <= 0 ? ds->location == location
			     : (int64_t)get_distance(ds->location, location) * 1000 <= distance; // We need 64 bit to represent distances in mm
}

GPSLocationInformationModel::GPSLocationInformationModel(QObject *parent) : QSortFilterProxyModel(parent),
	ignoreDs(nullptr),
	distance(0)
{
	setSourceModel(LocationInformationModel::instance());
}

void GPSLocationInformationModel::set(const struct dive_site *ignoreDsIn, const location_t &locationIn)
{
	ignoreDs = ignoreDsIn;
	location = locationIn;
	invalidate();
}

void GPSLocationInformationModel::setCoordinates(const location_t &locationIn)
{
	location = locationIn;
	invalidate();
}

void GPSLocationInformationModel::setDistance(int64_t dist)
{
	distance = dist;
	invalidate();
}
