// SPDX-License-Identifier: GPL-2.0
#ifndef PROFILEWIDGET2_H
#define PROFILEWIDGET2_H

#include <QGraphicsView>
#include <vector>
#include <memory>

// /* The idea of this widget is to display and edit the profile.
//  * It has:
//  * 1 - ToolTip / Legend item, displays every information of the current mouse position on it, plus the legends of the maps.
//  * 2 - ToolBox, displays the QActions that are used to do special stuff on the profile ( like activating the plugins. )
//  * 3 - Cartesian Axis for depth ( y )
//  * 4 - Cartesian Axis for Gases ( y )
//  * 5 - Cartesian Axis for Time  ( x )
//  *
//  * It needs to be dynamic, things should *flow* on it, not just appear / disappear.
//  */
#include "profile-widget/divelineitem.h"
#include "core/units.h"
#include "core/subsurface-qt/divelistnotifier.h"

class ProfileScene;
class RulerItem2;
struct dive;
class ToolTipItem;
class DiveEventItem;
class DivePlannerPointsModel;
class DiveHandler;
class QGraphicsSimpleTextItem;
class QModelIndex;
class DivePictureItem;
class MediaInfoBoxItem;

class ProfileWidget2 : public QGraphicsView {
	Q_OBJECT
public:
	enum State {
		PROFILE,
		EDIT,
		PLAN,
		INIT
	};

	struct RenderFlags {
		static constexpr int None = 0;
		static constexpr int Instant = 1 << 0;
		static constexpr int DontRecalculatePlotInfo = 1 << 1;
	};

	// Pass null as plannerModel if no support for planning required
	ProfileWidget2(DivePlannerPointsModel *plannerModel, double dpr, QWidget *parent = 0);
	~ProfileWidget2();
	void resetZoom();
	// AI-generated (Claude)
	// When > 0, resize events use this width for the scene instead of the
	// viewport width. The right-hand portion of the scene is clipped (we
	// keep ScrollBarAlwaysOff). Used by the side picture-preview pane to
	// keep the time axis pixels-per-second constant while the profile
	// viewport shrinks. Pass 0 to release the lock.
	void setLockedSceneWidth(int widthPx);
	void plotDive(const struct dive *d, int dc, int flags = RenderFlags::None);
	void setProfileState(const struct dive *d, int dc);
	void setPlanState(const struct dive *d, int dc);
	void setEditState(const struct dive *d, int dc);
	bool isPlanner() const;
	void clear();
#ifndef SUBSURFACE_MOBILE
	bool eventFilter(QObject *, QEvent *) override;
#endif
	std::unique_ptr<ProfileScene> profileScene;
	State currentState;

signals:
	void stopAdded(); // only emitted in edit mode
	void stopRemoved(int count); // only emitted in edit mode
	void stopMoved(int count); // only emitted in edit mode
	void stopEdited(); // only emitted in edit mode
	// AI-generated (Claude)
	// Emitted when a thumbnail in the profile is left-clicked. MainWindow
	// routes this based on the current ApplicationState.
	void pictureClicked(const QString &fileUrl);

public
slots: // Necessary to call from QAction's signals.
	void settingsChanged();
	void actionRequestedReplot(bool triggered);
	void divesChanged(const QVector<dive *> &dives, DiveField field);
#ifndef SUBSURFACE_MOBILE
	void plotPictures();
	void picturesRemoved(dive *d, QVector<QString> filenames);
	void picturesAdded(dive *d, QVector<picture> pics);
	void pointsReset();
	void pointInserted(const QModelIndex &parent, int start, int end);
	void pointsRemoved(const QModelIndex &, int start, int end);
	void pointsMoved(const QModelIndex &, int start, int end, const QModelIndex &destination, int row);
	void updateThumbnail(QString filename, QImage thumbnail, duration_t duration);
	void profileChanged(dive *d);
	void pictureOffsetChanged(dive *d, QString filename, offset_t offset);
	void removePicture(const QString &fileUrl);
	// AI-generated (Claude)
	// When the side preview pane opens, the profile widget shrinks to
	// half width. To keep the visible pixels-per-second ratio, bump the
	// zoom by ~2x and pan so the clicked picture is centered. On exit,
	// restore the user's previous zoom/pan state.
	void enterPicturePreviewMode(const QString &fileUrl);
	void exitPicturePreviewMode();
	// AI-generated (Claude)
	// Show/clear the selection-driven media info inside the profile.
	// Visibility of the infobox follows prefs.mediainfobox (persistent
	// toggle); the red time marker follows prefs.show_pictures_in_profile.
	void showMediaInfo(const QString &fileUrl, int offsetSec,
			   const QString &timeStr, const QString &depthStr,
			   const QString &tempStr);
	void clearMediaInfo();

