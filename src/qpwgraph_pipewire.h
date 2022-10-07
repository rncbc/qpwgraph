// qpwgraph_pipewire.h
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

#ifndef __qpwgraph_pipewire_h
#define __qpwgraph_pipewire_h

#include "config.h"
#include "qpwgraph_sect.h"

#include <QHash>

#include <QMutex>


//----------------------------------------------------------------------------
// qpwgraph_pipewire -- PipeWire graph driver

class qpwgraph_pipewire : public qpwgraph_sect
{
	Q_OBJECT

public:

	// Constructor.
	qpwgraph_pipewire(qpwgraph_canvas *canvas);

	// Destructor.
	~qpwgraph_pipewire();

	// Client methods.
	bool open();
	void close();

	// Callback notifiers.
	void changedNotify();

	// PipeWire port (dis)connection.
	void connectPorts(qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect);

	// PipeWire graph updaters.
	void updateItems();
	void clearItems();

	// Special port-type colors defaults (virtual).
	void resetPortTypeColors();

	// PipeWire node type inquirer.
	static bool isNodeType(uint node_type);
	// PipeWire node type.
	static uint nodeType();

	// PipeWire port type(s) inquirer.
	static bool isPortType(uint port_type);
	// PipeWire port types.
	static uint audioPortType();
	static uint midiPortType();
	static uint videoPortType();
	static uint otherPortType();

	// Node/port renaming method (virtual override).
	void renameItem(qpwgraph_item *item, const QString& name);

	// PipeWire client data struct access.
	//
	struct Data;
	struct Object;
	struct Node;
	struct Port;
	struct Link;

	Data *data() const;

	// Object methods...
	Object *findObject(uint id) const;
	void addObject(uint id, Object *object);
	void removeObject(uint id);
	void clearObjects();

	// Node methods....
	Node *findNode(uint node_id) const;
	Node *createNode(
		uint node_id,
		const QString& node_name,
		qpwgraph_item::Mode node_mode,
		uint node_types);
	void destroyNode(Node *node);

	// Port methods....
	Port *findPort(uint port_id) const;
	Port *createPort(
		uint port_id,
		uint node_id,
		const QString& port_name,
		qpwgraph_item::Mode port_mode,
		uint port_type,
		uint port_flags);
	void destroyPort(Port *port);

	// Link methods....
	Link *findLink(uint link_id) const;
	Link *createLink(uint link_id, uint port1_id, uint port2_id);
	void destroyLink(Link *link);

signals:

	void changed();

protected slots:

	void reset();

protected:

	// PipeWire node:port finder and creator if not existing.
	bool findNodePort(
		uint32_t node_id, uint32_t port_id, qpwgraph_item::Mode port_mode,
		qpwgraph_node **node, qpwgraph_port **port, bool add_new);

private:

	// PipeWire client impl.
	Data *m_data;

	// PipeWire object database.
	QHash<uint, Object *> m_objectids;
	QList<Object *> m_objects;

	// Callback sanity mutex.
	static QMutex g_mutex;
};


#endif	// __qpwgraph_pipewire_h

// end of qpwgraph_pipewire.h
