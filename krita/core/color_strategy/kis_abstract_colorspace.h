/*
 *  Copyright (c) 2002 Patrick Julien  <freak@codepimps.org>
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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
#ifndef KIS_ABSTRACT_COLORSPACE_H_
#define KIS_ABSTRACT_COLORSPACE_H_

#include <qmap.h>
#include <qcolor.h>
#include <qstringlist.h>
#include <qpair.h>

#include <ksharedptr.h>

#include "kis_global.h"
#include "kis_types.h"
#include "kis_channelinfo.h"
#include "kis_profile.h"
#include "kis_id.h"
#include "kis_composite_op.h"

#include "koffice_export.h"

class QPainter;
class KisIteratorPixel;
class KisPixel;
class KisPixelRO;


// XXX: This is not enough to identify all the transforms we need: we
// need something that combines the two profiles + the rendering
// intent + the lcms colortype -- a certain profile can be used with
// many varieties of, e.g. RGB(a).
typedef QPair<KisProfileSP, KisProfileSP> KisProfilePair;

// This would be better, I guess.
struct transform {
    KisProfile srcProfile;
    Q_UINT32 srcCmType;
    KisProfile dstProfile;
    Q_UINT32 dstType;
    Q_UINT32 renderIntent;
};

struct KisColorAdjustment;

/**
 * A colorspace strategy is the definition of a certain color model
 * in Krita.
 */
class KRITACORE_EXPORT KisAbstractColorSpace : public KShared {


public:

    /**
     * Create a new colorspace strategy.
     *
     * @param id The Krita identification of this color model.
     * @param cmType The littlecms colorstrategy type we wrap.
     * @param colorSpaceSignature The icc signature for the colorspace we are.
     */
    KisAbstractColorSpace(const KisID & id, DWORD cmType, icColorSpaceSignature colorSpaceSignature);

    /**
     * After creating the default profile, call init to setup the default
     * colortransforms from and to rgb and xyz -- if your colorspace needs
     * the fallback to the default transforms for the qcolor conversion
     * and the default pixel ops.
     */
    void init();

    virtual ~KisAbstractColorSpace();


//================== Information about this color strategy ========================//

public:


    //========== Channels =====================================================//

    // Return a vector describing all the channels this color model has.
    virtual vKisChannelInfoSP channels() const = 0;

    /**
     * The total number of channels for a single pixel in this color model
     */
    virtual Q_INT32 nChannels() const = 0;

    /**
     * The total number of color channels (excludes alpha and substance) for a single
     * pixel in this color model.
     */
    virtual Q_INT32 nColorChannels() const = 0;

    /**
     * The total number of substance channels for a single pixel
     * in this color model
     */
    virtual Q_INT32 nSubstanceChannels() const { return 0; };


    /**
     * The size in bytes of a single pixel in this color model
     */
    virtual Q_INT32 pixelSize() const = 0;

    /**
     * Whether this color model has a channel of type ALPHA
     */
    virtual bool hasAlpha() const = 0;

    /**
     * Gives the position of the first byte of alpha in the pixel
     */
    virtual Q_INT32 alphaPos() { return m_alphaPos; }

    /**
     * Gives the length in bytes of the alphachannel
     */
    virtual Q_INT32 alphaSize() { return m_alphaSize; }

    /**
     * Return a string with the channel's value suitable for display in the gui.
     */
    virtual QString channelValueText(const Q_UINT8 *pixel, Q_UINT32 channelIndex) const = 0;

    /**
     * Return a string with the channel's value with integer
     * channels normalised to the floating point range 0 to 1, if appropriate.
     */
    virtual QString normalisedChannelValueText(const Q_UINT8 *pixel, Q_UINT32 channelIndex) const = 0;

    //========== Identification ===============================================//

    /**
     * Krita definition for use in .kra files and internally: unchanging name +
     * i18n'able description.
     */
    inline KisID id() const { return m_id; }

    /**
     * lcms colorspace type definition.
     */
    void setColorSpaceType(Q_UINT32 type) { m_cmType = type; }
    Q_UINT32 colorSpaceType() { return m_cmType; }

    /**
     * icc profile files colorspace id
     */
    void setColorSpaceSignature(icColorSpaceSignature signature) { m_colorSpaceSignature = signature; }
    icColorSpaceSignature colorSpaceSignature() { return m_colorSpaceSignature; }


    //========== Capabilities =================================================//

    /**
     * Returns the list of user-visible composite ops supported by this colourspace. Internal
     * ops such as COPY, CLEAR, and ERASE, are not included as these make no sense
     * for layers in the full image model.
     */
    virtual KisCompositeOpList userVisiblecompositeOps() const = 0;


    virtual bool valid() { return true; }


    //========== Display profiles =============================================//

    /**
     * Get a list of profiles that apply to this color space
     */
    vKisProfileSP profiles();


    /**
     * Return the number of profiles available for this color space
     */
    Q_INT32 profileCount();



//================= Conversion functions ==================================//


