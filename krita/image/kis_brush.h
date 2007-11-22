/*
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef KIS_BRUSH_
#define KIS_BRUSH_

#include <QImage>
#include <QVector>



#include "kis_types.h"

#include "KoResource.h"
#include "kis_paint_information.h"

#include "krita_export.h"

class QString;
class QPoint;
class QIODevice;
class KoColor;

class KoColorSpace;

class KisBoundary;

enum enumBrushType {
    INVALID,
    MASK,
    IMAGE,
    PIPE_MASK,
    PIPE_IMAGE,
    AIRBRUSH
};

class KRITAIMAGE_EXPORT KisBrush : public KoResource {
  class ScaledBrush;

    Q_OBJECT

public:

    /// Construct brush to load filename later as brush
    KisBrush(const QString& filename);

    /// Load brush from the specified data, at position dataPos, and set the filename
    KisBrush(const QString& filename,
             const QByteArray & data,
             qint32 & dataPos);

    /// Load brush from the specified paint device, in the specified region
    KisBrush(KisPaintDeviceSP image, int x, int y, int w, int h);

    /// Load brush as a copy from the specified QImage (handy when you need to copy a brush!)
    KisBrush(const QImage& image, const QString& name = QString(""));

    virtual ~KisBrush();

    virtual bool load();
    /// synchronous, doesn't emit any signal (none defined!)
    virtual bool save();
    virtual QImage img() const;
    virtual bool saveToDevice(QIODevice* dev) const;

    /**
       @return a mask computed from the grey-level values of the
       pixels in the brush.
    */
    virtual void mask(KisPaintDeviceSP dst, const KoColor& color, const KisPaintInformation& info, double subPixelX = 0, double subPixelY = 0) const;
    virtual void mask(KisPaintDeviceSP dst, KisPaintDeviceSP src, const KisPaintInformation& info, double subPixelX = 0, double subPixelY = 0) const;
    // XXX: return non-tiled simple buffer
    virtual KisPaintDeviceSP image(const KoColorSpace * colorSpace, const KisPaintInformation& info,
                             double subPixelX = 0, double subPixelY = 0) const;

    void setHotSpot(QPointF);
    QPointF hotSpot(const KisPaintInformation& info = KisPaintInformation()) const;

    void setSpacing(double s) { m_spacing = s; }
    double spacing() const { return m_spacing; }
    double xSpacing(double pressure = PRESSURE_DEFAULT) const;
    double ySpacing(double pressure = PRESSURE_DEFAULT) const;

    // Dimensions in pixels of the mask/image at a given pressure.
    qint32 maskWidth(const KisPaintInformation& info) const;
    qint32 maskHeight(const KisPaintInformation& info) const;

    virtual void setUseColorAsMask(bool useColorAsMask) { m_useColorAsMask = useColorAsMask; }
    virtual bool useColorAsMask() const { return m_useColorAsMask; }
    virtual bool hasColor() const;

    virtual void makeMaskImage();
    qint32 width() const;
    qint32 height() const;

    virtual enumBrushType brushType() const;

    //QImage outline(double pressure = PRESSURE_DEFAULT);
    virtual KisBoundary boundary();

    /**
     * Returns true if this brush can return something useful for the info. This is used
     * by Pipe Brushes that can't paint sometimes
     **/
    virtual bool canPaintFor(const KisPaintInformation& /*info*/) { return true; }

    virtual KisBrush* clone() const;

    virtual void toXML(QDomDocument& , QDomElement&) const;

protected:
    void setWidth(qint32 w);
    void setHeight(qint32 h);
    void setImage(const QImage& img);
    void setBrushType(enumBrushType type) { m_brushType = type; }
    static double scaleForPressure(double pressure);

private:


    bool init();
    bool initFromPaintDev(KisPaintDeviceSP image, int x, int y, int w, int h);
    void createScaledBrushes() const;

    KisQImagemaskSP scaleMask(const ScaledBrush *srcBrush, double scale, double subPixelX, double subPixelY) const;
    QImage scaleImage(const ScaledBrush *srcBrush, double scale, double subPixelX, double subPixelY) const;

    static QImage scaleImage(const QImage& srcImage, int width, int height);
    static QImage interpolate(const QImage& image1, const QImage& image2, double t);

    static KisQImagemaskSP scaleSinglePixelMask(double scale, quint8 maskValue, double subPixelX, double subPixelY);
    static QImage scaleSinglePixelImage(double scale, QRgb pixel, double subPixelX, double subPixelY);

    // Find the scaled brush(es) nearest to the given scale.
    void findScaledBrushes(double scale, const ScaledBrush **aboveBrush, const ScaledBrush **belowBrush) const;

    // Initialize our boundary
    void generateBoundary();

    QByteArray m_data;
    bool m_ownData;
    QPointF m_hotSpot;
    double m_spacing;
    bool m_useColorAsMask;
    bool m_hasColor;
    QImage m_img;
    mutable QVector<ScaledBrush> m_scaledBrushes;

    qint32 m_width;
    qint32 m_height;

    quint32 m_header_size;  /*  header_size = sizeof (BrushHeader) + brush name  */
    quint32 m_version;      /*  brush file version #  */
    quint32 m_bytes;        /*  depth of brush in bytes */
    quint32 m_magic_number; /*  GIMP brush magic number  */

    enumBrushType m_brushType;

    KisBoundary* m_boundary;

};
#endif // KIS_BRUSH_

