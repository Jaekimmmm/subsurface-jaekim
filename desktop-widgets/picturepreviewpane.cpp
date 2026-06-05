// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "desktop-widgets/picturepreviewpane.h"
#include "core/qthelper.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

// Draw a simple white X on a transparent square. Avoids relying on a
// theme/resource icon that may be missing or wrongly aliased.
QIcon makeCloseIcon(int sizePx)
{
	QPixmap pm(sizePx, sizePx);
	pm.fill(Qt::transparent);
	QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::white);
	pen.setWidth(2);
	pen.setCapStyle(Qt::RoundCap);
	p.setPen(pen);
	const int inset = sizePx / 5;
	p.drawLine(inset, inset, sizePx - inset, sizePx - inset);
	p.drawLine(sizePx - inset, inset, inset, sizePx - inset);
	return QIcon(pm);
}

} // namespace

PicturePreviewPane::PicturePreviewPane(QWidget *parent) : QWidget(parent)
{
	// Opaque black background.
	QPalette pal = palette();
	pal.setColor(QPalette::Window, Qt::black);
	setAutoFillBackground(true);
	setPalette(pal);

	imageLabel = new QLabel(this);
	imageLabel->setAlignment(Qt::AlignCenter);
	imageLabel->setMinimumSize(1, 1);
	{
		QPalette ipal = imageLabel->palette();
		ipal.setColor(QPalette::Window, Qt::black);
		ipal.setColor(QPalette::WindowText, Qt::white);
		imageLabel->setPalette(ipal);
		imageLabel->setAutoFillBackground(true);
	}

	errorLabel = new QLabel(this);
	errorLabel->setAlignment(Qt::AlignCenter);
	errorLabel->setWordWrap(true);
	errorLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	{
		QPalette epal = errorLabel->palette();
		epal.setColor(QPalette::WindowText, Qt::white);
		errorLabel->setPalette(epal);
	}
	errorLabel->hide();

	pathLabel = new QLabel(this);
	pathLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	pathLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	pathLabel->setMinimumWidth(0);
	{
		QPalette ppal = pathLabel->palette();
		ppal.setColor(QPalette::WindowText, Qt::white);
		pathLabel->setPalette(ppal);
	}
	pathLabel->setStyleSheet(
		"QLabel { color: white; padding: 2px 6px; "
		"background: rgba(255, 255, 255, 20); border-radius: 3px; }");

	const int btnPx = 18;
	closeBtn = new QToolButton(this);
	closeBtn->setIcon(makeCloseIcon(btnPx));
	closeBtn->setIconSize(QSize(btnPx, btnPx));
	closeBtn->setAutoRaise(true);
	closeBtn->setToolTip(tr("Close preview"));
	closeBtn->setCursor(Qt::PointingHandCursor);
	closeBtn->setStyleSheet(
		"QToolButton { border: none; background: transparent; }"
		"QToolButton:hover { background: rgba(255, 255, 255, 40); }");
	connect(closeBtn, &QToolButton::clicked, this, &PicturePreviewPane::closed);

	// Top row: spacer + close button anchored top-right.
	// Below: image label (stretch) + optional error label.
	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->setContentsMargins(0, 0, 0, 0);
	vbox->setSpacing(0);

	QHBoxLayout *topRow = new QHBoxLayout();
	topRow->setContentsMargins(6, 6, 6, 0);
	topRow->setSpacing(6);
	topRow->addWidget(pathLabel, 1);
	topRow->addWidget(closeBtn, 0);
	vbox->addLayout(topRow);

	// imageLabel and errorLabel occupy the same vertical area; only one is
	// visible at a time so the error text appears centered in the pane.
	vbox->addWidget(imageLabel, 1);
	vbox->addWidget(errorLabel, 1);
}

void PicturePreviewPane::showFor(const QString &fileUrl)
{
	currentFile = fileUrl;
	const QString localPath = localFilePath(fileUrl);
	original = QPixmap(localPath);
	loadFailed = original.isNull();
	if (loadFailed) {
		const QString shown = localPath.isEmpty() ? fileUrl : localPath;
		errorLabel->setText(tr("Failed to load image:\n%1").arg(shown));
		imageLabel->clear();
		imageLabel->hide();
		errorLabel->show();
	} else {
		errorLabel->hide();
		imageLabel->show();
		rescaleImage();
	}
	updatePathLabel();
}

void PicturePreviewPane::updatePathLabel()
{
	const QString full = !localFilePath(currentFile).isEmpty()
				     ? localFilePath(currentFile)
				     : currentFile;
	pathLabel->setToolTip(full);
	int avail = pathLabel->width();
	if (avail <= 0)
		avail = width() - closeBtn->width() - 24;
	if (avail < 20) {
		pathLabel->setText(QString());
		return;
	}
	QFontMetrics fm(pathLabel->font());
	pathLabel->setText(fm.elidedText(full, Qt::ElideMiddle, avail - 12));
}

bool PicturePreviewPane::isShownFor(const QString &fileUrl) const
{
	return isVisible() && currentFile == fileUrl;
}

void PicturePreviewPane::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	rescaleImage();
	updatePathLabel();
}

void PicturePreviewPane::rescaleImage()
{
	if (loadFailed || original.isNull()) {
		imageLabel->clear();
		return;
	}
	QSize target = imageLabel->size();
	if (target.width() < 2 || target.height() < 2)
		return;
	imageLabel->setPixmap(original.scaled(target,
					      Qt::KeepAspectRatio,
					      Qt::SmoothTransformation));
}
