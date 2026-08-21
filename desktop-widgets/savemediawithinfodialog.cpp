// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "desktop-widgets/savemediawithinfodialog.h"

#include "core/dive.h"
#include "core/divecomputer.h"
#include "core/divesite.h"
#include "core/gettextfromc.h"
#include "core/pref.h"
#include "core/qthelper.h"
#include "core/sample.h"
#include "core/string-format.h"
#include "core/taxonomy.h"
#include "core/units.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QLayout>
#include <QPlainTextEdit>
#include <QTimer>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSettings>
#include <QSpinBox>
#include <QTextCursor>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QString colorSwatchStyle(const QColor &c)
{
	return QString("background:%1; border: 1px solid #888; min-width: 32px; min-height: 18px;")
		.arg(c.name(QColor::HexArgb));
}

QString formatElapsed(int s)
{
	s = std::max(s, 0);
	int h = s / 3600, m = (s % 3600) / 60, sec = s % 60;
	if (h > 0)
		return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
	return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
}

// AI-generated (Claude)
// User-visible name of a taxonomy category, lowercased with spaces turned
// into underscores so it makes a safe `{placeholder}` token. Matches the
// label shown on the corresponding button in the dialog.
QString displayPlaceholder(enum taxonomy_category c)
{
	QString name = gettextFromC::tr(taxonomy_category_names[c]);
	name = name.toLower();
	name.replace(' ', '_');
	return name;
}

// AI-generated (Claude)
// Stable internal placeholder for a taxonomy category — used as a fall-
// back key in the resolver so templates written before this change keep
// working.
QString internalPlaceholder(enum taxonomy_category c)
{
	switch (c) {
	case TC_OCEAN:       return "ocean";
	case TC_COUNTRY:     return "country";
	case TC_ADMIN_L1:    return "admin_l1";
	case TC_ADMIN_L2:    return "admin_l2";
	case TC_LOCALNAME:   return "localname";
	case TC_ADMIN_L3:    return "admin_l3";
	case TC_DIVE_REGION: return "region";
	case TC_DIVE_POINT:  return "point";
	default:             return "";
	}
}

} // namespace

SaveMediaWithInfoDialog::SaveMediaWithInfoDialog(const dive *dIn, int dcNrIn,
						 const QStringList &photosIn,
						 const QList<int> &offsetsIn,
						 QWidget *parent) :
	QDialog(parent), d(dIn), dcNr(dcNrIn), photos(photosIn), offsets(offsetsIn)
{
	setWindowTitle(tr("Save media with dive info"));
	resize(900, 600);
	loadSettings();
	buildUi();
	// AI-generated (Claude)
	// Defer the (potentially slow) image read until after the dialog is
	// painted, so the user sees the UI immediately with a "Loading…" hint.
}

// AI-generated (Claude)
QString SaveMediaWithInfoDialog::defaultTemplate() const
{
	QStringList parts;
	for (enum taxonomy_category c : taxonomy_active_categories()) {
		QString p = displayPlaceholder(c);
		if (!p.isEmpty())
			parts << QString("{%1}").arg(p);
	}
	QString lines =
		"Time : {date} {time} (+{elapsed})\n"
		"Depth : {depth}\n"
		"Temp. : {temp}\n";
	if (!parts.isEmpty())
		lines += QString("Dive point : %1\n").arg(parts.join(", "));
	lines += "Entry GPS : {gps}";
	return lines;
}

void SaveMediaWithInfoDialog::loadSettings()
{
	QSettings st;
	st.beginGroup("SaveMediaWithInfo");
	s.templateText   = st.value("templateText", defaultTemplate()).toString();
	s.font           = st.value("font", QFont("Helvetica")).value<QFont>();
	s.fontSize       = st.value("fontSize",   s.fontSize).toInt();
	s.textColor      = st.value("textColor",  s.textColor).value<QColor>();
	s.bgColor        = st.value("bgColor",    s.bgColor).value<QColor>();
	s.position       = st.value("position",   s.position).toInt();
	s.padding        = st.value("padding",    s.padding).toInt();
	s.margin         = st.value("margin",     s.margin).toInt();
	s.outputDir      = st.value("outputDir",  s.outputDir).toString();
	s.postfix        = st.value("postfix",    s.postfix).toString();
	st.endGroup();
}

