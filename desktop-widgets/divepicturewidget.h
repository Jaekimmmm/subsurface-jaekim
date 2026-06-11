// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPICTUREWIDGET_H
#define DIVEPICTUREWIDGET_H

#include <QListView>
#include <QPoint>

class DivePictureWidget : public QListView {
	Q_OBJECT
public:
	DivePictureWidget(QWidget *parent);
protected:
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	// AI-generated (Claude)
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

signals:
	void photoDoubleClicked(const QString filePath);
	void zoomLevelChanged(int delta);

private:
	// AI-generated (Claude)
	// Track the left-button press so we can defer drag until the mouse
	// actually moves past QApplication::startDragDistance().
	bool dragPending = false;
	QPoint dragStartPos;
};

#endif
