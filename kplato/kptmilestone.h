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

#ifndef kptmilestone_h
#define kptmilestone_h

#include "kptnode.h"
#include "defs.h"

class KPTResourceGroup;
class KPTPertCanvas;
class QCanvas;

/**
 * This class is a 0-time node which is dependent on any number of other nodes. 
 */
class KPTMilestone : public KPTNode {
    public:

        KPTMilestone(KPTNode *parent = 0);
        ~KPTMilestone();

        virtual int type() const;
        static int TYPE;
	
        // no children permitted.
        void addChildNode( KPTNode *) {}
        void addResource( KPTResourceGroup *) {}
        void insertResource( KPTResourceGroup *) {}
        void insertChildNode( unsigned int, KPTNode *) {}

        // No child time dependencies permitted

        void addDependChildNode( KPTNode*, TimingType, TimingRelation) {}
        void addDependChildNode( KPTNode*, TimingRelation, KPTDuration) {}
        void insertDependChildNode( unsigned int, KPTNode*, TimingType, TimingRelation) {}


        /** The expected Duration is the expected time to complete a Task, Project, etc. For an 
         *  individual Task, this will calculate the expected duration by querying 
         *  the Distribution of the Task. If the Distribution is a simple RiskNone, the value 
         *  will equal the mode Duration, but for other Distributions like RiskHigh, the value 
         *  will have to be calculated. For a Project or Subproject, the expected Duration is 
         *  calculated by PERT/CPM. 
         */
        KPTDuration *getExpectedDuration() { return new KPTDuration(); }

        /** Instead of using the expected duration, generate a random value using the Distribution of 
         *  each Task. This can be used for Monte-Carlo estimation of Project duration.
         */
        KPTDuration *getRandomDuration() { return getExpectedDuration(); }

        /** Retrive the time this node starts. This is either implied from the set time, or calculated
         *  by asking the parents.
         */
        KPTDuration *getStartTime();

        /** Retrieve the calculated float of this node
         */
        KPTDuration *getFloat();

        /**
         * TODO: Load and save
        */
        virtual bool load(QDomElement &element);
        virtual void save(QDomElement &element) const;

        virtual bool openDialog();
        
        virtual void drawPert(KPTPertCanvas *view, KPTNode *parent = 0);
   
#ifndef NDEBUG
public:
    void printDebug(bool children, QCString indent);
#endif

};
#endif
