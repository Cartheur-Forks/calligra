/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2007 Sven Langkamp <sven.langkamp@gmail.com>
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
#include <QString>
#include <QBitArray>

#include <klocale.h>

#include "KoColorSpaceRegistry.h"
#include "KoColor.h"
#include "KoColorProfile.h"
#include "KoColorSpace.h"

#include "kis_image_commands.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_group_layer.h"
#include "kis_undo_adapter.h"

KisImageCommand::KisImageCommand(const QString& name, KisImageSP image)
    : QUndoCommand(name)
    , m_image(image)
{
}

KisImageCommand::~KisImageCommand()
{
}


void KisImageCommand::setUndo(bool undo)
{
    if (m_image->undoAdapter()) {
        m_image->undoAdapter()->setUndo(undo);
    }
}



KisImageLockCommand::KisImageLockCommand(KisImageSP image, bool lockImage)
    : KisImageCommand("lock image", image)  // Not for translation, this is only ever used inside a macro command.
{
    m_lockImage = lockImage;
}

void KisImageLockCommand::redo()
{
    setUndo(false);
    if (m_lockImage) {
        m_image->lock();
    } else {
        m_image->unlock();
    }
    setUndo(true);
}

void KisImageLockCommand::undo()
{
    setUndo(false);
    if (m_lockImage) {
        m_image->unlock();
    } else {
        m_image->lock();
    }
    setUndo(true);
}



KisImageResizeCommand::KisImageResizeCommand(KisImageSP image, qint32 width, qint32 height, qint32 oldWidth, qint32 oldHeight)
    : KisImageCommand(i18n("Resize Image"), image)
{
    m_before = QSize(oldWidth, oldHeight);
    m_after = QSize(width, height);
}

void KisImageResizeCommand::redo()
{
    setUndo(false);
    m_image->resize(m_after.width(), m_after.height());
    setUndo(true);
}

void KisImageResizeCommand::undo()
{
    setUndo(false);
    m_image->resize(m_before.width(), m_before.height());
    setUndo(true);
}



KisImageConvertTypeCommand::KisImageConvertTypeCommand(KisImageSP image, const KoColorSpace * beforeColorSpace, const KoColorSpace * afterColorSpace)
    : KisImageCommand(i18n("Convert Image Type"), image)
{
    m_beforeColorSpace = beforeColorSpace;
    m_afterColorSpace = afterColorSpace;
}

void KisImageConvertTypeCommand::redo()
{
    setUndo(false);
    m_image->setColorSpace(m_afterColorSpace);
    m_image->setProfile(m_afterColorSpace->profile());
    setUndo(true);
}

void KisImageConvertTypeCommand::undo()
{
    setUndo(false);
    m_image->setColorSpace(m_beforeColorSpace);
    m_image->setProfile(m_beforeColorSpace->profile());
    setUndo(true);
}



KisImagePropsCommand::KisImagePropsCommand(KisImageSP image, const KoColorSpace* newColorSpace, const KoColorProfile* newProfile)
    : KisImageCommand(i18n("Property Changes"), image)
    , m_newColorSpace( newColorSpace )
    , m_newProfile(newProfile)
{
    m_oldColorSpace = m_image->colorSpace();
    m_oldProfile = m_image->profile();
}

void KisImagePropsCommand::redo()
{
    setUndo(false);
    m_image->setColorSpace(m_newColorSpace);
    m_image->setProfile(m_newProfile);
    setUndo(true);
}

void KisImagePropsCommand::undo()
{
    setUndo(false);
    m_image->setColorSpace(m_oldColorSpace);
    m_image->setProfile(m_oldProfile);
    setUndo(true);
}



KisImageChangeLayersCommand::KisImageChangeLayersCommand(KisImageSP image, KisNodeSP oldRootLayer, KisNodeSP newRootLayer, const QString& name)
    : KisImageCommand(name, image)
{
    m_oldRootLayer = oldRootLayer;
    m_newRootLayer = newRootLayer;
}

void KisImageChangeLayersCommand::redo()
{
    setUndo(false);
    m_image->setRootLayer( static_cast<KisGroupLayer*>( m_newRootLayer.data() ) );
    m_image->notifyLayersChanged();
    setUndo(true);
}

void KisImageChangeLayersCommand::undo()
{
    setUndo(false);
    m_image->setRootLayer( static_cast<KisGroupLayer*>( m_oldRootLayer.data() ) );
    m_image->notifyLayersChanged();
    setUndo(true);
}



KisImageLayerAddCommand::KisImageLayerAddCommand(KisImageSP image, KisNodeSP layer)
    : KisImageCommand(i18n("Add Layer"), image)
{
    m_layer = layer;
    m_parent = layer->parent();
    m_aboveThis = layer->nextSibling();
}

void KisImageLayerAddCommand::redo()
{
    m_image->addNode(m_layer, m_parent, m_aboveThis);
}

void KisImageLayerAddCommand::undo()
{
    m_image->removeNode(m_layer);
}



KisImageLayerRemoveCommand::KisImageLayerRemoveCommand(KisImageSP image, KisNodeSP layer, KisNodeSP wasParent, KisNodeSP wasAbove)
    : KisImageCommand(i18n("Remove Layer"), image)
{
    m_layer = layer;
    m_prevParent = wasParent;
    m_prevAbove = wasAbove;
}


void KisImageLayerRemoveCommand::redo()
{
    m_image->removeNode(m_layer);
}

void KisImageLayerRemoveCommand::undo()
{
    m_image->addNode(m_layer, m_prevParent, m_prevAbove);
}



KisImageLayerMoveCommand::KisImageLayerMoveCommand(KisImageSP image, KisNodeSP layer, KisNodeSP wasParent, KisNodeSP wasAbove)
    : KisImageCommand(i18n("Move Layer"), image)
{
    m_layer = layer;
    m_prevParent = wasParent;
    m_prevAbove = wasAbove;
    m_newParent = layer->parent();
    m_newAbove = layer->nextSibling();
}

void KisImageLayerMoveCommand::redo()
{
    m_image->moveNode(m_layer, m_newParent, m_newAbove);
}

void KisImageLayerMoveCommand::undo()
{
    m_image->moveNode(m_layer, m_prevParent, m_prevAbove);
}




KisImageLayerPropsCommand::KisImageLayerPropsCommand(KisImageSP image, KisLayerSP layer, qint32 opacity, const KoCompositeOp* compositeOp, const QString& name, QBitArray channelFlags )
    : KisImageCommand(i18n("Property Changes"), image)
{
    m_layer = layer;
    m_name = name;
    m_opacity = opacity;
    m_compositeOp = compositeOp;
    m_channelFlags = channelFlags;
}

KisImageLayerPropsCommand::~KisImageLayerPropsCommand()
{
}

void KisImageLayerPropsCommand::redo()
{
    QString name = m_layer->name();
    qint32 opacity = m_layer->opacity();
    const KoCompositeOp* compositeOp = m_layer->compositeOp();
    QBitArray channelFlags = m_layer->channelFlags();

    m_layer->setName(name);
    m_layer->setOpacity(opacity);
    m_layer->setCompositeOp(compositeOp);
    m_layer->setChannelFlags( channelFlags );

    m_compositeOp = compositeOp;
    m_channelFlags = channelFlags;
    m_name = name;
    m_opacity = opacity;

    m_layer->setDirty();

}

void KisImageLayerPropsCommand::undo()
{
    redo();
}
