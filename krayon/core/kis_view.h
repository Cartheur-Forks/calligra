/*
 *  kis_view.h - part of Krayon
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *                1999 Michael Koch    <koch@kde.org>
 *                1999 Carsten Pfeiffer <pfeiffer@kde.org>
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

#ifndef __kis_view_h__
#define __kis_view_h__

#include <koView.h>
#include <qscrollbar.h>
#include "kis_color.h"

class KAction;
class KToggleAction;
class KHelpMenu;
class QPaintEvent;

class KisDoc;
class KisImage;
class KisCanvas;
class KisPainter;
class KisTabBar;
class KisSideBar;

class KisBrushChooser;
class KisPatternChooser;
class KisKrayonChooser;

class KisLayerView;
class KisChannelView;

class QScrollBar;
class KRuler;

class FreehandSelectTool;
class RectangularSelectTool;
class PolygonalSelectTool;
class EllipticalSelectTool;
class ContiguousSelectTool;

class PasteTool;
class KisKrayon;
class KisBrush;
class KisPattern;
class KisTool;
class MoveTool;
class BrushTool;
class AirBrushTool;
class EraserTool;
class PenTool;
class ZoomTool;
class KisGradient;
class LineTool;
class PolyLineTool;
class PolyGonTool;
class RectangleTool;
class EllipseTool;
class ColorPicker;
class ColorChangerTool;
class FillTool;
class StampTool;
class QButton;


class KisView : public KoView
{
    Q_OBJECT

 public:  
 
    KisView( KisDoc* doc, QWidget* parent = 0, const char* name = 0 );
    ~KisView();
    
    KisColor& fgColor() { return m_fg; }
    KisColor& bgColor() { return m_bg; }

    KisCanvas   *kisCanvas()        { return m_pCanvas; }
    KisPainter  *kisPainter()       { return m_pPainter; }
    KisPattern  *currentPattern()   { return m_pPattern; }
    KisBrush    *currentBrush()     { return m_pBrush; }

    void updateCanvas(QRect & ur);
    void showScrollBars();
    void layerScale(bool smooth);
        
    int 	docWidth();
    int 	docHeight();

    int 	xPaintOffset();
    int 	yPaintOffset();
     
    int     xScrollOffset() { return m_pHorz->value(); }
    int     yScrollOffset() { return m_pVert->value(); }
    void    scrollTo( QPoint p );

    void    insert_layer_image(bool newLayer, const QString &filename = QString::null);
    void    save_layer_image(bool mergeLayers);

    void 	zoom( int x, int y, float zf );
    void    zoom_in( int x, int y );
    void	zoom_out( int x, int y );
    float   zoomFactor();
    void    setZoomFactor( float zf );

    virtual void setupPrinter( KPrinter &printer );
    virtual void print( KPrinter &printer );
    
 signals:
 
    void canvasMousePressEvent( QMouseEvent * );
    void canvasMouseMoveEvent( QMouseEvent * );
    void canvasMouseReleaseEvent( QMouseEvent * );
    void canvasEnterEvent( QEvent * );
    void canvasLeaveEvent( QEvent * );
    
    void bgColorChanged(const KisColor & );
    void fgColorChanged(const KisColor & );     

 public slots:
 
    void slotRefreshPainter();
    void slotUpdateImage();
    void slotDocUpdated();
    void slotDocUpdated(const QRect&);

    void slotSetKrayon(KisKrayon *);
    void slotSetBrush(KisBrush *);
    void slotSetPattern(KisPattern *);
    void slotSetFGColor(const KisColor&);
    void slotSetBGColor(const KisColor&);
    void slotSetPaintOffset();

    void slotTabSelected(const QString& name);

    void slotHalt(); // for the faint of heart
    void slotGimp(); // for the weak of mind
    
    void zoom_in();
    void zoom_out();
    
  protected slots:
 
    // edit action slots
    void undo();
    void redo();
    void copy();
    void cut();
    void removeSelection();
    void paste();
    void crop();
    void selectAll();
    void unSelectAll();
    
    // dialog action slots
    void dialog_gradient();
    void dialog_colors();
    void dialog_krayons();
    void dialog_brushes();
    void dialog_patterns();
    void dialog_layers();
    void dialog_channels();

    // layer action slots
    void insert_layer();
    void remove_layer();
    void link_layer();
    void hide_layer();
    void next_layer();
    void previous_layer();

    void layer_properties(); 

    void insert_image_as_layer();
    void save_layer_as_image();
    void slotEmbeddImage(const QString &filename);

    void layer_scale_smooth();
    void layer_scale_rough();
    void layer_rotate180();
    void layer_rotateleft90();
    void layer_rotateright90();
    void layer_mirrorX();
    void layer_mirrorY();

    // image action slots
    void import_image();
    void export_image();
    void add_new_image_tab();
    void remove_current_image_tab();
    void merge_all_layers();
    void merge_visible_layers();
    void merge_linked_layers();

    // tool action slots
    void tool_properties();

    void tool_select_freehand();
    void tool_select_rectangular();
    void tool_select_polygonal();
    void tool_select_elliptical();
    void tool_select_contiguous();

    void tool_move();
    void tool_zoom();
    void tool_brush();
    void tool_airbrush();
    void tool_pen();
    void tool_eraser();
    void tool_line();
    void tool_polyline();
    void tool_polygon();
    void tool_rectangle();
    void tool_ellipse();
    void tool_colorpicker();
    void tool_colorchanger();
    void tool_fill();
    void tool_stamp();
    void tool_paste();
    
    // settings action slots
    void showMenubar();
    void showToolbar();
    void showStatusbar();
    void floatSidebar();
    void showSidebar();
    void leftSidebar();
    void saveOptions();
    void preferences();

    // scrollbar slots
    void scrollH(int);
    void scrollV(int);

    void canvasGotMousePressEvent( QMouseEvent * );
    void canvasGotMouseMoveEvent ( QMouseEvent * );
    void canvasGotMouseReleaseEvent ( QMouseEvent * );
    void canvasGotPaintEvent( QPaintEvent* );
    void canvasGotEnterEvent( QEvent * );
    void canvasGotLeaveEvent( QEvent * );
    void canvasGotMouseWheelEvent( QWheelEvent * );
    void slotUndoRedoChanged( QString _undo, QString _redo );
    void slotUndoRedoChanged( QStringList _undo, QStringList _redo );

 protected:
 
    virtual void resizeEvent( QResizeEvent* );
    virtual void updateReadWrite( bool readwrite );

    void setupPixmap();
    void setupPainter();
    void setupCanvas();
    void setupSideBar();
    void setupScrollBars();
    void setupRulers();
    void setupTools();
    void setupDialogs();
    void setupActions();
    void setupTabBar();

    void activateTool(KisTool*);
             
 private:

    // import/export actions
    KAction *m_layer_rotate180, *m_layer_rotate270, *m_layer_rotate90,
        *m_layer_mirrorX, *m_layer_mirrorY; 
        
    // edit actions
    KAction *m_undo, *m_redo, *m_copy, *m_cut, *m_paste, *m_crop,
        *m_select_all, *m_unselect_all;

    // tool settings dialog actions
    KAction *m_dialog_gradient;

	// krayon box floating dialog actions
	KToggleAction *m_dialog_colors, *m_dialog_krayons, *m_dialog_brushes,
		*m_dialog_patterns, *m_dialog_layers, *m_dialog_channels;

    // krayon box (sidebar)
    KToggleAction *m_side_bar, *m_float_side_bar, *m_lsidebar;

    // settings actions
    KToggleAction *m_toggle_paint_offset;
    
    // tool actions (main toolbar & menu)
    KToggleAction *m_tool_select_freehand, *m_tool_select_rectangular, *m_tool_select_polygonal,
    *m_tool_select_elliptical, *m_tool_select_contiguous,
    *m_tool_move, *m_tool_zoom, 
    *m_tool_brush, *m_tool_draw, *m_tool_pen, 
    *m_tool_colorpicker, *m_tool_colorchanger, 
    *m_tool_fill, *m_tool_stamp, *m_tool_paste,
    *m_tool_airbrush, *m_tool_eraser,
    *m_tool_line, *m_tool_polyline, *m_tool_polygon,
    *m_tool_rectangle, *m_tool_ellipse;

    KisDoc                *m_pDoc;  // always needed
    KisTool               *m_pTool; // current active tool

    // tools    
    FreehandSelectTool     *m_pFreehandSelectTool;
    RectangularSelectTool  *m_pRectangularSelectTool;
    PolygonalSelectTool    *m_pPolygonalSelectTool;
    EllipticalSelectTool   *m_pEllipticalSelectTool;
    ContiguousSelectTool   *m_pContiguousSelectTool;

    PasteTool           *m_pPasteTool;
    MoveTool            *m_pMoveTool;
    BrushTool           *m_pBrushTool;
    EraserTool          *m_pEraserTool;
    AirBrushTool        *m_pAirBrushTool;
    PenTool             *m_pPenTool;
    ZoomTool            *m_pZoomTool;

    LineTool            *m_pLineTool;
    PolyLineTool        *m_pPolyLineTool;
    PolyGonTool         *m_pPolyGonTool;
    RectangleTool       *m_pRectangleTool;
    EllipseTool         *m_pEllipseTool;   
    ColorPicker         *m_pColorPicker;
    ColorChangerTool    *m_pColorChangerTool;
    FillTool            *m_pFillTool;
    StampTool           *m_pStampTool;
    
    // krayon objects - all can be krayons
    KisKrayon     *m_pKrayon;   // current krayon for this view   
    KisBrush      *m_pBrush;    // current brush for this view
    KisPattern    *m_pPattern;  // current pattern for this view 
    KisGradient   *m_pGradient; // current gradient   
    KisImage      *m_pImage;    // current image for this view

    // sidebar dock widgets
    KisKrayonChooser     *m_pKrayonChooser;    
    KisBrushChooser      *m_pBrushChooser;
    KisPatternChooser    *m_pPatternChooser;
    QWidget              *m_pPaletteChooser;    
    QWidget              *m_pGradientChooser;
    QWidget              *m_pImageChooser;
    KisLayerView         *m_pLayerView;
    KisChannelView       *m_pChannelView;

    // krayon and kde objects
    KisCanvas           *m_pCanvas;
    KisPainter          *m_pPainter;
    KisSideBar          *m_pSideBar;
    QScrollBar          *m_pHorz, *m_pVert;
    KRuler              *m_pHRuler, *m_pVRuler;
    KisColor            m_fg, m_bg;
    KisTabBar           *m_pTabBar;
    QButton             *m_pTabFirst, *m_pTabLeft, 
                        *m_pTabRight, *m_pTabLast;

    // what's this doing here?
    KHelpMenu            *m_helpMenu;

    // normal variables
    float	    m_zoomFactor;
    int         m_xPaintOffset;
    int         m_yPaintOffset;    
    bool        buttonIsDown;
};

#endif
