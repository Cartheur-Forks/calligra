/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef KIS_DLG_IMAGE_PROPERTIES_H_
#define KIS_DLG_IMAGE_PROPERTIES_H_

#include <qspinbox.h>

#include <kdialogbase.h>

#include <qcolor.h>

#include "kis_global.h"
#include "wdgnewimage.h"

#include "kis_types.h"
#include "kis_image.h"

class KisView;
class QButtonGroup;
class KisID;

class KisDlgImageProperties : public KDialogBase {
	typedef KDialogBase super;
	Q_OBJECT

public:
	KisDlgImageProperties(KisImageSP image,
			      QWidget *parent = 0, 
			      const char *name = 0);
	virtual ~KisDlgImageProperties();

private slots:

	void okClicked();

	void fillCmbProfiles(const KisID &);


private:
	
	WdgNewImage * m_page;
	KisImageSP m_image;
	KisView * m_view;
};



#endif // KIS_DLG_IMAGE_PROPERTIES_H_

