/* This file is part of the KDE project
   Copyright (C) 2001, The Karbon Developers
   Copyright (C) 2002, The Karbon Developers

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qdom.h>

#include <koRect.h>

#include "vcomposite.h"
#include "vfill.h"
#include "vgroup.h"
#include "vlayer.h"
#include "vstroke.h"
#include "vvisitor.h"
#ifdef HAVE_KARBONTEXT
#include "vtext.h"
#endif

#include <kdebug.h>


VGroup::VGroup( VObject* parent, VState state )
	: VObject( parent, state )
{
	m_stroke = new VStroke( this );
	m_fill = new VFill();
}

VGroup::VGroup( const VGroup& group )
	: VObject( group )
{
	m_stroke = new VStroke( *group.m_stroke );
	m_stroke->setParent( this );
	m_fill = new VFill( *group.m_fill );

	VObjectListIterator itr = group.m_objects;
	for ( ; itr.current() ; ++itr )
		append( itr.current()->clone() );
}

VGroup::~VGroup()
{
	VObjectListIterator itr = m_objects;
	for ( ; itr.current(); ++itr )
	{
		delete( itr.current() );
	}
}

void
VGroup::draw( VPainter* painter, const KoRect* rect ) const
{
	if(
		state() == deleted ||
		state() == hidden ||
		state() == hidden_locked )
	{
		return;
	}

	VObjectListIterator itr = m_objects;

	for ( ; itr.current(); ++itr )
		itr.current()->draw( painter, rect );
}

void
VGroup::transform( const QWMatrix& m )
{
	VObjectListIterator itr = m_objects;

	for ( ; itr.current() ; ++itr )
		itr.current()->transform( m );
}

const KoRect&
VGroup::boundingBox() const
{
	if( m_boundingBoxIsInvalid )
	{
		// clear:
		m_boundingBox = KoRect();

		VObjectListIterator itr = m_objects;
		for( ; itr.current(); ++itr )
		{
			m_boundingBox |= itr.current()->boundingBox();
		}

		m_boundingBoxIsInvalid = false;
	}

	return m_boundingBox;
}

VGroup*
VGroup::clone() const
{
	return new VGroup( *this );
}

void
VGroup::setFill( const VFill& fill )
{
	VObjectListIterator itr = m_objects;

	for ( ; itr.current() ; ++itr )
		itr.current()->setFill( fill );

	VObject::setFill( fill );
}

void
VGroup::setStroke( const VStroke& stroke )
{
	VObjectListIterator itr = m_objects;

	for ( ; itr.current() ; ++itr )
		itr.current()->setStroke( stroke );

	VObject::setStroke( stroke );
}

void
VGroup::setState( const VState state )
{
	VObjectListIterator itr = m_objects;

	for ( ; itr.current() ; ++itr )
		if( itr.current()->state() != VObject::deleted  )
			itr.current()->setState( state );

	VObject::setState( state );
}

void
VGroup::save( QDomElement& element ) const
{
	QDomElement me = element.ownerDocument().createElement( "GROUP" );
	element.appendChild( me );

	// save objects:
	VObjectListIterator itr = m_objects;

	for ( ; itr.current(); ++itr )
		itr.current()->save( me );
}

void
VGroup::load( const QDomElement& element )
{
	m_objects.setAutoDelete( true );
	m_objects.clear();
	m_objects.setAutoDelete( false );

	QDomNodeList list = element.childNodes();
	for( uint i = 0; i < list.count(); ++i )
	{
		if( list.item( i ).isElement() )
		{
			QDomElement e = list.item( i ).toElement();

			if( e.tagName() == "COMPOSITE" )
			{
				VComposite* composite = new VComposite( this );
				composite->load( e );
				append( composite );
			}
			else if( e.tagName() == "GROUP" )
			{
				VGroup* group = new VGroup( this );
				group->load( e );
				append( group );
			}
			else if( e.tagName() == "TEXT" )
			{
#ifdef HAVE_KARBONTEXT
				VText *text = new VText( this );
				text->load( e );
				append( text );
#endif
			}
		}
	}
}

void
VGroup::accept( VVisitor& visitor )
{
	visitor.visitVGroup( *this );
}


void
VGroup::take( const VObject& object )
{
	m_objects.removeRef( &object );

	invalidateBoundingBox();
}

void
VGroup::append( VObject* object )
{
	object->setParent( this );

	m_objects.append( object );

	invalidateBoundingBox();
}

void
VGroup::insertInfrontOf( VObject* newObject, VObject* oldObject )
{
	m_objects.insert( m_objects.find( oldObject ), newObject );
}

void
VGroup::clear()
{
	m_objects.clear();

	invalidateBoundingBox();
}

