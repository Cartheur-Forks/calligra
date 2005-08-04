/*
 *  kis_previewview.h - part of Krita
 *
 *  Copyright (c) 2001 John Califf  <jcaliff@compuzone.net>
 *  Copyright (c) 2004 Bart Coppens <kde@bartcoppens.be>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __kis_previewview_h__
#define __kis_previewview_h__

#include <qwidget.h>
#include <qsize.h>

#include "kis_types.h"

class KisUndoAdapter;

/**
 * A Widget simply displaying a preview layer
 **/
class KisPreviewView : public QWidget
{
	Q_OBJECT

public:
        KisPreviewView(QWidget* parent = 0, const char * name = 0, WFlags f = 0);
        void setSourceLayer(KisLayerSP s);
        KisLayerSP getSourceLayer();
        KisLayerSP getPreviewLayer();
        double getZoom() { return m_zoom; }
        QPoint getPos() { return m_pos; }
        void updateView();

public slots:
	void setZoom(double zoom);
        void zoomIn();
        void zoomOut();
        void updatedPreview();
        void slotStartMoving(QPoint startDrag);
        void slotMoving(QPoint zoomedPos);
        void slotMoved(QPoint zoomedPos);
signals:
        /** The moving/creation of this widget has finished; effects could be applied to it now */
        void updated();
        void startMoving(QPoint);
        /**
         * The widget is currently moving its layer.
         * @return the difference (in x and y) between the starting position and the current one
         */
        void moving(QPoint);
        void moved(QPoint);
protected:
        virtual void paintEvent(QPaintEvent*);
        virtual void mousePressEvent(QMouseEvent * e);
        virtual void mouseMoveEvent(QMouseEvent * e);
        virtual void mouseReleaseEvent(QMouseEvent * e);
        virtual void resizeEvent(QResizeEvent *e);
private:
        void render(QPainter &painter, KisImageSP image);
        void updateView(QPoint delta);

        KisLayerSP m_sourcelayer, m_clippedview;
        KisImageSP m_image;
        QPoint m_startDrag, m_pos;
        double m_zoom;
        bool m_moving;
};


#endif
