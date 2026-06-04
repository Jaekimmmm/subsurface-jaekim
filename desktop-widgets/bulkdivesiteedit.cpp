// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "bulkdivesiteedit.h"
#include "core/divesite.h"
#include "core/gettextfromc.h"
#include "core/taxonomy.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

enum { ID_LEAVE = 0, ID_CLEAR = 1, ID_SET = 2, ID_APPEND = 3 };

Command::BulkDiveSiteEdit::Mode BulkDiveSiteEditDialog::modeFromId(int id)
{
	using M = Command::BulkDiveSiteEdit::Mode;
	switch (id) {
	case ID_CLEAR:  return M::Clear;
	case ID_SET:    return M::Set;
	case ID_APPEND: return M::Append;
	case ID_LEAVE:
	default:        return M::LeaveUnchanged;
	}
}

BulkDiveSiteEditDialog::FieldRow
BulkDiveSiteEditDialog::makeRow(const QString &label, bool allowClear, bool allowAppend, QWidget *container, int row)
{
	auto *grid = qobject_cast<QGridLayout *>(container->layout());
	auto *lbl = new QLabel("<b>" + label + "</b>", container);
	auto *rLeave  = new QRadioButton(tr("Leave"),  container);
	auto *rClear  = allowClear  ? new QRadioButton(tr("Clear"),  container) : nullptr;
	auto *rSet    = new QRadioButton(tr("Set"),    container);
	auto *rAppend = allowAppend ? new QRadioButton(tr("Append"), container) : nullptr;
	rLeave->setChecked(true);
	auto *group = new QButtonGroup(container);
	group->addButton(rLeave,  ID_LEAVE);
	if (rClear)  group->addButton(rClear,  ID_CLEAR);
	group->addButton(rSet,    ID_SET);
	if (rAppend) group->addButton(rAppend, ID_APPEND);

	QLineEdit *line = nullptr;
	QPlainTextEdit *plain = nullptr;
	QWidget *input;
	if (allowAppend) {
		plain = new QPlainTextEdit(container);
		plain->setMaximumHeight(60);
		input = plain;
	} else {
		line = new QLineEdit(container);
		input = line;
	}
	input->setEnabled(false);

	auto syncEnabled = [input, group]() {
		int id = group->checkedId();
		input->setEnabled(id == ID_SET || id == ID_APPEND);
	};
	connect(group, QOverload<int>::of(&QButtonGroup::idClicked), this, syncEnabled);
	connect(group, QOverload<int>::of(&QButtonGroup::idClicked), this, &BulkDiveSiteEditDialog::updatePreview);
	if (line)  connect(line,  &QLineEdit::textChanged,      this, &BulkDiveSiteEditDialog::updatePreview);
	if (plain) connect(plain, &QPlainTextEdit::textChanged, this, &BulkDiveSiteEditDialog::updatePreview);

	auto *radios = new QHBoxLayout;
	radios->setContentsMargins(0, 0, 0, 0);
	radios->addWidget(rLeave);
	if (rClear)  radios->addWidget(rClear);
	radios->addWidget(rSet);
	if (rAppend) radios->addWidget(rAppend);
	radios->addStretch(1);

	grid->addWidget(lbl,    row, 0, Qt::AlignRight | Qt::AlignTop);
	grid->addLayout(radios, row, 1);
	grid->addWidget(input,  row, 2);
	return { group, line, plain, allowAppend };
}

