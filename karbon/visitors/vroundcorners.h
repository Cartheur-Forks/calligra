/* This file is part of the KDE project
   Copyright (C) 2002, The Karbon Developers
*/

#ifndef __VROUNDCORNERS_H__
#define __VROUNDCORNERS_H__


#include "vvisitor.h"


class VSegment;


class VRoundCorners : public VVisitor
{
public:
	VRoundCorners( double radius ) { m_radius = radius; }

	double radius() const { return m_radius; }
	void setRadius( double radius );

	virtual void visitVPath( VPath& path );
	virtual void visitVSegmentList( VSegmentList& segmentList );

private:
	/// The "radius" of the round corners.
	double m_radius;
};

#endif

