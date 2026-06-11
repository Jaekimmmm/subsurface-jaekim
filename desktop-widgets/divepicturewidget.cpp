// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/divepicturewidget.h"
#include "core/metrics.h"
#include "core/qthelper.h"
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>

DivePictureWidget::DivePictureWidget(QWidget *parent) : QListView(parent)
{
}

void DivePictureWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		QString filePath = model()->data(indexAt(event->position().toPoint()), Qt::DisplayPropertyRole).toString();
#else
		QString filePath = model()->data(indexAt(event->pos()), Qt::DisplayPropertyRole).toString();
#endif
		emit photoDoubleClicked(localFilePath(filePath));
	}
}

// AI-generated (Claude)
// Defer the drag until the mouse actually moves past the system drag
// threshold. Previously the drag-and-drop event loop started on every
// left-click, which made the click behave like a drag (and also broke
// normal selection, since QDrag::exec is blocking and runs before the
// QListView press handler).
void DivePictureWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && event->modifiers() == Qt::NoModifier) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		dragStartPos = event->position().toPoint();
#else
		dragStartPos = event->pos();
#endif
		dragPending = true;
	} else {
		dragPending = false;
	}
	QListView::mousePressEvent(event);
}

// AI-generated (Claude)
void DivePictureWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (!dragPending || !(event->buttons() & Qt::LeftButton)) {
		QListView::mouseMoveEvent(event);
		return;
	}
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QPoint pos = event->position().toPoint();
#else
	QPoint pos = event->pos();
#endif
	if ((pos - dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
		QListView::mouseMoveEvent(event);
		return;
	}

	// Threshold crossed — start the actual drag.
	dragPending = false;
	QModelIndex index = indexAt(dragStartPos);
	QString filename = model()->data(index, Qt::DisplayPropertyRole).toString();
	if (filename.isEmpty()) {
		QListView::mouseMoveEvent(event);
		return;
	}

	int dim = lrint(defaultIconMetrics().sz_pic * 0.2);
	QPixmap pixmap = model()->data(index, Qt::DecorationRole).value<QPixmap>();
	pixmap = pixmap.scaled(dim, dim, Qt::KeepAspectRatio);

	QByteArray itemData;
	QDataStream dataStream(&itemData, QIODevice::WriteOnly);
	dataStream << filename;

	QMimeData *mimeData = new QMimeData;
	mimeData->setData("application/x-subsurfaceimagedrop", itemData);

	QDrag *drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->setPixmap(pixmap);
	drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
}

// AI-generated (Claude)
void DivePictureWidget::mouseReleaseEvent(QMouseEvent *event)
{
	dragPending = false;
	QListView::mouseReleaseEvent(event);
}

void DivePictureWidget::wheelEvent(QWheelEvent *event)
{
	if (event->modifiers() == Qt::ControlModifier) {
		// Angle delta is given in eighth parts of a degree. A classical mouse
		// wheel click is 15 degrees. Each click should correspond to one zoom step.
		// Therefore, divide by 15*8=120. To also support touch pads and finer-grained
		// mouse wheels, take care to always round away from zero.
		int delta = event->angleDelta().y();
		int carry = delta > 0 ? 119 : -119;
		emit zoomLevelChanged((delta + carry) / 120);
	} else
		QListView::wheelEvent(event);
}