void SaveMediaWithInfoDialog::saveSettings() const
{
	QSettings st;
	st.beginGroup("SaveMediaWithInfo");
	st.setValue("templateText",   s.templateText);
	st.setValue("font",           s.font);
	st.setValue("fontSize",       s.fontSize);
	st.setValue("textColor",      s.textColor);
	st.setValue("bgColor",        s.bgColor);
	st.setValue("position",       s.position);
	st.setValue("padding",        s.padding);
	st.setValue("margin",         s.margin);
	st.setValue("outputDir",      s.outputDir);
	st.setValue("postfix",        s.postfix);
	st.endGroup();
}

// AI-generated (Claude)
// Build a compact pill-shaped token button. Sized to a single text line so
// rows stay tidy when the dialog is resized.
static QToolButton *makeTokenButton(const QString &label, const QString &token)
{
	auto *b = new QToolButton();
	b->setText(label);
	b->setToolTip(QString("{%1}").arg(token));
	b->setAutoRaise(false);
	b->setCursor(Qt::PointingHandCursor);
	b->setProperty("token", token);

	// Fixed sizing: never grows/shrinks with the layout, so the pill keeps
	// its shape regardless of dialog size.
	QFontMetrics fm(b->font());
	const int h = fm.height() + 4;          // one text line + tiny vertical padding
	b->setFixedHeight(h);
	const int textW = fm.horizontalAdvance(label);
	b->setMinimumWidth(textW + 14);          // 7px horizontal padding each side
	b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	b->setStyleSheet(QString(
		"QToolButton {"
		" background: #e6eef8; color: #1a3d6b; border: 1px solid #aac2dd;"
		" border-radius: %1px; padding: 0 7px; margin: 0; }"
		"QToolButton:hover { background: #d0e0f3; }"
		"QToolButton:pressed { background: #b8d2eb; }").arg(h / 2));
	return b;
}

