// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#ifndef MEDIAINFOBOXITEM_H
#define MEDIAINFOBOXITEM_H

#include "backend-shared/roundrectitem.h"

#include <QGraphicsSimpleTextItem>
#include <QString>

// Small in-profile infobox shown at the bottom-left of the profile
// region while a thumbnail (or Media-tab row) is selected. Uses the
// same rounded rectangle as the existing ToolTipItem so it visually
// matches "Toggle infobox".
class MediaInfoBoxItem : public RoundRectItem {
public:
	explicit MediaInfoBoxItem(QGraphicsItem *parent = nullptr);
	void setLines(const QString &time, const QString &depth, const QString &temperature);
	// Anchor the bottom-left of the box at the given scene-space point,
	// inset slightly so the text doesn't touch the axis.
	void anchorBottomLeft(const QPointF &p);

private:
	void layoutText();
	QGraphicsSimpleTextItem *timeItem;
	QGraphicsSimpleTextItem *depthItem;
	QGraphicsSimpleTextItem *tempItem;
};

#endif // MEDIAINFOBOXITEM_H
