// qpwgraph_command.h
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

#ifndef __qpwgraph_command_h
#define __qpwgraph_command_h

#include <QUndoCommand>

#include "qpwgraph_node.h"


// Forward decls.
class qpwgraph_canvas;


//----------------------------------------------------------------------------
// qpwgraph_command -- Generic graph command pattern

class qpwgraph_command : public QUndoCommand
{
public:

	// Constructor.
	qpwgraph_command(qpwgraph_canvas *canvas, QUndoCommand *parent = nullptr);

	// Accessors.
	qpwgraph_canvas *canvas() const
		{ return m_canvas; }

	// Command methods.
	void undo();
	void redo();

protected:

	// Command executive method.
	virtual bool execute(bool is_undo = false) = 0;

private:

	// Command arguments.
	qpwgraph_canvas *m_canvas;
};


//----------------------------------------------------------------------------
// qpwgraph_connect command -- Connect graph command

class qpwgraph_connect_command : public qpwgraph_command
{
public:

	// Constructor.
	qpwgraph_connect_command(qpwgraph_canvas *canvas,
		qpwgraph_port *port1, qpwgraph_port *port2,
		bool is_connect, qpwgraph_command *parent = nullptr);

protected:

	// Command item address
	struct Addr
	{
		// Constructors.
		Addr(qpwgraph_port *port)
		{
			qpwgraph_node *node = port->portNode();
			node_id   = node->nodeId();
			node_type = node->nodeType();
			port_id   = port->portId();
			port_type = port->portType();
		}
		// Copy constructor.
		Addr(const Addr& addr)
		{
			node_id   = addr.node_id;
			node_type = addr.node_type;
			port_id   = addr.port_id;
			port_type = addr.port_type;
		}
		// Member fields.
		uint node_id;
		uint node_type;
		uint port_id;
		uint port_type;
	};

	// Command item descriptor
	struct Item
	{
		// Constructor.
		Item(qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect)
			: addr1(port1), addr2(port2), m_connect(is_connect) {}
		// Copy constructor.
		Item(const Item& item) : addr1(item.addr1), addr2(item.addr2),
			m_connect(item.is_connect()) {}

		// Accessors.
		bool is_connect() const
			{ return m_connect; }

		// Public member fields.
		Addr addr1;
		Addr addr2;

	private:

		// Private member fields.
		bool m_connect;
	};

	// Command executive method.
	bool execute(bool is_undo);

private:

	// Command arguments.
	Item m_item;
};


//----------------------------------------------------------------------------
// qpwgraph_move_command -- Move (node) graph command

class qpwgraph_move_command : public qpwgraph_command
{
public:

	// Constructor.
	qpwgraph_move_command(qpwgraph_canvas *canvas,
		const QList<qpwgraph_node *>& nodes,
		const QPointF& pos1, const QPointF& pos2,
		qpwgraph_command *parent = nullptr);

	// Destructor.
	~qpwgraph_move_command();

	// Add/replace (an already moved) node position for undo/redo...
	void addItem(qpwgraph_node *node, const QPointF& pos1, const QPointF& pos2);

protected:

	// Command item descriptor
	struct Item
	{
		uint node_id;
		qpwgraph_item::Mode node_mode;
		uint node_type;
		QPointF node_pos1;
		QPointF node_pos2;
	};

	// Command executive method.
	bool execute(bool is_undo);

private:

	// Command arguments.
	QHash<qpwgraph_node *, Item *> m_items;

	int m_nexec;
};


//----------------------------------------------------------------------------
// qpwgraph_rename_command -- Rename (item) graph command

class qpwgraph_rename_command : public qpwgraph_command
{
public:

	// Constructor.
	qpwgraph_rename_command(qpwgraph_canvas *canvas,
		qpwgraph_item *item, const QString& name,
		qpwgraph_command *parent = nullptr);

protected:

	// Command item descriptor
	struct Item
	{
		int  item_type;
		uint node_id;
		qpwgraph_item::Mode node_mode;
		uint node_type;
		uint port_id;
		qpwgraph_item::Mode port_mode;
		uint port_type;
	};

	// Command executive method.
	bool execute(bool is_undo);

private:

	// Command arguments.
	Item    m_item;
	QString m_name;
};


#endif	// __qpwgraph_command_h

// end of qpwgraph_command.h