void SaveMediaWithInfoDialog::buildUi()
{
	auto *root = new QHBoxLayout(this);
	root->setContentsMargins(8, 8, 8, 8);
	root->setSpacing(8);

	// ---- LEFT: controls ----
	auto *left = new QVBoxLayout();
	left->setSpacing(4);
	left->setContentsMargins(0, 0, 0, 0);

	auto *templateBox = new QGroupBox(tr("Info template"));
	auto *templateLayout = new QVBoxLayout(templateBox);
	templateLayout->setContentsMargins(8, 8, 8, 8);
	templateLayout->setSpacing(4);
	templateEdit = new QPlainTextEdit();
	templateEdit->setPlainText(s.templateText);
	templateEdit->setTabChangesFocus(true);
	// AI-generated (Claude) — inherit dialog font (no monospace override)
	templateEdit->setMinimumHeight(90);
	auto *templateTopRow = new QHBoxLayout();
	templateTopRow->setContentsMargins(0, 0, 0, 0);
	templateTopRow->setSpacing(4);
	auto *templateHint = new QLabel(tr("Click a tag below to insert or edit a field name directly."));
	templateHint->setStyleSheet("color: #666;");
	auto *resetBtn = new QPushButton(tr("Reset to default"));
	resetBtn->setAutoDefault(false);
	resetBtn->setDefault(false);
	templateTopRow->addWidget(templateHint, 1);
	templateTopRow->addWidget(resetBtn);
	templateLayout->addLayout(templateTopRow);
	templateLayout->addWidget(templateEdit);

	// Token groups
	struct TokenDef { QString label; QString token; };
	struct TokenGroup { QString title; QList<TokenDef> tokens; };
	QList<TokenGroup> groups = {
		{ tr("Time / Date"), {
			{"date","date"}, {"time","time"}, {"elapsed","elapsed"}
		} },
		{ tr("Measurements"), {
			{"depth","depth"}, {"temp","temp"}
		} },
		{ tr("Dive"), {
			{"dive #","dive_number"}, {"site name","site"}
		} },
	};
	// AI-generated (Claude)
	// Location hierarchy follows Preferences > Georeference. Both the
	// button label and the inserted placeholder match the user-visible
	// category name (e.g. "State" inserts {state}). resolveTemplate also
	// accepts the legacy internal names ({admin_l1}, etc.) so old
	// templates continue to work.
	QList<TokenDef> locTokens;
	for (enum taxonomy_category c : taxonomy_active_categories()) {
		QString placeholder = displayPlaceholder(c);
		if (placeholder.isEmpty())
			continue;
		QString label = gettextFromC::tr(taxonomy_category_names[c]);
		if (label.isEmpty())
			label = placeholder;
		locTokens.append({ label, placeholder });
	}
	if (!locTokens.isEmpty())
		groups.append({ tr("Location hierarchy"), locTokens });
	groups.append({ tr("GPS"), { { "gps", "gps" } } });

	// AI-generated (Claude)
	// Reliable wrapping: pack each group's title + tokens into one or
	// more fixed QHBoxLayout rows (max N tokens per row). No custom
	// flow layout — straight stacked horizontal rows can't overlap
	// regardless of the dialog width.
	const int maxPerRow = 5;
	for (const TokenGroup &g : groups) {
		QList<QWidget *> rowItems;
		auto *titleLabel = new QLabel(QString("<b>%1:</b>").arg(g.title));
		titleLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		rowItems << titleLabel;
		for (const TokenDef &t : g.tokens) {
			auto *b = makeTokenButton(t.label, t.token);
			connect(b, &QToolButton::clicked, this,
				[this, tok = QString(t.token)]() { insertToken(tok); });
			rowItems << b;
		}
		// Split into rows of up to (1 title + maxPerRow tokens) on the
		// first row, then maxPerRow tokens on follow-up rows.
		int idx = 0;
		bool first = true;
		while (idx < rowItems.size()) {
			auto *row = new QHBoxLayout();
			row->setContentsMargins(0, 0, 0, 0);
			row->setSpacing(4);
			int take = first ? (1 + maxPerRow) : maxPerRow;
			for (int j = 0; j < take && idx < rowItems.size(); ++j, ++idx)
				row->addWidget(rowItems[idx]);
			row->addStretch(1);
			templateLayout->addLayout(row);
			first = false;
		}
	}
	left->addWidget(templateBox);

	connect(resetBtn, &QPushButton::clicked, this, &SaveMediaWithInfoDialog::resetTemplate);
	connect(templateEdit, &QPlainTextEdit::textChanged, this,
		&SaveMediaWithInfoDialog::renderPreview);

	auto *typoBox = new QGroupBox(tr("Typography"));
	auto *typoForm = new QFormLayout(typoBox);
	typoForm->setContentsMargins(8, 6, 8, 6);
	typoForm->setVerticalSpacing(3);
	typoForm->setHorizontalSpacing(6);
	fontCombo = new QFontComboBox();
	fontCombo->setCurrentFont(s.font);
	fontSizeSpin = new QSpinBox();
	fontSizeSpin->setRange(6, 200);
	fontSizeSpin->setValue(s.fontSize);
	textColorBtn = new QToolButton();
	textColorBtn->setStyleSheet(colorSwatchStyle(s.textColor));
	bgColorBtn = new QToolButton();
	bgColorBtn->setStyleSheet(colorSwatchStyle(s.bgColor));
	typoForm->addRow(tr("Font"), fontCombo);
	typoForm->addRow(tr("Size"), fontSizeSpin);
	typoForm->addRow(tr("Text color"), textColorBtn);
	typoForm->addRow(tr("Background color"), bgColorBtn);
	left->addWidget(typoBox);

	auto *layoutBox = new QGroupBox(tr("Layout"));
	auto *layoutForm = new QFormLayout(layoutBox);
	layoutForm->setContentsMargins(8, 6, 8, 6);
	layoutForm->setVerticalSpacing(3);
	layoutForm->setHorizontalSpacing(6);
	positionCombo = new QComboBox();
	positionCombo->addItem(tr("Top-Left"));
	positionCombo->addItem(tr("Top-Right"));
	positionCombo->addItem(tr("Bottom-Left"));
	positionCombo->addItem(tr("Bottom-Right"));
	positionCombo->setCurrentIndex(s.position);
	paddingSpin = new QSpinBox();
	paddingSpin->setRange(0, 64);
	paddingSpin->setValue(s.padding);
	marginSpin = new QSpinBox();
	marginSpin->setRange(0, 200);
	marginSpin->setValue(s.margin);
	layoutForm->addRow(tr("Position"), positionCombo);
	layoutForm->addRow(tr("Box padding"), paddingSpin);
	layoutForm->addRow(tr("Margin from edge"), marginSpin);
	left->addWidget(layoutBox);

	auto *outBox = new QGroupBox(tr("Output"));
	auto *outForm = new QFormLayout(outBox);
	outForm->setContentsMargins(8, 6, 8, 6);
	outForm->setVerticalSpacing(3);
	outForm->setHorizontalSpacing(6);
	outputDirEdit = new QLineEdit(s.outputDir);
	outputDirEdit->setPlaceholderText(tr("(same folder as source)"));
	outputDirBtn = new QPushButton(tr("Browse…"));
	outputDirBtn->setAutoDefault(false);
	outputDirBtn->setDefault(false);
	auto *outRow = new QHBoxLayout();
	outRow->setContentsMargins(0, 0, 0, 0);
	outRow->setSpacing(4);
	outRow->addWidget(outputDirEdit, 1);
	outRow->addWidget(outputDirBtn);
	auto *outRowW = new QWidget();
	outRowW->setLayout(outRow);
	postfixEdit = new QLineEdit(s.postfix);
	postfixEdit->setPlaceholderText("_info");
	outForm->addRow(tr("Output folder"), outRowW);
	outForm->addRow(tr("Filename postfix"), postfixEdit);
	left->addWidget(outBox);

	left->addStretch(1);

	auto *btns = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
	// AI-generated (Claude)
	// Prevent Enter in a spin box / line edit from triggering Save —
	// the user must click the button explicitly.
	if (auto *saveBtn = btns->button(QDialogButtonBox::Save)) {
		saveBtn->setAutoDefault(false);
		saveBtn->setDefault(false);
	}
	if (auto *cancelBtn = btns->button(QDialogButtonBox::Cancel)) {
		cancelBtn->setAutoDefault(false);
		cancelBtn->setDefault(false);
	}
	left->addWidget(btns);

	root->addLayout(left, 0);

	// ---- RIGHT: preview ----
	// AI-generated (Claude)
	// Preview shows only the FIRST selected photo. Loading every photo
	// up front made the dialog slow to appear on large RAW/JPEG sets.
	auto *right = new QVBoxLayout();
	auto *previewBox = new QGroupBox(tr("Preview"));
	auto *previewLayout = new QVBoxLayout(previewBox);
	previewLayout->setContentsMargins(8, 6, 8, 6);
	previewLayout->setSpacing(4);
	auto *firstName = new QLabel(
		photos.isEmpty() ? QString() : QFileInfo(photos.first()).fileName());
	firstName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	previewLayout->addWidget(firstName);
	previewLabel = new QLabel();
	previewLabel->setAlignment(Qt::AlignCenter);
	previewLabel->setMinimumSize(400, 300);
	previewLabel->setStyleSheet("background: #222; color: #ccc;");
	previewLabel->setText(tr("Loading image…"));
	previewLayout->addWidget(previewLabel, 1);
	right->addWidget(previewBox, 1);

	root->addLayout(right, 1);

	// ---- Wire signals ----
	connect(fontCombo,   &QFontComboBox::currentFontChanged, this, [this](const QFont &) { renderPreview(); });
	connect(fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { renderPreview(); });
	connect(positionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { renderPreview(); });
	connect(paddingSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { renderPreview(); });
	connect(marginSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { renderPreview(); });
	connect(textColorBtn, &QToolButton::clicked, this, &SaveMediaWithInfoDialog::pickTextColor);
	connect(bgColorBtn,   &QToolButton::clicked, this, &SaveMediaWithInfoDialog::pickBgColor);
	connect(outputDirBtn, &QPushButton::clicked, this, &SaveMediaWithInfoDialog::pickOutputDir);
	connect(btns, &QDialogButtonBox::accepted, this, &SaveMediaWithInfoDialog::onSave);
	connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SaveMediaWithInfoDialog::pickTextColor()
{
	QColor c = QColorDialog::getColor(s.textColor, this, tr("Text color"),
					   QColorDialog::ShowAlphaChannel);
	if (c.isValid()) {
		s.textColor = c;
		textColorBtn->setStyleSheet(colorSwatchStyle(s.textColor));
		renderPreview();
	}
}

void SaveMediaWithInfoDialog::pickBgColor()
{
	QColor c = QColorDialog::getColor(s.bgColor, this, tr("Background color"),
					   QColorDialog::ShowAlphaChannel);
	if (c.isValid()) {
		s.bgColor = c;
		bgColorBtn->setStyleSheet(colorSwatchStyle(s.bgColor));
		renderPreview();
	}
}

void SaveMediaWithInfoDialog::pickOutputDir()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Output folder"),
							 s.outputDir);
	if (!dir.isEmpty()) {
		s.outputDir = dir;
		outputDirEdit->setText(dir);
	}
}

