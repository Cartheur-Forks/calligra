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

#include "kis_bookmarked_configurations_model.h"

#include <QList>

#include <kdebug.h>

#include <KoID.h>

#include <kis_bookmarked_configuration_manager.h>

#ifdef Q_CC_MSVC
#include <iso646.h>
#endif

struct KisBookmarkedConfigurationsModel::Private
{
    KisBookmarkedConfigurationManager* bookmarkManager;
    QList<QString> configsKey;
};


KisBookmarkedConfigurationsModel::KisBookmarkedConfigurationsModel(KisBookmarkedConfigurationManager* bm) : d(new Private)
{
    d->bookmarkManager = bm;
    d->configsKey = d->bookmarkManager->configurations();
    qSort( d->configsKey );
}

KisBookmarkedConfigurationsModel::~KisBookmarkedConfigurationsModel()
{
    delete d;
}

int KisBookmarkedConfigurationsModel::rowCount(const QModelIndex &parent ) const
{
    Q_UNUSED(parent);
    return 2 + d->configsKey.size();
}

QVariant KisBookmarkedConfigurationsModel::data(const QModelIndex &index, int role) const
{
    if(not index.isValid())
    {
        return QVariant();
    }
    if(role == Qt::DisplayRole or role == Qt::EditRole)
    {
        switch(index.row())
        {
            case 0:
                return KisBookmarkedConfigurationManager::ConfigDefault.name();
            case 1:
                return KisBookmarkedConfigurationManager::ConfigLastUsed.name();
            default:
                return d->configsKey[ index.row() - 2 ];
        }
    }
    return QVariant();
}

bool KisBookmarkedConfigurationsModel::setData ( const QModelIndex & index, const QVariant & value, int role )
{
    if( role == Qt::EditRole and index.row() >= 2)
    {
        QString name = value.toString();
        int idx = index.row() - 2;
        KisSerializableConfiguration* config = d->bookmarkManager->load( d->configsKey[idx] );
        d->bookmarkManager->remove( d->configsKey[idx]);
        d->bookmarkManager->save(name, config);
        d->configsKey[idx] = name;

        emit(dataChanged(index,index));
        return true;
    }
    return false;
}

KisSerializableConfiguration* KisBookmarkedConfigurationsModel::configuration(const QModelIndex &index) const
{
    if(not index.isValid()) return 0;
    switch(index.row())
    {
        case 0:
            kDebug() << "loading default" << endl;
            return d->bookmarkManager->load( KisBookmarkedConfigurationManager::ConfigDefault.id() );
            break;
        case 1:
            return d->bookmarkManager->load( KisBookmarkedConfigurationManager::ConfigLastUsed.id() );
            break;
        default:
            return d->bookmarkManager->load( d->configsKey[ index.row() - 2 ] );
    }
}

bool KisBookmarkedConfigurationsModel::isIndexDeletable(const QModelIndex &index) const
{
    if(not index.isValid() or index.row() < 2) return false;
    return true;
}

void KisBookmarkedConfigurationsModel::saveConfiguration(QString name, const KisSerializableConfiguration* config)
{
    d->bookmarkManager->save(name, config);
    if(not d->configsKey.contains(name))
    {
        beginInsertRows(QModelIndex(), d->configsKey.count() + 2, d->configsKey.count() +2);
        d->configsKey << name;
        endInsertRows();
    }
}

void KisBookmarkedConfigurationsModel::deleteIndex(const QModelIndex &index)
{
    if(not index.isValid() or index.row() < 2) return ;
    int idx = index.row() - 2;
    d->bookmarkManager->remove( d->configsKey[idx]);
    beginRemoveRows(QModelIndex(), idx + 2, idx + 2);
    d->configsKey.removeAt(idx);
    endRemoveRows();
}

Qt::ItemFlags KisBookmarkedConfigurationsModel::flags(const QModelIndex & index) const
{
    if(not index.isValid()) return 0;
    switch(index.row())
    {
        case 0:
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        case 1:
            if(d->bookmarkManager->exist(KisBookmarkedConfigurationManager::ConfigLastUsed.id()) )
            {
                return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
            } else {
                return 0;
            }
        default:
            return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
    }
}
