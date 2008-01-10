/*
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KIS_DYNAMICOP_H_
#define KIS_DYNAMICOP_H_

#include "kis_paintop.h"
#include <QString>
#include <klocale.h>

class QWidget;
class QPointF;
class KisPainter;

class KisDynamicBrush;
class Ui_DynamicBrushOptions;

class KisBookmarkedConfigurationManager;
class KisBookmarkedConfigurationsModel;

class KisDynamicOpFactory : public KisPaintOpFactory  {
    public:
        KisDynamicOpFactory(KisBookmarkedConfigurationManager* shapeBookmarksManager, KisBookmarkedConfigurationManager* coloringBookmarksManager) :
          m_shapeBookmarksManager(shapeBookmarksManager), m_coloringBookmarksManager(coloringBookmarksManager)
        {}
        virtual ~KisDynamicOpFactory() {}

        virtual KisPaintOp * createOp(const KisPaintOpSettings *settings, KisPainter * painter, KisImageSP image);
        virtual QString id() const { return "dynamicbrush"; }
        virtual QString name() const { return i18n("Dynamic Brush"); }
        virtual QString pixmap() { return "dynamicbrush.png"; }
        virtual KisPaintOpSettings *settings(QWidget * parent, const KoInputDevice& inputDevice, KisImageSP image);
    private:
        KisBookmarkedConfigurationManager* m_shapeBookmarksManager;
        KisBookmarkedConfigurationManager* m_coloringBookmarksManager;
};

class KisDynamicOpSettings : public QObject, public KisPaintOpSettings {
    Q_OBJECT
    public:
        KisDynamicOpSettings(QWidget* parent, KisBookmarkedConfigurationManager* shapeConfigurationManager, KisBookmarkedConfigurationManager* coloringConfigurationManager);
        virtual ~KisDynamicOpSettings();
        virtual KisPaintOpSettings* clone() const;
        virtual QWidget *widget() const { return m_optionsWidget; }
        /// @return a brush with the current shapes, coloring and program
        KisDynamicBrush* createBrush(KisPainter *painter) const;
        virtual void fromXML(const QDomElement&);
        virtual void toXML(QDomDocument&, QDomElement&) const;
    private:
        QWidget* m_optionsWidget;
        Ui_DynamicBrushOptions* m_uiOptions;
        KisBookmarkedConfigurationsModel* m_shapeBookmarksModel;
        KisBookmarkedConfigurationsModel* m_coloringBookmarksModel;
};

class KisDynamicOp : public KisPaintOp {

public:

    KisDynamicOp(const KisDynamicOpSettings *settings, KisPainter * painter);
    virtual ~KisDynamicOp();

    void paintAt(const KisPaintInformation& info);

private:
    KisDynamicBrush* m_brush;
    const KisDynamicOpSettings *m_settings;
};

#endif // KIS_DYNAMICOP_H_
