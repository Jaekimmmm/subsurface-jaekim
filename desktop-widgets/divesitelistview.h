// SPDX-License-Identifier: GPL-2.0
#ifndef DIVE_SITE_LIST_VIEW_H
#define DIVE_SITE_LIST_VIEW_H

#include "ui_divesitelistview.h"

class DiveSiteSortedModel;
class DiveSiteTreeModel;

class DiveSiteListView : public QWidget {
	Q_OBJECT
public:
	DiveSiteListView(QWidget *parent = 0);
	~DiveSiteListView() override;
private slots:
	void add();
	void done();
	void diveSiteAdded(struct dive_site *, int idx);
	void diveSiteChanged(struct dive_site *ds, int field);
	void diveSiteClicked(const QModelIndex &);
	void on_purgeUnused_clicked();
	void on_filterText_textChanged(const QString &text);
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	// AI-generated (Claude)
	void on_treeToggle_toggled(bool checked);
	void treeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void applyTreeDefaultExpansion();
	void on_bulkEdit_clicked();
	void refreshBulkEditEnabled();
	// AI-generated (Claude): select rows for dive sites that were chosen via a
	// map-marker click while in dive-site mode.
	void selectDiveSitesFromMap(const QVector<dive_site *> &sites);
private:
	Ui::DiveSiteListView ui;
	DiveSiteSortedModel *model;
	// AI-generated (Claude)
	DiveSiteTreeModel *treeModel;
	std::vector<dive_site *> selectedDiveSites();
	std::vector<dive_site *> selectedDiveSitesTree();
	void hideEvent(QHideEvent *) override;
	void showEvent(QShowEvent *) override;
};

#endif
