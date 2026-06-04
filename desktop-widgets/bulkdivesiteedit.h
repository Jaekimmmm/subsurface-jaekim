// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude): modal dialog for bulk editing description, notes
// and per-category taxonomy values on a set of dive sites.
#ifndef BULKDIVESITEEDIT_H
#define BULKDIVESITEEDIT_H

#include "commands/command.h"
#include "core/taxonomy.h"

#include <QDialog>
#include <QVector>
#include <vector>

class QButtonGroup;
class QLineEdit;
class QTableWidget;
class QPlainTextEdit;
struct dive_site;

class BulkDiveSiteEditDialog : public QDialog {
	Q_OBJECT
public:
	BulkDiveSiteEditDialog(const QVector<dive_site *> &sites, QWidget *parent = nullptr);
	Command::BulkDiveSiteEdit edit() const;

private slots:
	void updatePreview();

private:
	struct FieldRow {
		QButtonGroup *modes;
		QLineEdit    *input;        // single-line for taxonomy
		QPlainTextEdit *textInput;  // multi-line for description / notes (nullptr for taxonomy)
		bool hasAppend;
	};
	FieldRow descriptionRow;
	FieldRow notesRow;
	std::vector<std::pair<taxonomy_category, FieldRow>> taxonomyRows;
	QVector<dive_site *> sites;
	QTableWidget *preview;

	FieldRow makeRow(const QString &label, bool allowClear, bool allowAppend, QWidget *container, int row);
	static Command::BulkDiveSiteEdit::Mode modeFromId(int id);
};

#endif