    /**
     * The nativeColor methods take a given color defined as an RGB QColor
     * and fills a byte array with the corresponding color in the
     * the colorspace managed by this strategy.
     *
     * The profile parameter is the profile of the paint device; the other profile
     * is the display profile -- since we are moving from QColor
     * that have most likely been picked from the display itself.
     *
     * XXX: We actually do not use the display yet, nor the paint device profile
     */
    virtual void nativeColor(const QColor& c, Q_UINT8 *dst, KisProfileSP profile = 0);
    virtual void nativeColor(const QColor& c, QUANTUM opacity, Q_UINT8 *dst, KisProfileSP profile = 0);

    /**
     * The toQColor methods take a byte array that is at least pixelSize() long
     * and converts the contents to a QColor, using the given profile as a source
     * profile and the display profile as a destination profile.
     *
     * XXX: We actually do not use the display yet, nor the paint device profile
     *
     */
    virtual void toQColor(const Q_UINT8 *src, QColor *c, KisProfileSP profile= 0 );
    virtual void toQColor(const Q_UINT8 *src, QColor *c, QUANTUM *opacity, KisProfileSP profile = 0);

    /**
     * Get the alpha value of the given pixel.
     * XXX: Change to float/int to match setAlpha() when that changes.
     */
    virtual void getAlpha(const Q_UINT8 *pixel, Q_UINT8 *alpha) = 0;

    virtual KisPixelRO toKisPixelRO(const Q_UINT8 *src, KisProfileSP profile) = 0;
    virtual KisPixel toKisPixel(Q_UINT8 *src, KisProfileSP profile) = 0;

    /**
     * This function is used to convert a KisPixelRepresentation from this color strategy to the specified
     * color strategy.
     */
    virtual bool convertTo(KisPixel& src, KisPixel& dst, Q_INT32 renderingIntent = INTENT_PERCEPTUAL);

    /**
     * Convert the pixels in data to (8-bit BGRA) QImage using the specified profiles.
     * The pixels are supposed to be encoded in this color model.
     *
     * @param data A pointer to a contiguous memory region containing width * height pixels
     * @param width in pixels
     * @param height in pixels
     * @param srcProfile source profile
     * @param dstProfile destination profile
     * @param renderingIntent the rendering intent
     * @param exposure The exposure setting for rendering a preview of a high dynamic range image.
     */
    virtual QImage convertToQImage(const Q_UINT8 *data, Q_INT32 width, Q_INT32 height,
                       KisProfileSP srcProfile, KisProfileSP dstProfile,
                       Q_INT32 renderingIntent = INTENT_PERCEPTUAL,
                       float exposure = 0.0f) = 0;



    /**
     * Convert a byte array of srcLen pixels *src to the specified color space
     * and put the converted bytes into the prepared byte array *dst.
     *
     * Returns false if the conversion failed, true if it succeeded
     */
    virtual bool convertPixelsTo(const Q_UINT8 * src, KisProfileSP srcProfile,
                     Q_UINT8 * dst, KisAbstractColorSpace * dstColorStrategy, KisProfileSP dstProfile,
                     Q_UINT32 numPixels,
                     Q_INT32 renderingIntent = INTENT_PERCEPTUAL);

    /**
     * Convert the value of the channel at the specified position into
     * an 8-bit value. The position is not the number of bytes, but
     * the position of the channel as defined in the channel info list.
     */
    Q_UINT8 scaleToU8(const Q_UINT8 * srcPixel, Q_INT32 channelPos);

    /**
     * Convert the value of the channel at the specified position into
     * a 16-bit value. This may be upscaling or downscaling, depending
     * on the defined value of the channel
     */
    Q_UINT8 scaleToU16(const Q_UINT8 * srcPixel, Q_INT32 channelPos);

//============================== Manipulation fucntions ==========================//


// 
// The manipulation functions have default implementations that _convert_ the pixel
// to a QColor and back. Reimplement these methods in your color strategy!
//

    /**
     * Set the alpha channel to the given value.
     *
     * pixels -- a pointer to the pixels that will have their alpha set to this value
     * alpha --  XXX: This must become int or float
     * nPixels -- the number of pixels
     *
     * XXX: Also add a function that modifies the current alpha with the given alpha, i.e., premultiply them?
     */
    virtual void setAlpha(Q_UINT8 * pixels, Q_UINT8 alpha, Q_INT32 nPixels);


    /**
     * Applies the specified 8-bit alpha mask to the pixels. We assume that there are just
     * as many alpha values as pixels but we do not check this.
     */
    virtual void applyAlphaU8Mask(Q_UINT8 * pixels, Q_UINT8 * alpha, Q_INT32 nPixels);

    /**
     * Applies the inverted 8-bit alpha mask to the pixels. We assume that there are just
     * as many alpha values as pixels but we do not check this.
     */
    virtual void applyInverseAlphaU8Mask(Q_UINT8 * pixels, Q_UINT8 * alpha, Q_INT32 nPixels);

