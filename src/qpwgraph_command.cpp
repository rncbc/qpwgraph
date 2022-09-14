// qpwgraph_command.cpp
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

#include "qpwgraph_command.h"

#include "qpwgraph_canvas.h"


//----------------------------------------------------------------------------
// qpwgraph_command -- Generic graph command pattern

// Constructor.
qpwgraph_command::qpwgraph_command ( qpwgraph_canvas *canvas,
	QUndoCommand *parent ) : QUndoCommand(parent),
		m_canvas(canvas)
{
}


// Command methods.
void qpwgraph_command::undo (void)
{
	execute(true);
}


void qpwgraph_command::redo (void)
{
	execute(false);
}


//----------------------------------------------------------------------------
// qpwgraph_connect_command -- Connect graph command pattern

// Constructor.
qpwgraph_connect_command::qpwgraph_connect_command ( qpwgraph_canvas *canvas,
	qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect,
	qpwgraph_command *parent ) : qpwgraph_command(canvas, parent),
		m_item(port1, port2, is_connect)
{
}


// Command executive
bool qpwgraph_connect_command::execute ( bool is_undo )
{
	qpwgraph_canvas *canvas = qpwgraph_command::canvas();
	if (canvas == nullptr)
		return false;

	qpwgraph_node *node1
		= canvas->findNode(
			m_item.addr1.node_id,
			qpwgraph_item::Output,
			m_item.addr1.node_type);
	if (node1 == nullptr)
		node1 = canvas->findNode(
			m_item.addr1.node_id,
			qpwgraph_item::Duplex,
			m_item.addr1.node_type);
	if (node1 == nullptr)
		return false;

	qpwgraph_port *port1
		= node1->findPort(
			m_item.addr1.port_id,
			qpwgraph_item::Output,
			m_item.addr1.port_type);
	if (port1 == nullptr)
		return false;

	qpwgraph_node *node2
		= canvas->findNode(
			m_item.addr2.node_id,
			qpwgraph_item::Input,
			m_item.addr2.node_type);
	if (node2 == nullptr)
		node2 = canvas->findNode(
			m_item.addr2.node_id,
			qpwgraph_item::Duplex,
			m_item.addr2.node_type);
	if (node2 == nullptr)
		return false;

	qpwgraph_port *port2
		= node2->findPort(
			m_item.addr2.port_id,
			qpwgraph_item::Input,
			m_item.addr2.port_type);
	if (port2 == nullptr)
		return false;

	const bool is_connect
		= (m_item.is_connect() && !is_undo) || (!m_item.is_connect() && is_undo);
	canvas->emitConnectPorts(port1, port2, is_connect);

	return true;
}


//----------------------------------------------------------------------------
// qpwgraph_move_command -- Move (node) graph command

// Constructor.
qpwgraph_move_command::qpwgraph_move_command ( qpwgraph_canvas *canvas,
	const QList<qpwgraph_node *>& nodes, const QPointF& pos1, const QPointF& pos2,
	qpwgraph_command *parent ) : qpwgraph_command(canvas, parent), m_nexec(0)
{
	qpwgraph_command::setText(QObject::tr("Move"));

	const QPointF delta = (pos1 - pos2);

	foreach (qpwgraph_node *node, nodes) {
		Item *item = new Item;
		item->node_id   = node->nodeId();
		item->node_mode = node->nodeMode();
		item->node_type = node->nodeType();
		const QPointF& pos = node->pos();
		item->node_pos1 = pos + delta;
		item->node_pos2 = pos;
		m_items.insert(node, item);
	}

	if (canvas && canvas->isRepelOverlappingNodes()) {
		foreach (qpwgraph_node *node, nodes)
			canvas->repelOverlappingNodes(node, this);
	}
}


// Destructor.
qpwgraph_move_command::~qpwgraph_move_command (void)
{
	qDeleteAll(m_items);
	m_items.clear();
}


// Add/replace (an already moved) node position for undo/redo...
void qpwgraph_move_command::addItem (
	qpwgraph_node *node, const QPointF& pos1, const QPointF& pos2 )
{
	Item *item = m_items.value(node, nullptr);
	if (item) {
	//	item->node_pos1 = pos1;
		item->node_pos2 = pos2;//node->pos();
	} else {
		item = new Item;
		item->node_id   = node->nodeId();
		item->node_mode = node->nodeMode();
		item->node_type = node->nodeType();
		item->node_pos1 = pos1;
		item->node_pos2 = pos2;//node->pos();
		m_items.insert(node, item);
	}
}



// Command executive method.
bool qpwgraph_move_command::execute ( bool /* is_undo */ )
{
	qpwgraph_canvas *canvas = qpwgraph_command::canvas();
	if (canvas == nullptr)
		return false;

	if (++m_nexec > 1) {
		foreach (qpwgraph_node *key, m_items.keys()) {
			Item *item = m_items.value(key, nullptr);
			if (item) {
				qpwgraph_node *node = canvas->findNode(
					item->node_id, item->node_mode, item->node_type);
				if (node) {
					const QPointF pos1 = item->node_pos1;
					node->setPos(pos1);
					item->node_pos1 = item->node_pos2;
					item->node_pos2 = pos1;
				}
			}
		}
	}

	return true;
}


//----------------------------------------------------------------------------
// qpwgraph_rename_command -- Rename (item) graph command

// Constructor.
qpwgraph_rename_command::qpwgraph_rename_command ( qpwgraph_canvas *canvas,
	qpwgraph_item *item, const QString& name, qpwgraph_command *parent )
	: qpwgraph_command(canvas, parent), m_name(name)
{
	qpwgraph_command::setText(QObject::tr("Rename"));

	m_item.item_type = item->type();

	qpwgraph_node *node = nullptr;
	qpwgraph_port *port = nullptr;

	if (m_item.item_type == qpwgraph_node::Type)
		node = static_cast<qpwgraph_node *> (item);
	else
	if (m_item.item_type == qpwgraph_port::Type)
		port = static_cast<qpwgraph_port *> (item);

	if (port)
		node = port->portNode();

	if (node) {
		m_item.node_id   = node->nodeId();
		m_item.node_mode = node->nodeMode();
		m_item.node_type = node->nodeType();
	}

	if (port) {
		m_item.port_id   = port->portId();
		m_item.port_mode = port->portMode();
		m_item.port_type = port->portType();
	}
}


// Command executive method.
bool qpwgraph_rename_command::execute ( bool /*is_undo*/ )
{
	qpwgraph_canvas *canvas = qpwgraph_command::canvas();
	if (canvas == nullptr)
		return false;

	QString name = m_name;
	qpwgraph_item *item = nullptr;

	qpwgraph_node *node = canvas->findNode(
		m_item.node_id, m_item.node_mode, m_item.node_type);

	if (m_item.item_type == qpwgraph_node::Type && node) {
		m_name = node->nodeTitle();
		item = node;
	}
	else
	if (m_item.item_type == qpwgraph_port::Type && node) {
		qpwgraph_port *port = node->findPort(
			m_item.port_id, m_item.port_mode, m_item.port_type);
		if (port) {
			m_name = port->portTitle();
			item = port;
		}
	}

	if (item == nullptr)
		return false;

	canvas->emitRenamed(item, name);
	return true;
}



// end of qpwgraph_command.cpp