// AI-generated (Claude)
// Resolve {token} placeholders in the template against the current dive
// and picture offset. Unknown tokens are left as-is so the user can spot
// typos. Empty values become empty strings.
QString SaveMediaWithInfoDialog::resolveTemplate(int offsetSec) const
{
	QString text = s.templateText;
	if (!d)
		return text;

	QDateTime taken = timestampToDateTime(d->when + offsetSec);
	QString dateFmt = QString::fromStdString(prefs.date_format);
	QString timeFmt = QString::fromStdString(prefs.time_format);
	if (dateFmt.isEmpty()) dateFmt = "yyyy-MM-dd";
	if (timeFmt.isEmpty()) timeFmt = "HH:mm:ss";

	const struct divecomputer *dcomp = d->get_dc(dcNr);
	depth_t depth = { .mm = get_depth_at_time(dcomp, offsetSec) };
	int tempMk = 0;
	if (dcomp) {
		for (const auto &smp : dcomp->samples) {
			if (smp.time.seconds > offsetSec)
				break;
			if (smp.temperature.mkelvin)
				tempMk = smp.temperature.mkelvin;
		}
	}
	QString tempStr;
	if (tempMk > 0) {
		temperature_t t = { .mkelvin = (uint32_t)tempMk };
		tempStr = get_temperature_string(t, true);
	}

	auto taxValue = [&](enum taxonomy_category cat) -> QString {
		if (!d->dive_site)
			return QString();
		return QString::fromStdString(taxonomy_get_value(d->dive_site->taxonomy, cat));
	};

	QString gpsStr;
	if (d->dive_site && d->dive_site->has_gps_location())
		gpsStr = printGPSCoords(&d->dive_site->location);

	QMap<QString, QString> tokens = {
		{ "{date}",         getLocale().toString(taken.toUTC().date(), dateFmt) },
		{ "{time}",         getLocale().toString(taken.toUTC().time(), timeFmt) },
		{ "{elapsed}",      formatElapsed(offsetSec) },
		{ "{depth}",        get_depth_string(depth, true, true) },
		{ "{temp}",         tempStr },
		{ "{dive_number}",  d->number > 0 ? QString::number(d->number) : QString() },
		{ "{site}",         d->dive_site ? QString::fromStdString(d->dive_site->name) : QString() },
		{ "{gps}",          gpsStr },
	};
	// AI-generated (Claude)
	// For each taxonomy category, register both the internal placeholder
	// ({admin_l1}) AND the user-visible name ({state}). This way the
	// dialog can insert the display name while older templates with the
	// internal name continue to resolve.
	for (int i = 0; i < TC_NR_CATEGORIES; ++i) {
		auto cat = static_cast<enum taxonomy_category>(i);
		QString internal = internalPlaceholder(cat);
		if (internal.isEmpty())
			continue;
		QString value = taxValue(cat);
		tokens.insert(QString("{%1}").arg(internal), value);
		QString display = displayPlaceholder(cat);
		if (!display.isEmpty() && display != internal)
			tokens.insert(QString("{%1}").arg(display), value);
	}
	for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it)
		text.replace(it.key(), it.value());
	return text;
}