	/* this is called for every move on the handlers. maybe we can speed up this a bit? */
	void divePlannerHandlerMoved();
	void divePlannerHandlerClicked();
	void divePlannerHandlerReleased();
#endif

private:
	void setProfileState(); // keep currently displayed dive
	void resizeEvent(QResizeEvent *event) override;
#ifndef SUBSURFACE_MOBILE
	void wheelEvent(QWheelEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void keyPressEvent(QKeyEvent *e) override;
	void addGasChangeMenu(QMenu &m, QString menuTitle, const struct dive &d, int dcNr, int changeTime);
#endif
	void dropEvent(QDropEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;

	void replot();
	void diveComputerEdited(dive &dive, divecomputer &dc);
	void setZoom(int level);
	void addGasSwitch(int tank, int seconds);
	void changeGas(int index, int newCylinderId);
	void setupSceneAndFlags();
	void addItemsToScene();
	void setupItemOnScene();
	struct plot_data *getEntryFromPos(QPointF pos);
	void clearPictures();
	void plotPicturesInternal(const struct dive *d, bool synchronous);
	void updateThumbnails();
	void addDivemodeSwitch(int seconds, int divemode);
	void addBookmark(int seconds);
	void splitDive(int seconds);
	void addSetpointChange(int seconds);
	void removeEvent(DiveEventItem *item);
	void hideOneEvent(DiveEventItem *item);
	void hideEventType(DiveEventItem *item);
	void editName(DiveEventItem *item);
	void unhideEvents();
	void unhideEventTypes();
	void makeFirstDC();
	void deleteCurrentDC();
	void splitCurrentDC();
	void renameCurrentDC();

	DivePlannerPointsModel *plannerModel; // If null, no planning supported.
	int zoomLevel;
	double zoomedPosition;	// Position, when zoomed: 0.0 = beginning, 1.0 = end.
	// AI-generated (Claude) — picture preview mode state
	bool picturePreviewMode = false;
	// AI-generated (Claude)
	int lockedSceneWidth; // 0 = follow viewport; >0 = use this width on resize
#ifndef SUBSURFACE_MOBILE
	ToolTipItem *toolTipItem;
	// AI-generated (Claude)
	MediaInfoBoxItem *mediaInfoBox;
	DiveLineItem *mediaTimeMarker;
	bool mediaInfoActive = false;
	int mediaInfoOffsetSec = 0;
	QString mediaInfoFileUrl;
	DivePictureItem *highlightedThumbnail = nullptr;
	void layoutMediaInfo();
	void applyThumbnailHighlight();
#endif
	const struct dive *d;
	int dc;
	bool empty; // No dive shown.
	bool panning; // Currently panning.
	double panningOriginalMousePosition;
	double panningOriginalProfilePosition;
#ifndef SUBSURFACE_MOBILE
	DiveLineItem *mouseFollowerVertical;
	DiveLineItem *mouseFollowerHorizontal;
	RulerItem2 *rulerItem;
#endif

	std::vector<std::unique_ptr<QGraphicsSimpleTextItem>> gases;

#ifndef SUBSURFACE_MOBILE
	// The list of pictures in this plot. The pictures are sorted by offset in seconds.
	// For the same offset, sort by filename.
	// Pictures that are outside of the dive time are not shown.
	struct PictureEntry {
		offset_t offset;
		duration_t duration;
		std::string filename;
		std::unique_ptr<DivePictureItem> thumbnail;
		// For videos with known duration, we represent the duration of the video by a line
		std::unique_ptr<QGraphicsRectItem> durationLine;
		PictureEntry (offset_t offsetIn, const std::string &filenameIn, ProfileWidget2 *profile, bool synchronous);
		bool operator< (const PictureEntry &e) const;
	};
	void updateThumbnailXPos(PictureEntry &e);
	std::vector<PictureEntry> pictures;
	void calculatePictureYPositions();
	void updateDurationLine(PictureEntry &e);
	void updateThumbnailPaintOrder();

	void keyDeleteAction();
	void keyUpAction();
	void keyDownAction();
	void keyLeftAction();
	void keyRightAction();
#endif

	std::vector<std::unique_ptr<DiveHandler>> handles;
	int handleIndex(const DiveHandler *h) const;
	void disconnectPlannerModel();
#ifndef SUBSURFACE_MOBILE
	void connectPlannerModel();
	void repositionDiveHandlers();
	int fixHandlerIndex(DiveHandler *activeHandler);
	DiveHandler *createHandle();
	QGraphicsSimpleTextItem *createGas();
#endif
	friend class DiveHandler;
	bool shouldCalculateMax; // Calculate maximum time and depth (default). False when dragging handles.
	std::vector<int> selectedDiveHandleIndices() const;

	// We store a const pointer to the shown dive. However, the undo commands want
	// (understandably) a non-const pointer. Since the profile has a context-menu
	// with actions, it needs such a non-const pointer. This function turns the
	// currently shown dive into such a pointer. Ugly, yes.
	struct dive *mutable_dive() const;
};

#endif // PROFILEWIDGET2_H
