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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef KIS_CHANNELINFO_H_
#define KIS_CHANNELINFO_H_

#include "qstring.h"
#include "ksharedptr.h"

enum enumChannelType {
    COLOR, // The channel represents a color
    ALPHA, // The channel represents the opacity of a pixel
    SUBSTANCE, // The channel represents a real-world substance like pigments or medium
    SUBSTRATE // The channel represents a real-world painting substrate like a canvas
};


enum enumChannelFlags {
    FLAG_COLOR = 1,
    FLAG_ALPHA = (1 << 1),
    FLAG_SUBSTANCE = (1 << 2),
    FLAG_SUBSTRATE = (1 << 3)
};


/** 
 * This class gives some basic information about a channel,
 * that is, one of the components that makes up a particular 
 * pixel.
 */
class KisChannelInfo : public KShared {
public:
    KisChannelInfo() { };
    KisChannelInfo( const QString & name, Q_INT32 npos, enumChannelType channelType, Q_INT32 size = 1)
        : m_name (name), m_pos (npos), m_channelType(channelType), m_size(size) { };
public:
    /**
     * User-friendly name for this channel for presentation purposes in the gui
     */
    inline QString name() const { return m_name; };
    
    /** 
     * returns the position of the first byte of the channel in the pixel
     */
    inline Q_INT32 pos() const { return m_pos; };
    
    /**
     * returns the number of bytes this channel takes
     */
    inline Q_INT32 size() const { return m_size; };

    /**
     * returns the type of the channel
     */
    inline enumChannelType channelType() const { return m_channelType; };
    
private:
    QString m_name;
    Q_INT32 m_pos;
    enumChannelType m_channelType;
    Q_INT32 m_size;
};

#endif // KIS_CHANNELINFO_H_
