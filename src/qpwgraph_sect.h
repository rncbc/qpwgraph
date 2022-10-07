// qpwgraph_sect.h
//
/****************************************************************************
   Copyright (C) 2021-2022, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#ifndef __qpwgraph_sect_h
#define __qpwgraph_sect_h

#include "qpwgraph_node.h"

#include <QObject>
#include <QList>


// Forwards decls.
class qpwgraph_canvas;


//----------------------------------------------------------------------------
// qpwgraph_sect -- Generic graph driver

class qpwgraph_sect : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	qpwgraph_sect(qpwgraph_canvas *canvas);

	// Accessors.
	qpwgraph_canvas *canvas() const;

	// Generic sect/graph methods.
	void addItem(qpwgraph_item *item, bool is_new = true);
	void removeItem(qpwgraph_item *item);

	// Clean-up all un-marked items...
	void resetItems(uint node_type);
	void clearItems(uint node_type);

	// Special node finder.
	qpwgraph_node *findNode(uint id, qpwgraph_item::Mode mode, int type = 0) const;

	// Client/port renaming method.
	virtual void renameItem(qpwgraph_item *item, const QString& name);

private:

	// Instance variables.
	qpwgraph_canvas *m_canvas;

	QList<qpwgraph_connect *> m_connects;
};


#endif	// __qpwgraph_sect_h

// end of qpwgraph_sect.h
