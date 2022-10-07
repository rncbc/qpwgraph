// qpwgraph_sect.cpp
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

#include "qpwgraph_sect.h"

#include "qpwgraph_canvas.h"
#include "qpwgraph_connect.h"


//----------------------------------------------------------------------------
// qpwgraph_sect -- Generic graph driver

// Constructor.
qpwgraph_sect::qpwgraph_sect ( qpwgraph_canvas *canvas )
	: QObject(canvas), m_canvas(canvas)
{
}


// Accessors.
qpwgraph_canvas *qpwgraph_sect::canvas (void) const
{
	return m_canvas;
}


// Generic sect/graph methods.
void qpwgraph_sect::addItem ( qpwgraph_item *item, bool is_new )
{
	if (is_new)
		m_canvas->addItem(item);

	if (item->type() == qpwgraph_connect::Type) {
		qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
		if (connect)
			m_connects.append(connect);
	}
}


void qpwgraph_sect::removeItem ( qpwgraph_item *item )
{
	if (item->type() == qpwgraph_connect::Type) {
		qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
		if (connect) {
			connect->disconnect();
			m_connects.removeAll(connect);
		}
	}

	m_canvas->removeItem(item);
}


// Clean-up all un-marked items...
void qpwgraph_sect::resetItems ( uint node_type )
{
	const QList<qpwgraph_connect *> connects(m_connects);

	foreach (qpwgraph_connect *connect, connects) {
		if (connect->isMarked()) {
			connect->setMarked(false);
		} else {
			removeItem(connect);
			delete connect;
		}
	}

	m_canvas->resetNodes(node_type);
}


void qpwgraph_sect::clearItems ( uint node_type )
{
	qpwgraph_sect::resetItems(node_type);

//	qDeleteAll(m_connects);
	m_connects.clear();

	m_canvas->clearNodes(node_type);
}


// Special node finder.
qpwgraph_node *qpwgraph_sect::findNode (
	uint id, qpwgraph_item::Mode mode, int type ) const
{
	return m_canvas->findNode(id, mode, type);
}


// Client/port renaming method.
void qpwgraph_sect::renameItem (
	qpwgraph_item *item, const QString& name )
{
	qpwgraph_node *node = nullptr;

	if (item->type() == qpwgraph_node::Type) {
		qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
		if (node)
			node->setNodeTitle(name);
	}
	else
	if (item->type() == qpwgraph_port::Type) {
		qpwgraph_port *port = static_cast<qpwgraph_port *> (item);
		if (port)
			node = port->portNode();
		if (port && node)
			port->setPortTitle(name);
	}

	if (node)
		node->updatePath();
}


// end of qpwgraph_sect.cpp
