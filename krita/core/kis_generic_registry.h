/*
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

#ifndef _KIS_GENERIC_REGISTRY_H_
#define _KIS_GENERIC_REGISTRY_H_

#include <map>

#include <qstring.h>
#include <qstringlist.h>
#include <kdebug.h>

#include "kis_global.h"
#include "kis_id.h"

template<typename _T>
class KisGenericRegistry {
	typedef std::map<KisID, _T> storageMap;
public:
	KisGenericRegistry() { };
	virtual ~KisGenericRegistry() { };
public:
	void add(_T item)
	{
		m_storage.insert( typename storageMap::value_type( item->id(), item) );
	}

	void add(KisID id, _T item)
	{
		m_storage.insert(typename storageMap::value_type(id, item));
		kdDebug() << "Added ID: " << id.id() << ", " << id.name() << "\n";
	}
	
	_T get(const KisID& name) const
	{
		_T p;
		typename storageMap::const_iterator it = m_storage.find(name);
		if (it != m_storage.end()) {
			p = it -> second;
		}
		if (!p) {
			kdDebug() << "No item " << name.id() << ", " << name.name() << " found\n";
			return 0;
		}
		return p;
	}

	/**
	 * Get a single entry based on the identifying part of KisID, not the
	 * the descriptive part.
	 */
	_T get(const QString& id) const
	{
		return get(KisID(id, ""));
	}


	bool exists(const KisID& id) const
	{
		typename storageMap::const_iterator it = m_storage.find(id);
		return (it != m_storage.end());
	}

	bool exists(const QString& id) const
	{
		return exists(KisID(id, ""));
	}


	KisIDList listKeys() const
	{
		KisIDList list;
		typename storageMap::const_iterator it = m_storage.begin();
		typename storageMap::const_iterator endit = m_storage.end();
		while( it != endit )
		{
			list.append(it->first);
			++it;
		}
		return list;
	}

protected:
	KisGenericRegistry(const KisGenericRegistry&) { };
	KisGenericRegistry operator=(const KisGenericRegistry&) { };
	storageMap m_storage;
};

#endif
