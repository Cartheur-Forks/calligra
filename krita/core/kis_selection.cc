/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *
 *  this program is free software; you can redistribute it and/or modify
 *  it under the terms of the gnu general public license as published by
 *  the free software foundation; either version 2 of the license, or
 *  (at your option) any later version.
 *
 *  this program is distributed in the hope that it will be useful,
 *  but without any warranty; without even the implied warranty of
 *  merchantability or fitness for a particular purpose.  see the
 *  gnu general public license for more details.
 *
 *  you should have received a copy of the gnu general public license
 *  along with this program; if not, write to the free software
 *  foundation, inc., 675 mass ave, cambridge, ma 02139, usa.
 */
#include <stdlib.h>
#include <kcommand.h>
#include <kdebug.h>
#include <klocale.h>
#include <koColor.h>
#include "kis_doc.h"
#include "kis_global.h"
#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_painter.h"
#include "kis_selection.h"
#include "kistile.h"
#include "kistilemgr.h"
#include "kispixeldata.h"

namespace {
	class KisResetFirstMoveCmd : public KNamedCommand {
		typedef KNamedCommand super;

	public:
		KisResetFirstMoveCmd(KisSelectionSP selection) : super("reset selection first move flag")
		{
			m_selection = selection;
		}

		virtual ~KisResetFirstMoveCmd()
		{
		}

	public:
		virtual void execute()
		{
			m_selection -> clearParentOnMove(false);
		}

		virtual void unexecute()
		{
			m_selection -> clearParentOnMove(true);
		}

	private:
		KisSelectionSP m_selection;
	};
}

KisSelection::KisSelection(Q_INT32 width, Q_INT32 height, const enumImgType& imgType, const QString& name) : super(width, height, imgType, name)
{
	m_clearOnMove = true;
	visible(false);
	setName(name);
}

KisSelection::KisSelection(KisPaintDeviceSP parent, KisImageSP img, const QString& name, QUANTUM opacity) : super(img, 0, 0, name, opacity)
{
	Q_ASSERT(parent);
	Q_ASSERT(parent -> visible());
	Q_ASSERT(img);
	m_parent = parent;
	m_img = img;
	m_name = name;
	m_firstMove = true;
	m_clearOnMove = true;
	connect(m_parent, SIGNAL(visibilityChanged(KisPaintDeviceSP)), SLOT(parentVisibilityChanged(KisPaintDeviceSP)));
}

KisSelection::~KisSelection()
{
}

void KisSelection::commit()
{
	if (m_parent) {
		KisDoc *doc = image() -> document();
		KisImageSP img;
		KisPainter gc;
		QRect rc = clip();
		QPoint pt(0, 0);
		Q_INT32 w = width();
		Q_INT32 h = height();

		if (!rc.isEmpty()) {
			w = rc.width();
			h = rc.height();
		}

		if (x() < 0) {
			pt.setX(-x());
			setX(0);
		}

		if (y() < 0) {
			pt.setY(-y());
			setY(0);
		}

		img = m_parent -> image();

		if (x() < m_parent -> x() || y() < m_parent -> y())
			m_parent -> offsetBy(abs(m_parent -> x() - x()), abs(m_parent -> y() - y()));

		if (img -> width() < x() + w)
			w = img -> width() - x();

		if (img -> height() < y() + h)
			h = img -> height() - y();

		if (x() + w > m_parent -> x() + m_parent -> width() || y() + h > m_parent -> y() + m_parent -> height())
			m_parent -> expand(x() + w - m_parent -> x(), y() + h - m_parent -> y());

		gc.begin(m_parent);
		gc.beginTransaction("copy selection to parent");
		Q_ASSERT(w <= width());
		Q_ASSERT(h <= height());
		gc.bitBlt(x() - m_parent -> x(), y() - m_parent -> y(), COMPOSITE_COPY, this, opacity(), pt.x(), pt.y(), w, h);
		doc -> addCommand(gc.endTransaction());
		gc.end();
	}
}

