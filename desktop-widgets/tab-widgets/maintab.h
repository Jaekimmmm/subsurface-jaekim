// SPDX-License-Identifier: GPL-2.0
/*
 * maintab.h
 *
 * header file for the main tab of Subsurface
 *
 */
#ifndef MAINTAB_H
#define MAINTAB_H

#include <QTabWidget>

#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"

class TabBase;
class MainTab : public QTabWidget {
	Q_OBJECT
public:
	MainTab(QWidget *parent = 0);
	void clearTabs();
	void nextInputField(QKeyEvent *event);
	void stealFocus();
	bool includesCurrentDive(const QVector<dive *> &dives) const;
	divecomputer *getCurrentDC() const;
	// AI-generated (Claude)
	// Switch to the Media tab and select/scroll-to the given photo.
	void focusPicture(const QString &fileUrl);

	dive *currentDive;
	int currentDC;
signals:
	// AI-generated (Claude)
	// Re-emitted from TabDivePhotos so MainWindow can drive the
	// in-profile media infobox without poking into extraWidgets.
	void pictureFocused(const QString &fileUrl);
public
slots:
	// Always called with non-null currentDive
	void updateDiveInfo(const std::vector<dive *> &selection, dive *currentDive, int currentDC);
	void settingsChanged();
	void escDetected();
	void colorsChanged();
private:
	bool lastSelectedDive;
	int lastTabSelectedDive;
	int lastTabSelectedDiveTrip;
	QList<TabBase*> extraWidgets;
	void changeEvent(QEvent *ev) override;
	bool isDark;
};

#endif // MAINTAB_H
