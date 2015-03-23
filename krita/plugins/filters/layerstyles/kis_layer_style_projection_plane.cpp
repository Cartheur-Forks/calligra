/*
 *  Copyright (c) 2015 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_layer_style_projection_plane.h"

#include "kis_layer_style_filter_projection_plane.h"
#include "kis_psd_layer_style.h"

#include "kis_ls_drop_shadow_filter.h"
#include "kis_ls_satin_filter.h"
#include "kis_ls_overlay_filter.h"
#include "kis_ls_stroke_filter.h"


struct KisLayerStyleProjectionPlane::Private
{
    KisAbstractProjectionPlaneWSP sourceProjectionPlane;

    QVector<KisAbstractProjectionPlaneSP> stylesBefore;
    QVector<KisAbstractProjectionPlaneSP> stylesAfter;

};

KisLayerStyleProjectionPlane::KisLayerStyleProjectionPlane(KisLayer *sourceLayer)
    : m_d(new Private)
{
    KisPSDLayerStyleSP style = sourceLayer->layerStyle();

    KIS_ASSERT_RECOVER(style) {
        style = toQShared(new KisPSDLayerStyle());
    }

    init(sourceLayer, style);
}

// for testing purposes only!
KisLayerStyleProjectionPlane::KisLayerStyleProjectionPlane(KisLayer *sourceLayer, KisPSDLayerStyleSP layerStyle)
    : m_d(new Private)
{
    init(sourceLayer, layerStyle);
}

void KisLayerStyleProjectionPlane::init(KisLayer *sourceLayer, KisPSDLayerStyleSP style)
{
    m_d->sourceProjectionPlane = sourceLayer->internalProjectionPlane();

    {
        KisLayerStyleFilterProjectionPlane *dropShadow =
            new KisLayerStyleFilterProjectionPlane(sourceLayer);
        dropShadow->setStyle(new KisLsDropShadowFilter(KisLsDropShadowFilter::DropShadow), style);
        m_d->stylesBefore << toQShared(dropShadow);
    }

    {
        KisLayerStyleFilterProjectionPlane *innerShadow =
            new KisLayerStyleFilterProjectionPlane(sourceLayer);
        innerShadow->setStyle(new KisLsDropShadowFilter(KisLsDropShadowFilter::InnerShadow), style);
        m_d->stylesAfter << toQShared(innerShadow);
    }

    {
        KisLayerStyleFilterProjectionPlane *outerGlow =
            new KisLayerStyleFilterProjectionPlane(sourceLayer);
        outerGlow->setStyle(new KisLsDropShadowFilter(KisLsDropShadowFilter::OuterGlow), style);
        m_d->stylesAfter << toQShared(outerGlow);
    }

    {
        KisLayerStyleFilterProjectionPlane *innerGlow =
            new KisLayerStyleFilterProjectionPlane(sourceLayer);
        innerGlow->setStyle(new KisLsDropShadowFilter(KisLsDropShadowFilter::InnerGlow), style);
        m_d->stylesAfter << toQShared(innerGlow);
    }

    {
        KisLayerStyleFilterProjectionPlane *satin =
            new KisLayerStyleFilterProjectionPlane(sourceLayer);
        satin->setStyle(new KisLsSatinFilter(), style);
        m_d->stylesAfter << toQShared(satin);
    }

    {
        KisLayerStyleFilterProjectionPlane *colorOverlay =
            new KisLayerStyleFilterProjectionPlane(sourceLayer);
        colorOverlay->setStyle(new KisLsOverlayFilter(KisLsOverlayFilter::Color), style);
        m_d->stylesAfter << toQShared(colorOverlay);
    }

    {
        KisLayerStyleFilterProjectionPlane *gradientOverlay =
            new KisLayerStyleFilterProjectionPlane(sourceLayer);
        gradientOverlay->setStyle(new KisLsOverlayFilter(KisLsOverlayFilter::Gradient), style);
        m_d->stylesAfter << toQShared(gradientOverlay);
    }

    {
        KisLayerStyleFilterProjectionPlane *patternOverlay =
            new KisLayerStyleFilterProjectionPlane(sourceLayer);
        patternOverlay->setStyle(new KisLsOverlayFilter(KisLsOverlayFilter::Pattern), style);
        m_d->stylesAfter << toQShared(patternOverlay);
    }

    {
        KisLayerStyleFilterProjectionPlane *stroke =
            new KisLayerStyleFilterProjectionPlane(sourceLayer);
        stroke->setStyle(new KisLsStrokeFilter(), style);
        m_d->stylesAfter << toQShared(stroke);
    }
}

KisLayerStyleProjectionPlane::~KisLayerStyleProjectionPlane()
{
}

KisAbstractProjectionPlaneSP KisLayerStyleProjectionPlane::factoryObject(KisLayer *sourceLayer)
{
    return toQShared(new KisLayerStyleProjectionPlane(sourceLayer));
}

QRect KisLayerStyleProjectionPlane::recalculate(const QRect& rect, KisNodeSP filthyNode)
{
    KisAbstractProjectionPlaneSP sourcePlane = m_d->sourceProjectionPlane.toStrongRef();
    return sourcePlane->recalculate(rect, filthyNode);
}

void KisLayerStyleProjectionPlane::apply(KisPainter *painter, const QRect &rect)
{
    KisAbstractProjectionPlaneSP sourcePlane = m_d->sourceProjectionPlane.toStrongRef();

    foreach (const KisAbstractProjectionPlaneSP plane, m_d->stylesBefore) {
        plane->apply(painter, rect);
    }

    sourcePlane->apply(painter, rect);

    foreach (const KisAbstractProjectionPlaneSP plane, m_d->stylesAfter) {
        plane->apply(painter, rect);
    }
}

QRect KisLayerStyleProjectionPlane::needRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    KisAbstractProjectionPlaneSP sourcePlane = m_d->sourceProjectionPlane.toStrongRef();
    return sourcePlane->needRect(rect, pos);
}

QRect KisLayerStyleProjectionPlane::changeRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    KisAbstractProjectionPlaneSP sourcePlane = m_d->sourceProjectionPlane.toStrongRef();
    QRect layerChangeRect = sourcePlane->changeRect(rect, pos);
    QRect changeRect = layerChangeRect;

    foreach (const KisAbstractProjectionPlaneSP plane, m_d->stylesBefore) {
        changeRect |= plane->changeRect(layerChangeRect, KisLayer::N_ABOVE_FILTHY);
    }

    foreach (const KisAbstractProjectionPlaneSP plane, m_d->stylesAfter) {
        changeRect |= plane->changeRect(layerChangeRect, KisLayer::N_ABOVE_FILTHY);
    }

    return changeRect;
}

QRect KisLayerStyleProjectionPlane::accessRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    KisAbstractProjectionPlaneSP sourcePlane = m_d->sourceProjectionPlane.toStrongRef();
    QRect accessRect = sourcePlane->accessRect(rect, pos);

    foreach (const KisAbstractProjectionPlaneSP plane, m_d->stylesBefore) {
        accessRect |= plane->accessRect(rect, KisLayer::N_ABOVE_FILTHY);
    }

    foreach (const KisAbstractProjectionPlaneSP plane, m_d->stylesAfter) {
        accessRect |= plane->accessRect(rect, KisLayer::N_ABOVE_FILTHY);
    }

    return accessRect;
}

