// qpwgraph_item.cpp
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

#include "qpwgraph_item.h"

#include "qpwgraph_node.h"
#include "qpwgraph_port.h"
#include "qpwgraph_connect.h"

#include <QPalette>


//----------------------------------------------------------------------------
// qpwgraph_item -- Base graphics item.

// Constructor.
qpwgraph_item::qpwgraph_item ( QGraphicsItem *parent )
	: QGraphicsPathItem(parent), m_marked(false), m_hilite(false)
{
	const QPalette pal;
	m_foreground = pal.buttonText().color();
	m_background = pal.button().color();
}


// Basic color accessors.
void qpwgraph_item::setForeground ( const QColor& color )
{
	m_foreground = color;
}

const QColor& qpwgraph_item::foreground (void) const
{
	return m_foreground;
}


void qpwgraph_item::setBackground ( const QColor& color )
{
	m_background = color;
}

const QColor& qpwgraph_item::background (void) const
{
	return m_background;
}


// Marking methods.
void qpwgraph_item::setMarked ( bool marked )
{
	m_marked = marked;
}


bool qpwgraph_item::isMarked (void) const
{
	return m_marked;
}


// Highlighting methods.
void qpwgraph_item::setHighlight ( bool hilite )
{
	m_hilite = hilite;

	if (m_hilite)
		raise();

	QGraphicsPathItem::update();
}


bool qpwgraph_item::isHighlight (void) const
{
	return m_hilite;
}


// Raise item z-value (dynamic always-on-top).
void qpwgraph_item::raise (void)
{
	static qreal s_zvalue = 0.0;

	switch (type()) {
	case  qpwgraph_port::Type: {
		QGraphicsPathItem::setZValue(s_zvalue += 0.003);
		qpwgraph_port *port = static_cast<qpwgraph_port *> (this);
		if (port) {
			qpwgraph_node *node = port->portNode();
			if (node)
				node->setZValue(s_zvalue += 0.002);
		}
		break;
	}
	case qpwgraph_connect::Type:
	default:
		QGraphicsPathItem::setZValue(s_zvalue += 0.001);
		break;
	}
}


// Item-type hash (static)
uint qpwgraph_item::itemType ( const QByteArray& type_name )
{
	return qHash(type_name);
}


// Rectangular editor extents (virtual)
QRectF qpwgraph_item::editorRect (void) const
{
	return QRectF();
}


// Path and bounding rectangle override.
void qpwgraph_item::setPath ( const QPainterPath& path )
{
	m_rect = path.controlPointRect();

	QGraphicsPathItem::setPath(path);
}


// Bounding rectangle accessor.
const QRectF& qpwgraph_item::itemRect (void) const
{
	return m_rect;
}


// end of qpwgraph_item.cpp