BulkDiveSiteEditDialog::BulkDiveSiteEditDialog(const QVector<dive_site *> &sitesIn, QWidget *parent)
	: QDialog(parent), sites(sitesIn)
{
	setWindowTitle(tr("Bulk edit %n dive site(s)", "", sites.size()));
	resize(960, 720);

	auto *outer = new QVBoxLayout(this);

	auto *header = new QLabel(tr("Editing <b>%n</b> selected dive site(s).", "", sites.size()), this);
	outer->addWidget(header);

	auto *splitter = new QSplitter(Qt::Vertical, this);
	outer->addWidget(splitter, 1);

	// --- editor area (taxonomy first, then description / notes) ------------
	auto *editorScroll = new QScrollArea(splitter);
	editorScroll->setWidgetResizable(true);
	auto *editor = new QWidget(editorScroll);
	auto *grid = new QGridLayout(editor);
	grid->setColumnStretch(2, 1);
	int row = 0;

	for (enum taxonomy_category cat: taxonomy_active_categories()) {
		QString lbl = gettextFromC::tr(taxonomy_category_names[cat]);
		// AI-generated (Claude): taxonomy exposes Leave / Clear / Set (no Append).
		// Set takes a free-text string.
		FieldRow r = makeRow(lbl, /*clear*/ true, /*append*/ false, editor, row++);
		taxonomyRows.push_back({ cat, r });
	}

	auto *sep = new QFrame(editor);
	sep->setFrameShape(QFrame::HLine);
	grid->addWidget(sep, row++, 0, 1, 3);

	descriptionRow = makeRow(tr("Description"), /*clear*/ true, /*append*/ true, editor, row++);
	notesRow       = makeRow(tr("Notes"),       /*clear*/ true, /*append*/ true, editor, row++);

	grid->setRowStretch(row, 1);
	editorScroll->setWidget(editor);
	splitter->addWidget(editorScroll);

	// --- preview table -----------------------------------------------------
	auto *previewBox = new QGroupBox(tr("Preview"), splitter);
	auto *previewLayout = new QVBoxLayout(previewBox);
	preview = new QTableWidget(previewBox);
	preview->setEditTriggers(QAbstractItemView::NoEditTriggers);
	preview->setSelectionMode(QAbstractItemView::SingleSelection);
	preview->setSelectionBehavior(QAbstractItemView::SelectRows);
	preview->setAlternatingRowColors(true);
	preview->verticalHeader()->setVisible(false);
	preview->horizontalHeader()->setStretchLastSection(true);
	previewLayout->addWidget(preview);
	splitter->addWidget(previewBox);
	splitter->setStretchFactor(0, 2);
	splitter->setStretchFactor(1, 3);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	outer->addWidget(buttons);

	updatePreview();
}

Command::BulkDiveSiteEdit BulkDiveSiteEditDialog::edit() const
{
	Command::BulkDiveSiteEdit e;
	e.descriptionMode  = modeFromId(descriptionRow.modes->checkedId());
	e.descriptionValue = descriptionRow.textInput ? descriptionRow.textInput->toPlainText() : QString();
	e.notesMode        = modeFromId(notesRow.modes->checkedId());
	e.notesValue       = notesRow.textInput ? notesRow.textInput->toPlainText() : QString();
	for (const auto &[cat, r]: taxonomyRows) {
		auto mode = modeFromId(r.modes->checkedId());
		if (mode == Command::BulkDiveSiteEdit::Mode::LeaveUnchanged)
			continue;
		Command::BulkDiveSiteEdit::TaxonomyEdit te;
		te.mode = mode;
		if (mode == Command::BulkDiveSiteEdit::Mode::Set && r.input)
			te.value = r.input->text().trimmed().toStdString();
		e.taxonomy[static_cast<int>(cat)] = std::move(te);
	}
	return e;
}

static QString applyText(Command::BulkDiveSiteEdit::Mode mode, const std::string &cur, const QString &val)
{
	using M = Command::BulkDiveSiteEdit::Mode;
	switch (mode) {
	case M::LeaveUnchanged: return QString::fromStdString(cur);
	case M::Clear:          return {};
	case M::Set:            return val;
	case M::Append: {
		QString out = QString::fromStdString(cur);
		if (!out.isEmpty()) out += "\n";
		out += val;
		return out;
	}}
	return QString::fromStdString(cur);
}