QStringList SaveMediaWithInfoDialog::buildLines(int offsetSec) const
{
	const QString resolved = resolveTemplate(offsetSec);
	// Split into lines; drop trailing empty lines only.
	QStringList lines = resolved.split('\n');
	while (!lines.isEmpty() && lines.last().trimmed().isEmpty())
		lines.removeLast();
	return lines;
}

// AI-generated (Claude)
void SaveMediaWithInfoDialog::resetTemplate()
{
	templateEdit->setPlainText(defaultTemplate());
}

// AI-generated (Claude)
void SaveMediaWithInfoDialog::insertToken(const QString &token)
{
	templateEdit->insertPlainText(QString("{%1}").arg(token));
	templateEdit->setFocus();
}

QImage SaveMediaWithInfoDialog::renderOverlay(const QImage &src, int offsetSec) const
{
	if (src.isNull())
		return QImage();

	QImage out = src.convertToFormat(QImage::Format_ARGB32);
	QStringList lines = buildLines(offsetSec);
	if (lines.isEmpty())
		return out;

	QPainter p(&out);
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::TextAntialiasing);

	QFont f = fontCombo->currentFont();
	f.setPixelSize(fontSizeSpin->value());
	p.setFont(f);
	QFontMetrics fm(f);

	int textW = 0, textH = 0;
	const int lineSpacing = 2;
	for (const QString &line : lines) {
		QRect r = fm.boundingRect(line);
		textW = std::max(textW, r.width());
		textH += fm.height();
	}
	if (lines.size() > 1)
		textH += lineSpacing * (lines.size() - 1);

	int pad = paddingSpin->value();
	int mar = marginSpin->value();
	int boxW = textW + pad * 2;
	int boxH = textH + pad * 2;

	int pos = positionCombo->currentIndex();
	int x, y;
	switch (pos) {
	case 0: x = mar;                       y = mar;                       break; // TL
	case 1: x = out.width() - boxW - mar;  y = mar;                       break; // TR
	case 2: x = mar;                       y = out.height() - boxH - mar; break; // BL
	default:x = out.width() - boxW - mar;  y = out.height() - boxH - mar; break; // BR
	}

	p.fillRect(QRect(x, y, boxW, boxH), s.bgColor);

	p.setPen(s.textColor);
	int ty = y + pad + fm.ascent();
	for (const QString &line : lines) {
		p.drawText(x + pad, ty, line);
		ty += fm.height() + lineSpacing;
	}

	return out;
}

