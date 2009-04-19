/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_RECORDED_ACTION_H_
#define _KIS_RECORDED_ACTION_H_

class QDomDocument;
class QDomElement;
class QString;
class KisUndoAdapter;

#include <krita_export.h>
#include <kis_types.h>

class KRITAIMAGE_EXPORT KisRecordedAction
{
public:
    KisRecordedAction(const QString& name, const QString& id);
    KisRecordedAction(const KisRecordedAction&);
    virtual ~KisRecordedAction();
    virtual void play(KisUndoAdapter* adapter = 0) const = 0;
    virtual KisRecordedAction* clone() const = 0;
    virtual void toXML(QDomDocument& doc, QDomElement& elt) const;
public:
    const QString& id() const;
    const QString& name() const;
    void setName(const QString& name);
    /**
     * @return a widget that allow to edit this action, or 0 if this action is not editable
     */
    virtual QWidget* createEditor(QWidget* parent);
protected:
    static QString nodeToIndexPath(KisNodeSP node);
private:
    struct Private;
    Private* const d;
};

class KRITAIMAGE_EXPORT KisRecordedActionFactory
{
public:
    KisRecordedActionFactory(QString id);
    virtual ~KisRecordedActionFactory();
    virtual KisRecordedAction* fromXML(KisImageSP img, const QDomElement& elt) = 0;
    QString id() const;
    QString name() const;
protected:
    static KisNodeSP indexPathToNode(KisImageSP img, const QString &);
private:
    struct Private;
    Private* const d;
};

#endif
