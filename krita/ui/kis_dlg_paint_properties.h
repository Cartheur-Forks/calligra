/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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
#if !defined KIS_DLG_LAYER_PROPERTIES_H_
#define KIS_DLG_LAYER_PROPERTIES_H_

#include <kdialogbase.h>

class QWidget;
class KIntSpinBox;
class KLineEdit;
class KisPaintDevice;
class KIntNumInput;

class KisPaintPropertyDlg : public KDialogBase {
	typedef KDialogBase super;

public:
	KisPaintPropertyDlg(const QString& deviceName, const QPoint& pos, Q_INT32 opacity, QWidget *parent = 0, const char *name = 0, WFlags f = 0);
	virtual ~KisPaintPropertyDlg();

	QString getName() const;
	Q_INT32 getOpacity() const;
	QPoint getPosition() const;

private:
	KLineEdit *m_name;
	KIntNumInput *m_opacity;
	KIntSpinBox *m_x;
	KIntSpinBox *m_y;
};

#endif // KIS_DLG_LAYER_PROPERTIES_H_

