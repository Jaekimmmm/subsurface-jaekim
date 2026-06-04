// SPDX-License-Identifier: GPL-2.0
#include "divesitelistview.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/divefilter.h"
#include "qt-models/divelocationmodel.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/bulkdivesiteedit.h"
#include "desktop-widgets/mapwidget.h"
#include "commands/command.h"

#include <QMessageBox>
// AI-generated (Claude)
#include <QGuiApplication>
#include <QTreeView>
#include <QHeaderView>
#include <QSettings>
#include <functional>

DiveSiteListView::DiveSiteListView(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);

	// What follows is duplicate code with locationinformation.cpp.
	// We might want to unify this.
	ui.diveSiteMessage->setCloseButtonVisible(false);

	QAction *acceptAction = new QAction(tr("Done"), this);
	connect(acceptAction, &QAction::triggered, this, &DiveSiteListView::done);

	ui.diveSiteMessage->setText(tr("Dive site management"));
	ui.diveSiteMessage->addAction(acceptAction);

	model = new DiveSiteSortedModel(this);
	ui.diveSites->setTitle(tr("Dive sites"));
	ui.diveSites->setModel(model);
	// Default: sort by name
	ui.diveSites->view()->sortByColumn(LocationInformationModel::NAME, Qt::AscendingOrder);
	ui.diveSites->view()->setSortingEnabled(true);
	// AI-generated (Claude): user-resizable / movable columns; saved state
	// (widths, order, visibility, sort indicator) is restored at the end of
	// the constructor.
	auto *header0 = ui.diveSites->view()->horizontalHeader();
	header0->setSectionResizeMode(QHeaderView::Interactive);
	header0->setStretchLastSection(true);
	header0->setSectionsMovable(true);
	header0->resizeSection(LocationInformationModel::NAME,        220);
	header0->resizeSection(LocationInformationModel::TAXONOMY,    240);
	header0->resizeSection(LocationInformationModel::NUM_DIVES,    90);
	header0->resizeSection(LocationInformationModel::DESCRIPTION, 240);
	ui.diveSites->view()->setSelectionBehavior(QAbstractItemView::SelectRows);

	// AI-generated (Claude): keep Name / Tags / # of dives / Description visible
	// (in that order); hide LOCATION/NOTES/DIVESITE internals.
	for (int i = LocationInformationModel::LOCATION; i < LocationInformationModel::COLUMNS; ++i) {
		if (i == LocationInformationModel::TAXONOMY)
			continue;
		ui.diveSites->view()->setColumnHidden(i, true);
	}
	// AI-generated (Claude): reorder visible columns -> Name, Tags, # of dives, Description.
	auto *hdr = ui.diveSites->view()->horizontalHeader();
	const int order[] = {
		LocationInformationModel::NAME,
		LocationInformationModel::TAXONOMY,
		LocationInformationModel::NUM_DIVES,
		LocationInformationModel::DESCRIPTION,
	};
	int target = hdr->visualIndex(LocationInformationModel::NAME);
	for (int logical: order)
		hdr->moveSection(hdr->visualIndex(logical), target++);

	connect(ui.diveSites, &TableView::addButtonClicked, this, &DiveSiteListView::add);
	connect(ui.diveSites, &TableView::itemClicked, this, &DiveSiteListView::diveSiteClicked);
	connect(ui.diveSites->view()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DiveSiteListView::selectionChanged);
	// AI-generated (Claude): keep the Bulk-edit button enabled only when >=2 rows selected
	connect(ui.diveSites->view()->selectionModel(), &QItemSelectionModel::selectionChanged,
		this, &DiveSiteListView::refreshBulkEditEnabled);

	// Subtle: We depend on this slot being executed after the slot in the model.
	// This is realized because the model was constructed as a member object and connects in the constructor.
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &DiveSiteListView::diveSiteChanged);

	// AI-generated (Claude): tree view for hierarchical grouping
	treeModel = new DiveSiteTreeModel(this);
	ui.diveSitesTree->setModel(treeModel);
	ui.diveSitesTree->header()->setSectionResizeMode(DiveSiteTreeModel::NAME, QHeaderView::Stretch);
	connect(ui.diveSitesTree->selectionModel(), &QItemSelectionModel::selectionChanged,
		this, &DiveSiteListView::treeSelectionChanged);

	// AI-generated (Claude): on first open and after any rebuild, expand every
	// grouping level except the deepest one so leaves stay tucked away.
	connect(treeModel, &QAbstractItemModel::modelReset, this, &DiveSiteListView::applyTreeDefaultExpansion);
	applyTreeDefaultExpansion();

	// AI-generated (Claude): bulk-edit also enabled when the tree-view selection has >=2 leaves
	connect(ui.diveSitesTree->selectionModel(), &QItemSelectionModel::selectionChanged,
		this, &DiveSiteListView::refreshBulkEditEnabled);

	// AI-generated (Claude): map-marker clicks in dive-site mode select rows
	connect(MapWidget::instance(), &MapWidget::selectedDiveSitesFromMap,
		this, &DiveSiteListView::selectDiveSitesFromMap);

	// AI-generated (Claude): restore persisted header / tree-header states
	QSettings hs;
	const QByteArray flatBlob = hs.value("DiveSiteListView/flatHeaderState").toByteArray();
	if (!flatBlob.isEmpty())
		header0->restoreState(flatBlob);
	auto *treeHdr = ui.diveSitesTree->header();
	treeHdr->setSectionsMovable(true);
	treeHdr->setSectionResizeMode(QHeaderView::Interactive);
	treeHdr->setStretchLastSection(true);
	const QByteArray treeBlob = hs.value("DiveSiteListView/treeHeaderState").toByteArray();
	if (!treeBlob.isEmpty())
		treeHdr->restoreState(treeBlob);
}