    /**
     * Create an adjustment object for adjusting the brightness and contrast
     * transferValues is a 256 bins array with values from 0 to 0xFFFF
     */
    virtual KisColorAdjustment *createBrightnessContrastAdjustment(Q_UINT16 *transferValues);

    
    /**
     * Apply the adjustment created with onr of the other functions
     */
    virtual void applyAdjustment(const Q_UINT8 *src, Q_UINT8 *dst, KisColorAdjustment *, Q_INT32 nPixels);

    
    // XXX: What with alpha channels? YYY: Add an overloaded function that takes alpha into account?
    /**
     * Get the difference between 2 colors, normalized in the range (0,255)
     */
    virtual Q_INT8 difference(const Q_UINT8* src1, const Q_UINT8* src2);

    /**
     * Mix the colors given their weights and return in dst
     * The sum of weights is assumed 255 */
    virtual void mixColors(const Q_UINT8 **colors, const Q_UINT8 *weights, Q_UINT32 nColors, Q_UINT8 *dst) const;

    /**
     * Convolve the given array of pointers to pixels and return the result
     * in dst. The kernel values are clamped between -128 and 128
     */
    virtual void convolveColors(Q_UINT8** colors, Q_INT32* kernelValues, enumChannelFlags channelFlags, Q_UINT8 *dst, Q_INT32 factor, Q_INT32 offset, Q_INT32 nPixels) const;

    /**
     * Darken all color channels with the given amount. If compensate is true,
     * the compensation factor will be used to limit the darkening.
     *
     * (See the bumpmap filter)
     */
    virtual void darken(const Q_UINT8 * src, Q_UINT8 * dst, Q_INT32 shade, bool compensate, double compensation, Q_INT32 nPixels) const;

    /**
     * Calculate the intensity of the given pixel, scaled down to the range 0-255. XXX: Maybe this should be more flexible
    */
    virtual Q_UINT8 intensity8(const Q_UINT8 * src) const;
    
    /**
     * Compose two arrays of pixels together. If source and target
     * are not the same colour model, the source pixels will be
     * converted to the target model.
     */
    virtual void bitBlt(Q_UINT8 *dst,
                Q_INT32 dststride,
                KisAbstractColorSpace * srcSpace,
                const Q_UINT8 *src,
                Q_INT32 srcRowStride,
                const Q_UINT8 *srcAlphaMask,
                Q_INT32 maskRowStride,
                QUANTUM opacity,
                Q_INT32 rows,
                Q_INT32 cols,
                const KisCompositeOp& op,
                KisProfileSP srcProfile = 0,
                KisProfileSP dstProfile = 0);

//========================== END of Public API ========================================//

protected:


    /**
     * Compose two byte arrays containing pixels in the same color
     * model together.
     */
    virtual void bitBlt(Q_UINT8 *dst,
                Q_INT32 dstRowSize,
                const Q_UINT8 *src,
                Q_INT32 srcRowStride,
                const Q_UINT8 *srcAlphaMask,
                Q_INT32 maskRowStride,
                QUANTUM opacity,
                Q_INT32 rows,
                Q_INT32 cols,
                const KisCompositeOp& op) = 0;

    virtual cmsHTRANSFORM createTransform(KisAbstractColorSpace * dstColorStrategy,
                          KisProfileSP srcProfile,
                          KisProfileSP dstProfile,
                          Q_INT32 renderingIntent);


    /**
     * Return the default profile for this colorspace. This may be 0,
     * if it's the kind of colorstrategy, like CMYK, that doesn't have
     * a default profile.
     */
    KisProfileSP  getDefaultProfile() { return m_defaultProfile; };
    void setDefaultProfile(KisProfileSP profile) { m_defaultProfile = profile; };

protected:

    QStringList m_profileFilenames;
    KisProfileSP m_defaultProfile;
    Q_UINT8 * m_qcolordata; // A small buffer for conversion from and to qcolor.
    Q_INT32 m_alphaPos;
    Q_INT32 m_alphaSize;

    cmsHTRANSFORM m_defaultToRGB;
    cmsHTRANSFORM m_defaultFromRGB;
    cmsHTRANSFORM m_defaultToXYZ;
    cmsHTRANSFORM m_defaultFromXYZ;

    KisProfileSP m_lastUsedSrcProfile;
    KisProfileSP m_lastUsedDstProfile;
    cmsHTRANSFORM m_lastUsedTransform;



private:

    KisID m_id;
    DWORD m_cmType;                           // The colorspace type as defined by littlecms
    icColorSpaceSignature m_colorSpaceSignature; // The colorspace signature as defined in icm/icc files

    typedef QMap<KisProfilePair, cmsHTRANSFORM>  TransformMap;
    TransformMap m_transforms; // Cache for existing transforms


    KisAbstractColorSpace(const KisAbstractColorSpace&);
    KisAbstractColorSpace& operator=(const KisAbstractColorSpace&);

    Q_UINT8 * m_conversionCache;
    Q_UINT32 m_conversionCacheSize;
};

#endif // KIS_STRATEGY_COLORSPACE_H_
