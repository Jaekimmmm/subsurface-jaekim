// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#ifndef SAVEMEDIAWITHINFODIALOG_H
#define SAVEMEDIAWITHINFODIALOG_H

#include <QColor>
#include <QDialog>
#include <QFont>
#include <QImage>
#include <QString>
#include <QStringList>

struct dive;

class QComboBox;
class QFontComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QToolButton;

class SaveMediaWithInfoDialog : public QDialog {
	Q_OBJECT
public:
	// Photos: list of (local file path, offset in seconds).
	SaveMediaWithInfoDialog(const dive *d, int dcNr,
				const QStringList &photos,
				const QList<int> &offsets,
				QWidget *parent = nullptr);

protected:
	// AI-generated (Claude)
	void showEvent(QShowEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;

private slots:
	void renderPreview();
	void pickTextColor();
	void pickBgColor();
	void pickOutputDir();
	void onSave();
	// AI-generated (Claude)
	void resetTemplate();
	void insertToken(const QString &token);

private:
	struct Settings {
		QString templateText;
		QFont font;
		int fontSize = 18;
		QColor textColor = Qt::white;
		QColor bgColor = QColor(0, 0, 0, 170);
		int position = 3; // 0=TL 1=TR 2=BL 3=BR
		int padding = 8;
		int margin = 12;

		QString outputDir;
		QString postfix = "_info";
	};

	void buildUi();
	void loadSettings();
	void saveSettings() const;
	QStringList buildLines(int offsetSec) const;
	QImage renderOverlay(const QImage &src, int offsetSec) const;
	// AI-generated (Claude)
	QString resolveTemplate(int offsetSec) const;
	// Builds the default template at runtime; the Dive-point line only
	// includes categories the user has enabled in Preferences > Georeference.
	QString defaultTemplate() const;

	const dive *d;
	int dcNr;
	QStringList photos;
	QList<int> offsets;
	Settings s;

	// Widgets
	QPlainTextEdit *templateEdit;
	QFontComboBox *fontCombo;
	QSpinBox *fontSizeSpin;
	QToolButton *textColorBtn;
	QToolButton *bgColorBtn;
	QComboBox *positionCombo;
	QSpinBox *paddingSpin;
	QSpinBox *marginSpin;
	QLineEdit *outputDirEdit;
	QPushButton *outputDirBtn;
	QLineEdit *postfixEdit;
	QLabel *previewLabel;

	// AI-generated (Claude)
	// First photo only, loaded once asynchronously after show.
	QImage cachedPreviewImage;
	bool previewLoadQueued = false;
	bool previewLoadAttempted = false;
	void loadPreviewImageAsync();
};

#endif // SAVEMEDIAWITHINFODIALOG_H