bool KisSelection::shouldDrawBorder() const
{
	return true;
}

void KisSelection::move(Q_INT32 x, Q_INT32 y)
{
	QRect rc = clip();

	if (m_clearOnMove && m_firstMove && m_parent) {
		KisPainter gc(m_parent);
		KisDoc *doc = image() -> document();

		doc -> beginMacro(i18n("Move Selection"));
		doc -> addCommand(new KisResetFirstMoveCmd(this));
		gc.beginTransaction("clear the parent's background from KisSelection::move");
		gc.eraseRect(this -> x() - m_parent -> x(), this -> y() - m_parent -> y(), width(), height());
		m_firstMove = false;
		m_parent -> invalidate(rc);
		doc -> addCommand(gc.endTransaction());
	}

	super::move(x, y);
}

void KisSelection::setBounds(Q_INT32 parentX, Q_INT32 parentY, Q_INT32 width, Q_INT32 height)
{
	if (m_img) {
		KisPainter gc;

		// TODO if the parent is linked... copy from all linked layers?!?
		configure(m_img, width, height, m_img -> imgType(), m_name);
		gc.begin(this);
		gc.bitBlt(0, 0, COMPOSITE_COPY, m_parent, parentX - m_parent -> x(), parentY - m_parent -> y(), width, height);
		super::move(parentX, parentY);
	}
}

void KisSelection::fromImage(const QImage& img)
{
	KoColor c;
	QRgb rgb;
	Q_INT32 opacity;

	if (img.isNull())
		return;

	for (Q_INT32 y = 0; y < height(); y++) {
		for (Q_INT32 x = 0; x < width(); x++) {
			rgb = img.pixel(x, y);
			c.setRGB(upscale(qRed(rgb)), upscale(qGreen(rgb)), upscale(qBlue(rgb)));

			if (img.hasAlphaBuffer())
				opacity = qAlpha(rgb);
			else
				opacity = OPACITY_OPAQUE;

			pixel(x, y, c, opacity);
		}
	}
}

QImage KisSelection::toImage()
{
	KisTileMgrSP tm = data();
	KisPixelDataSP raw;
	Q_INT32 stride;
	QUANTUM *src;

	if (tm) {
		if (tm -> width() == 0 || tm -> height() == 0)
			return QImage();

		raw = tm -> pixelData(0, 0, tm -> width() - 1, tm -> height() - 1, TILEMODE_READ);

		if (raw == 0)
			return QImage();

		if (m_clipImg.width() != tm -> width() || m_clipImg.height() != tm -> height())
			m_clipImg.create(tm -> width(), tm -> height(), 32);

		stride = tm -> depth();
		src = raw -> data;

		for (Q_INT32 y = 0; y < tm -> height(); y++) {
			for (Q_INT32 x = 0; x < tm -> width(); x++) {
				// TODO Different img formats
				m_clipImg.setPixel(x, y, qRgb(downscale(src[PIXEL_RED]), downscale(src[PIXEL_GREEN]), downscale(src[PIXEL_BLUE])));
				src += stride;
			}
		}
	}

	return m_clipImg;
}

void KisSelection::setBounds(const QRect& rc)
{
	setBounds(rc.x(), rc.y(), rc.width(), rc.height());
}

void KisSelection::parentVisibilityChanged(KisPaintDeviceSP parent)
{
	visible(parent -> visible());
}

void KisSelection::setParent(KisPaintDeviceSP parent)
{
	m_parent = parent;
}

KisPaintDeviceSP KisSelection::parent() const
{
	return m_parent;
}

void KisSelection::anchor()
{
	KisDoc *doc;

	if (m_firstMove == false) {
		doc = m_parent -> image() -> document();
		doc -> endMacro();
	}

	super::anchor();
}

void KisSelection::clearParentOnMove(bool f)
{
	m_clearOnMove = f;
	m_firstMove = true;
}

#include "kis_selection.moc"