// Flatten newlines so preview rows stay single-line.
static QString flat(const QString &s)
{
	QString out = s;
	return out.replace('\n', QStringLiteral(" ⏎ "));
}

void BulkDiveSiteEditDialog::updatePreview()
{
	using M = Command::BulkDiveSiteEdit::Mode;
	Command::BulkDiveSiteEdit e = edit();

	// Build the column list: Name, then any field the user is changing.
	// Description / Notes appear after the taxonomy columns to mirror the
	// editor layout.
	struct Col {
		QString header;
		std::function<QString(const dive_site &)> oldVal;
		std::function<QString(const dive_site &)> newVal;
	};
	std::vector<Col> cols;
	cols.push_back({ tr("Name"),
		[](const dive_site &ds) { return QString::fromStdString(ds.name); },
		[](const dive_site &)   { return QString(); } });

	for (const auto &[catInt, te]: e.taxonomy) {
		auto cat = static_cast<enum taxonomy_category>(catInt);
		QString label = gettextFromC::tr(taxonomy_category_names[cat]);
		auto modeCopy = te.mode;
		auto valueCopy = QString::fromStdString(te.value);
		cols.push_back({ label,
			[cat](const dive_site &ds) {
				return QString::fromStdString(taxonomy_get_value(ds.taxonomy, cat));
			},
			[cat, modeCopy, valueCopy](const dive_site &ds) -> QString {
				if (modeCopy == M::Clear) return {};
				if (modeCopy == M::Set)   return valueCopy;
				return QString::fromStdString(taxonomy_get_value(ds.taxonomy, cat));
			}});
	}
	if (e.descriptionMode != M::LeaveUnchanged) {
		auto modeCopy = e.descriptionMode, descMode = modeCopy;
		auto valueCopy = e.descriptionValue;
		cols.push_back({ tr("Description"),
			[](const dive_site &ds) { return flat(QString::fromStdString(ds.description)); },
			[descMode, valueCopy](const dive_site &ds) {
				return flat(applyText(descMode, ds.description, valueCopy));
			}});
	}
	if (e.notesMode != M::LeaveUnchanged) {
		auto modeCopy = e.notesMode;
		auto valueCopy = e.notesValue;
		cols.push_back({ tr("Notes"),
			[](const dive_site &ds) { return flat(QString::fromStdString(ds.notes)); },
			[modeCopy, valueCopy](const dive_site &ds) {
				return flat(applyText(modeCopy, ds.notes, valueCopy));
			}});
	}

	preview->clear();
	preview->setColumnCount(static_cast<int>(cols.size()));
	preview->setRowCount(sites.size());

	QStringList headers;
	headers.reserve(static_cast<int>(cols.size()));
	headers << cols[0].header;
	for (size_t i = 1; i < cols.size(); ++i)
		headers << tr("%1: old → new").arg(cols[i].header);
	preview->setHorizontalHeaderLabels(headers);

	for (int r = 0; r < sites.size(); ++r) {
		const dive_site *ds = sites[r];
		if (!ds) continue;
		// Name column
		preview->setItem(r, 0, new QTableWidgetItem(cols[0].oldVal(*ds)));
		// Change columns: "old  →  new"
		for (size_t c = 1; c < cols.size(); ++c) {
			QString o = cols[c].oldVal(*ds);
			QString n = cols[c].newVal(*ds);
			QString cell;
			if (o.isEmpty() && n.isEmpty())
				cell = QStringLiteral("(empty)");
			else
				cell = QStringLiteral("%1  →  %2")
				         .arg(o.isEmpty() ? QStringLiteral("(empty)") : o,
				              n.isEmpty() ? QStringLiteral("(empty)") : n);
			auto *item = new QTableWidgetItem(cell);
			if (o != n) {
				QFont f = item->font();
				f.setBold(true);
				item->setFont(f);
			}
			preview->setItem(r, static_cast<int>(c), item);
		}
	}
	preview->resizeColumnsToContents();
	if (preview->columnCount() > 0)
		preview->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
}