// AI-generated (Claude)
// Once the dialog is visible, kick off the deferred image load (if not
// already done). renderPreview itself only paints what's currently in
// cachedPreviewImage so the call here is cheap.
void SaveMediaWithInfoDialog::showEvent(QShowEvent *e)
{
	QDialog::showEvent(e);
	loadPreviewImageAsync();
	renderPreview();
}

// AI-generated (Claude)
void SaveMediaWithInfoDialog::resizeEvent(QResizeEvent *e)
{
	QDialog::resizeEvent(e);
	renderPreview();
}

// AI-generated (Claude)
// Queue a single-shot lambda that loads the first photo on the main
// thread *after* the dialog has been painted. QImage::load is blocking
// but typically fast for JPEG; doing it on the GUI thread is acceptable
// once the user has seen the loading hint.
void SaveMediaWithInfoDialog::loadPreviewImageAsync()
{
	if (previewLoadQueued || !cachedPreviewImage.isNull() || photos.isEmpty())
		return;
	previewLoadQueued = true;
	QString path = photos.first();
	QTimer::singleShot(0, this, [this, path]() {
		cachedPreviewImage = QImage(path);
		previewLoadAttempted = true;
		renderPreview();
	});
}

void SaveMediaWithInfoDialog::renderPreview()
{
	// AI-generated (Claude)
	// Keep s.templateText in sync with the editor so resolveTemplate sees
	// the current text on every keystroke / token insert.
	if (templateEdit)
		s.templateText = templateEdit->toPlainText();

	// AI-generated (Claude)
	// Use the cached first-photo image (loaded asynchronously in
	// loadPreviewImageAsync). Until the load finishes, leave whatever is
	// in previewLabel (typically the "Loading image…" hint set by
	// buildUi).
	if (photos.isEmpty())
		return;
	if (cachedPreviewImage.isNull()) {
		if (!previewLoadAttempted)
			return; // still loading — keep the "Loading image…" hint
		previewLabel->setText(tr("Cannot load: %1").arg(photos.first()));
		return;
	}
	int offsetSec = offsets.value(0, 0);
	QImage out = renderOverlay(cachedPreviewImage, offsetSec);

	QSize target = previewLabel->size();
	if (target.width() < 10 || target.height() < 10)
		target = QSize(600, 400);
	QPixmap pm = QPixmap::fromImage(out).scaled(target,
						    Qt::KeepAspectRatio,
						    Qt::SmoothTransformation);
	previewLabel->setPixmap(pm);
}

