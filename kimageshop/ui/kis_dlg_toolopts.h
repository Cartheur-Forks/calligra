/*
 *  kis_dlg_toolopts.h - part of Krayon
 *
 *  Copyright (c) 2001 John Califf 
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __tooloptionsdialog_h__
#define __tooloptionsdialog_h__

#include <qspinbox.h>
#include <qlineedit.h>
#include <qcheckbox.h>

#include <kdialog.h>

class QVBoxLayout;

enum tooltype 
{
    tt_linetool,
    tt_rectangletool,
    tt_ellipsetool,
    tt_polylinetool,
    tt_brushtool,
    tt_airbrushtool,
    tt_pentool,
    tt_erasertool,
    tt_filltool,
    tt_colorchangertool,
    tt_colorpickertool,
    tt_movetool,
    tt_stamptool,
    tt_pastetool
};

struct ToolOptsStruct
{
    int lineThickness;
    int lineOpacity;
    
    int penThreshold;
    int penOpacity;
    
    int opacity;
    
    bool fillShapes;
    bool usePattern;
    bool useGradient;
};


class KisToolTab : public QWidget
{
  Q_OBJECT

public:

    KisToolTab(ToolOptsStruct ts, QWidget *parent = 0, const char *name = 0 );

    int  thickness()    { return mpThickness->value(); };
    int  opacity()      { return mpOpacity->value(); };
    int  penThreshold() { return mpPenThreshold->value(); };
    int  density()      { return mpDensity->value(); };
    int  granularuty()  { return mpGranularity->value(); };
    
    bool solid()        { return mpSolid->isChecked(); };
    bool usePattern()   { return mpUsePattern->isChecked(); };
    bool useGradient()  { return mpUseGradient->isChecked(); };
    bool blendPattern() { return mpBlendPattern->isChecked(); };
    bool blendGradient(){ return mpBlendGradient->isChecked(); };
    
protected:

    QSpinBox  *mpThickness;
    QSpinBox  *mpOpacity;
    QSpinBox  *mpPenThreshold;
    QSpinBox  *mpDensity;
    QSpinBox  *mpGranularity;
        
    QCheckBox *mpSolid;
    QCheckBox *mpUsePattern;
    QCheckBox *mpUseGradient;
    QCheckBox *mpBlendPattern;
    QCheckBox *mpBlendGradient;    
};


class NullToolTab : public KisToolTab
{
  Q_OBJECT

public:

    NullToolTab( ToolOptsStruct ts,  QWidget *parent = 0, 
        const char *name = 0 );
};


class LineToolTab : public KisToolTab
{
  Q_OBJECT

public:

    LineToolTab( ToolOptsStruct ts,  QWidget *parent = 0, 
        const char *name = 0 );
};


class PenToolTab : public KisToolTab
{
  Q_OBJECT

public:

    PenToolTab( ToolOptsStruct ts,  QWidget *parent = 0, 
        const char *name = 0 );
};


class BrushToolTab : public KisToolTab
{
  Q_OBJECT

public:

    BrushToolTab( ToolOptsStruct ts,  QWidget *parent = 0, 
        const char *name = 0 );
};


class EraserToolTab : public KisToolTab
{
  Q_OBJECT

public:

    EraserToolTab( ToolOptsStruct ts,  QWidget *parent = 0, 
        const char *name = 0 );
};


class AirBrushToolTab : public KisToolTab
{
  Q_OBJECT

public:

    AirBrushToolTab( ToolOptsStruct ts,  QWidget *parent = 0, 
        const char *name = 0 );
};


class FillToolTab : public KisToolTab
{
  Q_OBJECT

public:

    FillToolTab( ToolOptsStruct ts,  QWidget *parent = 0, 
        const char *name = 0 );
};


class StampToolTab : public KisToolTab
{
  Q_OBJECT

public:

    StampToolTab( ToolOptsStruct ts,  QWidget *parent = 0, 
        const char *name = 0 );
};


class ToolOptionsDialog : public KDialog
{
  Q_OBJECT

public:

    ToolOptionsDialog( tooltype tt, ToolOptsStruct ts,
        QWidget *parent = 0, const char *name = 0 );
    
    NullToolTab *nullToolTab()      
        { return pNullToolTab; }

    LineToolTab *lineToolTab()      
        { return pLineToolTab; }    

    PenToolTab  *penToolTab()       
        { return pPenToolTab; }

    BrushToolTab *brushToolTab()    
        { return pBrushToolTab; }

    AirBrushToolTab *airBrushToolTab() 
        { return pAirBrushToolTab; }    

    EraserToolTab *eraserToolTab()  
        { return pEraserToolTab; }

    FillToolTab *fillToolTab()      
        { return pFillToolTab; }

    StampToolTab *stampToolTab()      
        { return pStampToolTab; }

private:

    NullToolTab     *pNullToolTab;
    LineToolTab     *pLineToolTab;
    PenToolTab      *pPenToolTab;
    BrushToolTab    *pBrushToolTab;
    AirBrushToolTab *pAirBrushToolTab;    
    EraserToolTab   *pEraserToolTab;
    FillToolTab     *pFillToolTab;
    StampToolTab    *pStampToolTab;    
};

#endif // __tooloptionsdialog.h__
