/*
 *  Copyright (c) 1999 Michael Koch    <koch@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#if !defined KIS_COMMAND_H_
#define KIS_COMMAND_H_

#include <qstring.h>
#include <kcommand.h>

class KisDoc;

class KisCommand : public KCommand {
public:
	KisCommand(KisDoc *doc);
	KisCommand(const QString& name, KisDoc *doc);
	virtual ~KisCommand();

	virtual void execute() = 0;
	virtual void unexecute() = 0;
	virtual QString name() const;

protected:
	KisDoc *m_doc;
	QString m_name;
};

#endif // KIS_COMMAND_H_


