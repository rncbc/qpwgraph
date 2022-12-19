// qpwgraph_connect.h
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

#ifndef __qpwgraph_connect_h
#define __qpwgraph_connect_h

#include "qpwgraph_item.h"


// Forward decls.
class qpwgraph_port;


//----------------------------------------------------------------------------
// qpwgraph_connect -- Connection-line graphics item.

class qpwgraph_connect : public qpwgraph_item
{
public:

	// Constructor.
	qpwgraph_connect();

	// Destructor..
	~qpwgraph_connect();

	// Graphics item type.
	enum { Type = QGraphicsItem::UserType + 3 };

	int type() const { return Type; }

	// Accessors.
	void setPort1(qpwgraph_port *port);
	qpwgraph_port *port1() const;

	void setPort2(qpwgraph_port *port);
	qpwgraph_port *port2() const;

	// Active disconnection.
	void disconnect();

	// Path/shaper updaters.
	void updatePathTo(const QPointF& pos);
	void updatePath();

	// Selection propagation method...
	void setSelectedEx(qpwgraph_port *port, bool is_selected);

	// Highlighting propagation method...
	void setHighlightEx(qpwgraph_port *port, bool is_highlight);

	// Special port-type color business.
	void updatePortTypeColors();

	// Dim/transparency option.
	void setDimmed(bool dimmed);
	int isDimmed() const;

	// Connector curve draw style (through vs. around nodes)
	static void setConnectThroughNodes(bool on);
	static bool isConnectThroughNodes();

protected:

	void paint(QPainter *painter,
		const QStyleOptionGraphicsItem *option, QWidget *widget);

	QVariant itemChange(GraphicsItemChange change, const QVariant& value);

	QPainterPath shape() const;

private:

	// Instance variables.
	qpwgraph_port *m_port1;
	qpwgraph_port *m_port2;

	bool m_dimmed;

	// Connector curve draw style (through vs. around nodes)
	static bool g_connect_through_nodes;
};


#endif	// __qpwgraph_connect_h

// end of qpwgraph_connect.h
