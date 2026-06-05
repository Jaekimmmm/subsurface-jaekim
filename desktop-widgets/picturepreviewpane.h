// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#ifndef PICTUREPREVIEWPANE_H
#define PICTUREPREVIEWPANE_H

#include <QPixmap>
#include <QString>
#include <QWidget>

class QLabel;
class QToolButton;

// Side-pane preview shown next to ProfileWidget when a thumbnail is
// left-clicked in ProfileMaximized view. Black background, image
// aspect-fit, and an X button in the top-right that emits closed().
// Displays an error message inside the pane when the image cannot be
// loaded from disk.
class PicturePreviewPane : public QWidget {
	Q_OBJECT
public:
	explicit PicturePreviewPane(QWidget *parent = nullptr);
	void showFor(const QString &fileUrl);
	bool isShownFor(const QString &fileUrl) const;
signals:
	void closed();
protected:
	void resizeEvent(QResizeEvent *event) override;
private:
	void rescaleImage();
	QString currentFile;
	QPixmap original;
	bool loadFailed = false;
	void updatePathLabel();
	QLabel *imageLabel;
	QLabel *errorLabel;
	QLabel *pathLabel;
	QToolButton *closeBtn;
};

#endif // PICTUREPREVIEWPANE_H
