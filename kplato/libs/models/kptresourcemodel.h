/* This file is part of the KDE project
  Copyright (C) 2007 Dag Andersen <kplato@kde.org>

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
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#ifndef KPTRESOURCEMODEL_H
#define KPTRESOURCEMODEL_H

#include "kplatomodels_export.h"

#include <kptitemmodelbase.h>

class QPoint;

namespace KPlato
{

class Project;
class Resource;
class ResourceGroup;
class Calendar;

class KPLATOMODELS_EXPORT ResourceItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit ResourceItemModel( QObject *parent = 0 );
    ~ResourceItemModel();

    virtual void setProject( Project *project );

    virtual Qt::ItemFlags flags( const QModelIndex & index ) const;

    virtual QModelIndex parent( const QModelIndex & index ) const;
    virtual QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex index( const ResourceGroup *group ) const;
    QModelIndex index( const Resource *resource ) const;

    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const; 
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const; 

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const; 
    virtual bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );


    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    virtual QStringList mimeTypes () const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );

    QItemDelegate *createDelegate( int col, QWidget *parent ) const;
    
    QObject *object( const QModelIndex &index ) const;
    QModelIndex insertGroup( ResourceGroup *g );
    QModelIndex insertResource( ResourceGroup *g, Resource *r, Resource *after = 0 );

protected slots:
    void slotResourceChanged( Resource* );
    void slotResourceGroupChanged( ResourceGroup * );
    void slotResourceGroupToBeInserted( const ResourceGroup *group, int row );
    void slotResourceGroupInserted( const ResourceGroup *group );
    void slotResourceGroupToBeRemoved( const ResourceGroup *group );
    void slotResourceGroupRemoved( const ResourceGroup *group );
    void slotResourceToBeInserted( const ResourceGroup *group, int row );
    void slotResourceInserted( const Resource *resource );
    void slotResourceToBeRemoved( const Resource *resource );
    void slotResourceRemoved( const Resource *resource );
    void slotCalendarChanged( Calendar* cal );
    
protected:
    QVariant notUsed( const ResourceGroup *res, int role ) const;
    
    QVariant name( const Resource *res, int role ) const;
    QVariant name( const ResourceGroup *res, int role ) const;
    bool setName( Resource *res, const QVariant &value, int role );
    bool setName( ResourceGroup *res, const QVariant &value, int role );
    
    QVariant type( const Resource *res, int role ) const;
    QVariant type( const ResourceGroup *res, int role ) const;
    bool setType( Resource *res, const QVariant &value, int role );
    bool setType( ResourceGroup *res, const QVariant &value, int role );

    QVariant initials( const Resource *res, int role ) const;
    bool setInitials( Resource *res, const QVariant &value, int role );
    
    QVariant email( const Resource *res, int role ) const;
    bool setEmail( Resource *res, const QVariant &value, int role );
    
    QVariant calendar( const Resource *res, int role ) const;
    bool setCalendar( Resource *res, const QVariant &value, int role );

    QVariant units( const Resource *res, int role ) const;
    bool setUnits( Resource *res, const QVariant &value, int role );

    QVariant availableFrom( const Resource *res, int role ) const;
    bool setAvailableFrom( Resource *res, const QVariant &value, int role );

    QVariant availableUntil( const Resource *res, int role ) const;
    bool setAvailableUntil( Resource *res, const QVariant &value, int role );

    QVariant normalRate( const Resource *res, int role ) const;
    bool setNormalRate( Resource *res, const QVariant &value, int role );

    QVariant overtimeRate( const Resource *res, int role ) const;
    bool setOvertimeRate( Resource *res, const QVariant &value, int role );

    QVariant fixedCost( const Resource *res, int role ) const;
    bool setFixedCost( Resource *res, const QVariant &value, int role );

private:
    ResourceGroup *m_group; // Used for sanity checks
    Resource *m_resource; // Used for sanity checks
};


}  //KPlato namespace

#endif
