/*
 *  Copyright (c) 2000 Michael Koch <koch@kde.org>
 *  Copyright (c) 2000 Patrick Julien <freak@codepimps.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef  KIS_DLG_NEW_LAYER_H_
#define KIS_DLG_NEW_LAYER_H_

class QSpinBox;
class QWidget;
class KIntSpinBox;
class KLineEdit;
class KisPaintDevice;
class KIntNumInput;
class KisCmbComposite;
class KisCmbImageType;
class KisCmbIDList;

#include <kdialogbase.h>

#include <kis_global.h>

class NewLayerDialog : public KDialogBase {
	typedef KDialogBase super;
	Q_OBJECT

public:
	NewLayerDialog(const KisID colorSpace,
		       const QString & deviceName,
		       QWidget *parent = 0,
		       const char *name = 0);

	QString layerName() const;
	CompositeOp compositeOp() const;
	Q_INT32 opacity() const;
	QPoint position() const;
	KisID colorStrategyID() const;

private:
	KLineEdit *m_name;
	KIntNumInput *m_opacity;
	KisCmbComposite *m_cmbComposite;
	KisCmbIDList *m_cmbImageType;
};

#endif // KIS_DLG_NEW_LAYER_H_

