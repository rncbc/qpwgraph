// qpwgraph_port.h
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

#ifndef __qpwgraph_port_h
#define __qpwgraph_port_h

#include "qpwgraph_item.h"


// Forward decls.
class qpwgraph_canvas;
class qpwgraph_node;
class qpwgraph_connect;

class QStyleOptionGraphicsItem;


//----------------------------------------------------------------------------
// qpwgraph_port -- Port graphics item.

class qpwgraph_port : public qpwgraph_item
{
public:

	// Constructor.
	qpwgraph_port(qpwgraph_node *node,
		uint id, const QString& name, Mode mode, uint type = 0);

	// Destructor.
	~qpwgraph_port();

	// Graphics item type.
	enum { Type = QGraphicsItem::UserType + 2 };

	int type() const { return Type; }

	// Accessors.
	qpwgraph_node *portNode() const;

	uint32_t portId() const;

	void setPortName(const QString& name);
	const QString& portName() const;

	void setPortMode(Mode mode);
	Mode portMode() const;

	bool isInput() const;
	bool isOutput() const;

	void setPortType(uint type);
	uint portType() const;

	void setPortTitle(const QString& title);
	const QString& portTitle() const;

	void setPortIndex(int index);
	int portIndex() const;

	QPointF portPos() const;

	// Connection-list methods.
	void appendConnect(qpwgraph_connect *connect);
	void removeConnect(qpwgraph_connect *connect);
	void removeConnects();

	qpwgraph_connect *findConnect(qpwgraph_port *port) const;

	// Connect-list accessor.
	const QList<qpwgraph_connect *>& connects() const;

	// Selection propagation method...
	void setSelectedEx(bool is_selected);

	// Highlighting propagation method...
	void setHighlightEx(bool is_highlight);

	// Special port-type color business.
	void updatePortTypeColors(qpwgraph_canvas *canvas);

	// Port hash/map key (by id).
	class PortIdKey : public IdKey
	{
	public:
		// Constructor.
		PortIdKey(qpwgraph_port *port)
			: IdKey(port->portId(), port->portMode(), port->portType()) {}
	};

	// Port hash/map key (by name).
	class PortNameKey : public NameKey
	{
	public:
		// Constructors.
		PortNameKey (const QString& name, Mode mode, uint type = 0)
			: NameKey(name, mode, type) {}
		PortNameKey(qpwgraph_port *port)
			: NameKey(port->portName(), port->portMode(), port->portType()) {}
	};

	typedef QMultiHash<PortNameKey, qpwgraph_port *> PortKeys;

	// Port sorting type.
	enum SortType { PortName = 0, PortTitle, PortIndex };

	static void setSortType(SortType sort_type);
	static SortType sortType();

	// Port sorting order.
	enum SortOrder { Ascending = 0, Descending };

	static void setSortOrder(SortOrder sort_order);
	static SortOrder sortOrder();

	// Port sorting comparators.
	struct Compare {
		bool operator()(qpwgraph_port *port1, qpwgraph_port *port2) const
			{ return qpwgraph_port::lessThan(port1, port2); }
	};

	struct ComparePos {
		bool operator()(qpwgraph_port *port1, qpwgraph_port *port2) const
			{ return (port1->scenePos().y() < port2->scenePos().y()); }
	};

	// Rectangular editor extents.
	QRectF editorRect() const;

protected:

	void paint(QPainter *painter,
		const QStyleOptionGraphicsItem *option, QWidget *widget);

	QVariant itemChange(GraphicsItemChange change, const QVariant& value);

	// Natural decimal sorting comparators.
	static bool lessThan(qpwgraph_port *port1, qpwgraph_port *port2);
	static bool lessThan(const QString& s1, const QString& s2);

private:

	// instance variables.
	qpwgraph_node *m_node;

	uint    m_id;
	QString m_name;
	Mode    m_mode;
	uint    m_type;

	QString m_title;
	int     m_index;

	QGraphicsTextItem *m_text;

	QList<qpwgraph_connect *> m_connects;

	int m_selectx;
	int m_hilitex;

	static SortType  g_sort_type;
	static SortOrder g_sort_order;
};


#endif	// __qpwgraph_port_h

// end of qpwgraph_port.h
