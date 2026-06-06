// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "profile-widget/mediainfoboxitem.h"

#include <QBrush>
#include <QColor>
#include <QPen>

namespace {
constexpr double kCornerRadius = 6.0;
constexpr double kPadX = 8.0;
constexpr double kPadY = 6.0;
constexpr double kLineGap = 2.0;
} // namespace

MediaInfoBoxItem::MediaInfoBoxItem(QGraphicsItem *parent) :
	RoundRectItem(kCornerRadius, parent),
	timeItem(new QGraphicsSimpleTextItem(this)),
	depthItem(new QGraphicsSimpleTextItem(this)),
	tempItem(new QGraphicsSimpleTextItem(this))
{
	// Match the existing infobox look: semi-transparent off-white background.
	setPen(QPen(QColor(0, 0, 0, 160), 1));
	setBrush(QBrush(QColor(255, 255, 255, 220)));
	setZValue(9997); // just below ToolTipItem (9998)

	for (auto *item : { timeItem, depthItem, tempItem })
		item->setBrush(QBrush(Qt::black));
}

void MediaInfoBoxItem::setLines(const QString &time, const QString &depth, const QString &temperature)
{
	timeItem->setText(time);
	depthItem->setText(depth);
	tempItem->setText(temperature);
	layoutText();
}

void MediaInfoBoxItem::layoutText()
{
	double textW = std::max({ timeItem->boundingRect().width(),
				 depthItem->boundingRect().width(),
				 tempItem->boundingRect().width() });
	double lineH = timeItem->boundingRect().height();
	double totalH = lineH * 3 + kLineGap * 2;

	double w = textW + kPadX * 2;
	double h = totalH + kPadY * 2;

	QPointF anchorBL = rect().bottomLeft();
	if (rect().isEmpty())
		anchorBL = pos(); // first layout
	setRect(anchorBL.x(), anchorBL.y() - h, w, h);

	QRectF r = rect();
	double y = r.top() + kPadY;
	timeItem->setPos(r.left() + kPadX, y);
	y += lineH + kLineGap;
	depthItem->setPos(r.left() + kPadX, y);
	y += lineH + kLineGap;
	tempItem->setPos(r.left() + kPadX, y);
}

void MediaInfoBoxItem::anchorBottomLeft(const QPointF &p)
{
	QRectF r = rect();
	if (r.isEmpty()) {
		setPos(p);
		layoutText();
		return;
	}
	setRect(p.x(), p.y() - r.height(), r.width(), r.height());
	layoutText();
}
