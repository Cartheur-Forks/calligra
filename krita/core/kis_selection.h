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
#if !defined KIS_SELECTION_H_
#define KIS_SELECTION_H_

#include <qimage.h>
#include <qrect.h>
#include "kis_global.h"
#include "kis_types.h"
#include "kis_layer.h"

class KisSelection : public KisLayer {
	Q_OBJECT
	typedef KisLayer super;

public:
	KisSelection(Q_INT32 width, Q_INT32 height, const enumImgType& imgType, const QString& name);
	KisSelection(KisPaintDeviceSP parent, KisImageSP img, const QString& name, QUANTUM opacity);
	virtual ~KisSelection();

public:
	// Overide KisLayer
	virtual bool shouldDrawBorder() const;
	virtual void move(Q_INT32 x, Q_INT32 y);

public:
	void commit();
	void fromImage(const QImage& img);
	QImage toImage();
	void setBounds(Q_INT32 parentX, Q_INT32 parentY, Q_INT32 width, Q_INT32 height);
	void setBounds(const QRect& rc);
	KisPaintDeviceSP parent() const;
	void setParent(KisPaintDeviceSP parent);

private slots:
	void parentVisibilityChanged(KisPaintDeviceSP parent);

private:
	KisPaintDeviceSP m_parent;
	KisImageSP m_img;
	QImage m_clipImg;
	QString m_name;
	QRect m_rc;
	bool m_firstMove;
};

#endif // KIS_SELECTION_H_