// AI-generated (Claude): persist header states on destruction.
DiveSiteListView::~DiveSiteListView()
{
	QSettings hs;
	hs.setValue("DiveSiteListView/flatHeaderState",
	            ui.diveSites->view()->horizontalHeader()->saveState());
	hs.setValue("DiveSiteListView/treeHeaderState",
	            ui.diveSitesTree->header()->saveState());
}

// AI-generated (Claude): if Shift/Ctrl/Cmd is held at click time, append to
// the existing selection (or toggle if the row is already selected); otherwise
// replace.
void DiveSiteListView::selectDiveSitesFromMap(const QVector<dive_site *> &sitesIn)
{
	if (sitesIn.isEmpty())
		return;

	const auto mods = QGuiApplication::keyboardModifiers();
	const bool extendSel = mods & (Qt::ShiftModifier | Qt::ControlModifier | Qt::MetaModifier);

	// Flat view: map each dive_site to its row index in the global table,
	// then through the proxy model, and select all rows.
	if (ui.viewStack->currentWidget() == ui.flatPage) {
		auto *selModel = ui.diveSites->view()->selectionModel();
		const int cols = LocationInformationModel::COLUMNS - 1;
		QItemSelection sel;
		for (dive_site *target: sitesIn) {
			size_t idx = divelog.sites.get_idx(target);
			if (idx == std::string::npos) continue;
			QModelIndex src = LocationInformationModel::instance()->index(static_cast<int>(idx), 0);
			QModelIndex proxy = model->mapFromSource(src);
			if (!proxy.isValid()) continue;
			sel.select(proxy, model->index(proxy.row(), cols));
		}
		if (sel.isEmpty())
			return;
		QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Rows;
		if (extendSel) {
			// Toggle: rows already selected get deselected, new ones added.
			QItemSelection deselect, addSel;
			for (const QItemSelectionRange &r: sel) {
				QModelIndex first = r.topLeft();
				if (selModel->isRowSelected(first.row(), first.parent()))
					deselect.select(r.topLeft(), r.bottomRight());
				else
					addSel.select(r.topLeft(), r.bottomRight());
			}
			if (!deselect.isEmpty())
				selModel->select(deselect, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
			if (!addSel.isEmpty())
				selModel->select(addSel,   QItemSelectionModel::Select   | QItemSelectionModel::Rows);
		} else {
			selModel->select(sel, QItemSelectionModel::ClearAndSelect | flags);
		}
		ui.diveSites->view()->scrollTo(sel.indexes().first());
		return;
	}

	// Tree view: walk the tree, collect indices whose leaf node matches one of
	// the input sites.
	QItemSelection sel;
	std::function<void(const QModelIndex &)> walk = [&](const QModelIndex &parent) {
		int rows = treeModel->rowCount(parent);
		for (int r = 0; r < rows; ++r) {
			QModelIndex idx = treeModel->index(r, 0, parent);
			if (struct dive_site *ds = treeModel->getDiveSite(idx)) {
				if (sitesIn.contains(ds))
					sel.select(idx, treeModel->index(r, DiveSiteTreeModel::COLUMNS - 1, parent));
			} else {
				walk(idx);
			}
		}
	};
	walk(QModelIndex());
	if (sel.isEmpty())
		return;
	auto *selModel = ui.diveSitesTree->selectionModel();
	if (extendSel) {
		QItemSelection deselect, addSel;
		for (const QItemSelectionRange &r: sel) {
			QModelIndex first = r.topLeft();
			if (selModel->isRowSelected(first.row(), first.parent()))
				deselect.select(r.topLeft(), r.bottomRight());
			else
				addSel.select(r.topLeft(), r.bottomRight());
		}
		if (!deselect.isEmpty())
			selModel->select(deselect, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
		if (!addSel.isEmpty())
			selModel->select(addSel,   QItemSelectionModel::Select   | QItemSelectionModel::Rows);
	} else {
		selModel->select(sel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
	}
	ui.diveSitesTree->scrollTo(sel.indexes().first());
	// Make sure the parents are expanded so the leaf is visible.
	for (const QModelIndex &i: sel.indexes()) {
		QModelIndex p = i.parent();
		while (p.isValid()) {
			ui.diveSitesTree->expand(p);
			p = p.parent();
		}
	}
}

// AI-generated (Claude)
void DiveSiteListView::refreshBulkEditEnabled()
{
	auto sites = (ui.viewStack->currentWidget() == ui.treePage)
	             ? selectedDiveSitesTree() : selectedDiveSites();
	ui.bulkEdit->setEnabled(sites.size() >= 1);
}

// AI-generated (Claude)
void DiveSiteListView::on_bulkEdit_clicked()
{
	auto sitesVec = (ui.viewStack->currentWidget() == ui.treePage)
	                ? selectedDiveSitesTree() : selectedDiveSites();
	if (sitesVec.empty())
		return;
	QVector<dive_site *> sites;
	sites.reserve(static_cast<int>(sitesVec.size()));
	for (dive_site *ds: sitesVec)
		if (ds) sites.push_back(ds);
	BulkDiveSiteEditDialog dlg(sites, this);
	if (dlg.exec() == QDialog::Accepted)
		Command::editDiveSitesBulk(sites, dlg.edit());
}

// AI-generated (Claude)
void DiveSiteListView::applyTreeDefaultExpansion()
{
	const int n = treeModel->hierarchyDepth();
	if (n >= 2)
		ui.diveSitesTree->expandToDepth(n - 2);
	else
		ui.diveSitesTree->collapseAll();
}

void DiveSiteListView::done()
{
	MainWindow::instance()->enterPreviousState();
}

void DiveSiteListView::diveSiteClicked(const QModelIndex &index)
{
	struct dive_site *ds = model->getDiveSite(index);
	if (!ds)
		return;
	switch (index.column()) {
	case LocationInformationModel::EDIT:
		MainWindow::instance()->editDiveSite(ds);
		break;
	case LocationInformationModel::REMOVE:
		if (!ds->dives.empty() &&
		    QMessageBox::warning(this, tr("Delete dive site?"),
					 tr("This dive site has %n dive(s). Do you really want to delete it?\n", "", ds->dives.size()),
					 QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
				return;
		Command::deleteDiveSites(QVector<dive_site *>{ds});
		break;
	}
}

void DiveSiteListView::add()
{
	// This is mighty dirty: We hook into the "dive site added" signal and
	// select the name field of the added dive site when the command sends
	// the signal. This works only because we know that the model added the
	// connection first. Very subtle!
	// After the command has finished, the signal is disconnected so that dive
	// site names are not selected on regular redo / undo.
	connect(&diveListNotifier, &DiveListNotifier::diveSiteAdded, this, &DiveSiteListView::diveSiteAdded);
	Command::addDiveSite(tr("New dive site"));
	disconnect(&diveListNotifier, &DiveListNotifier::diveSiteAdded, this, &DiveSiteListView::diveSiteAdded);
}

void DiveSiteListView::diveSiteAdded(struct dive_site *, int idx)
{
	if (idx < 0)
		return;
	QModelIndex globalIdx = LocationInformationModel::instance()->index(idx, LocationInformationModel::NAME);
	QModelIndex localIdx = model->mapFromSource(globalIdx);
	ui.diveSites->view()->setCurrentIndex(localIdx);
	ui.diveSites->view()->edit(localIdx);
}

void DiveSiteListView::diveSiteChanged(struct dive_site *ds, int field)
{
	size_t idx = divelog.sites.get_idx(ds);
	if (idx == std::string::npos)
		return;
	QModelIndex globalIdx = LocationInformationModel::instance()->index(static_cast<int>(idx), field);
	QModelIndex localIdx = model->mapFromSource(globalIdx);
	ui.diveSites->view()->scrollTo(localIdx);
}

void DiveSiteListView::on_purgeUnused_clicked()
{
	Command::purgeUnusedDiveSites();
}

void DiveSiteListView::on_filterText_textChanged(const QString &text)
{
	model->setFilter(text);
}

std::vector<dive_site *> DiveSiteListView::selectedDiveSites()
{
	const QModelIndexList indices = ui.diveSites->view()->selectionModel()->selectedRows();
	std::vector<dive_site *> sites;
	sites.reserve(indices.size());
	for (const QModelIndex &idx: indices) {
		struct dive_site *ds = model->getDiveSite(idx);
		sites.push_back(ds);
	}
	return sites;
}

// AI-generated (Claude): collect leaf sites from a tree-view selection.
// When the user picks an internal group node, take all leaves below it.
std::vector<dive_site *> DiveSiteListView::selectedDiveSitesTree()
{
	std::vector<dive_site *> sites;
	const QModelIndexList indices = ui.diveSitesTree->selectionModel()->selectedRows();
	std::function<void(const QModelIndex &)> collect = [&](const QModelIndex &idx) {
		if (struct dive_site *ds = treeModel->getDiveSite(idx)) {
			sites.push_back(ds);
			return;
		}
		int rows = treeModel->rowCount(idx);
		for (int r = 0; r < rows; ++r)
			collect(treeModel->index(r, 0, idx));
	};
	for (const QModelIndex &idx: indices)
		collect(idx);
	return sites;
}

void DiveSiteListView::selectionChanged(const QItemSelection &, const QItemSelection &)
{
	DiveFilter::instance()->setFilterDiveSite(selectedDiveSites());
}

// AI-generated (Claude)
void DiveSiteListView::treeSelectionChanged(const QItemSelection &, const QItemSelection &)
{
	if (ui.viewStack->currentWidget() == ui.treePage)
		DiveFilter::instance()->setFilterDiveSite(selectedDiveSitesTree());
}

// AI-generated (Claude)
void DiveSiteListView::on_treeToggle_toggled(bool checked)
{
	ui.viewStack->setCurrentWidget(checked ? ui.treePage : ui.flatPage);
	if (checked)
		DiveFilter::instance()->setFilterDiveSite(selectedDiveSitesTree());
	else
		DiveFilter::instance()->setFilterDiveSite(selectedDiveSites());
}

void DiveSiteListView::hideEvent(QHideEvent *)
{
	// If the user switches to the dive site tab and there was already a selection,
	// filter on that selection.
	DiveFilter::instance()->stopFilterDiveSites();
}

void DiveSiteListView::showEvent(QShowEvent *)
{
	// If the user switches to the dive site tab and there was already a selection,
	// filter on that selection.
	// AI-generated (Claude): respect the active view (flat vs tree)
	std::vector<dive_site *> sel =
		ui.viewStack->currentWidget() == ui.treePage ? selectedDiveSitesTree() : selectedDiveSites();
	DiveFilter::instance()->startFilterDiveSites(sel);
}
