/* This file is part of the KDE project
   Copyright (C) 2001 Thomas Zander zander@kde.org

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef kptduration_h
#define kptduration_h

#include <qdatetime.h>

class KPTDuration;

/**
 * The duration class can be used to store a timespan in a convenient format.
 * The timespan can be in length in many many hours down to miliseconds.
 */
class KPTDuration {
    public:
        KPTDuration();
        KPTDuration(const KPTDuration &d);
        KPTDuration(int h, int m, int s=0, int ms=0);
        KPTDuration(const QTime time);
        KPTDuration(const QDateTime time);
        KPTDuration(int seconds);
        ~KPTDuration();

        void add(KPTDuration time);
        void add(KPTDuration *time);
        void subtract(KPTDuration time);
        void subtract(KPTDuration *time);
        void const set(KPTDuration newTime);
        void const set(QDateTime newTime);

        int duration() const { return zero.secsTo(m_theTime); }

        bool   operator==( const KPTDuration &d ) const { return m_theTime == d.m_theTime; }
        bool   operator!=( const KPTDuration &d ) const { return m_theTime != d.m_theTime; }
        bool   operator<( const KPTDuration &d ) const { return m_theTime < d.m_theTime; }
        bool   operator<=( const KPTDuration &d ) const { return m_theTime <= d.m_theTime; }
        bool   operator>( const KPTDuration &d ) const { return m_theTime > d.m_theTime; }
        bool   operator>=( const KPTDuration &d ) const { return m_theTime >= d.m_theTime; }
        KPTDuration &operator = ( const KPTDuration &d ) { set(d); return *this;}

        QString toString() const { return m_theTime.toString(); }

        QDateTime dateTime() const { return m_theTime; }
        QDate date() const { return m_theTime.date(); }
        QTime time() const { return m_theTime.time(); }

    /**
     * This is useful for occasions where we need a zero duration.
     */
    static const KPTDuration zeroDuration;

    private:
        QDateTime zero, m_theTime;
};
#endif