void SaveMediaWithInfoDialog::onSave()
{
	// Sync UI state into settings struct.
	s.templateText   = templateEdit->toPlainText();
	s.font           = fontCombo->currentFont();
	s.fontSize       = fontSizeSpin->value();
	s.position       = positionCombo->currentIndex();
	s.padding        = paddingSpin->value();
	s.margin         = marginSpin->value();
	s.outputDir      = outputDirEdit->text();
	s.postfix        = postfixEdit->text().isEmpty() ? QString("_info") : postfixEdit->text();
	saveSettings();

	int written = 0;
	QStringList failures;
	for (int i = 0; i < photos.size(); ++i) {
		QString path = photos[i];
		int offsetSec = offsets.value(i, 0);
		QImage src(path);
		if (src.isNull()) {
			failures << path;
			continue;
		}
		QImage out = renderOverlay(src, offsetSec);
		QFileInfo fi(path);
		QString dir = s.outputDir.isEmpty() ? fi.absolutePath() : s.outputDir;
		QString outPath = QString("%1/%2%3.%4")
					.arg(dir, fi.completeBaseName(),
					     s.postfix, fi.suffix().isEmpty() ? "jpg" : fi.suffix());
		if (!out.save(outPath))
			failures << outPath;
		else
			++written;
	}

	QString msg = tr("Saved %1 of %2 files.").arg(written).arg(photos.size());
	if (!failures.isEmpty())
		msg += "\n" + tr("Failed:") + "\n" + failures.join("\n");
	QMessageBox::information(this, tr("Save media with dive info"), msg);
	accept();
}
