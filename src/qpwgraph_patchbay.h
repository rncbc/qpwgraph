// qpraph1_patchbay.h
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

#ifndef __qpwgraph_patchbay_h
#define __qpwgraph_patchbay_h

#include <QString>
#include <QList>
#include <QHash>


// Forward decls.
class qpwgraph_canvas;
class qpwgraph_connect;
class qpwgraph_port;


//----------------------------------------------------------------------------
// qpwgraph_patchbay -- Persistant connections patchbay decl.

class qpwgraph_patchbay
{
public:

	// Constructor.
	qpwgraph_patchbay(qpwgraph_canvas *canvas) : m_canvas(canvas),
		m_activated(false), m_exclusive(false), m_dirty(0) {}

	// Destructor.
	~qpwgraph_patchbay() { clear(); }

	// Mode/properties accessors.
	void setActivated(bool activated)
		{ m_activated = activated; }
	bool isActivated() const
		{ return m_activated; }

	void setExclusive(bool exclusive)
		{ m_exclusive = exclusive; }
	bool isExclusive() const
		{ return m_exclusive; }

	// Clear all patchbay rules and cache.
	void clear();

	// Snapshot of all current graph connections...
	void snap();

	// Patchbay rules file I/O methods.
	bool load(const QString& filename);
	bool save(const QString& filename);

	// Execute and apply rules to graph.
	bool scan();

	// Update rules on demand.
	bool connectPorts(qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect);
	bool connect(qpwgraph_connect *connect, bool is_connect);

	// Patchbay rule item.
	struct Item
	{
		Item(uint nt, uint pt,
			const QString& n1, const QString& p1,
			const QString& n2, const QString& p2)
			: node_type(nt), port_type(pt), node1(n1), port1(p1), node2(n2), port2(p2) {}

		Item(const Item& item) : node_type(item.node_type), port_type(item.port_type),
			node1(item.node1), port1(item.port1), node2(item.node2), port2(item.port2) {}

		bool operator== (const Item& item) const
		{
			return node_type == item.node_type
				&& port_type == item.port_type
				&& node1 == item.node1
				&& port1 == item.port1
				&& node2 == item.node2
				&& port2 == item.port2;
		}

		uint node_type;
		uint port_type;
		QString node1;
		QString port1;
		QString node2;
		QString port2;
	};

	typedef QHash<Item, Item *> Items;

	// Find a connection rule.
	Item *findConnectPorts(qpwgraph_port *port1, qpwgraph_port *port2) const;
	Item *findConnect(qpwgraph_connect *connect) const;

	// Dirty status flag.
	bool isDirty() const
		{ return (m_dirty > 0); }

protected:

	// Node and port type to text helpers.
	static uint nodeTypeFromText(const QString& text);
	static const char *textFromNodeType(uint node_type);

	static uint portTypeFromText(const QString& text);
	static const char *textFromPortType(uint port_type);

private:

	// Instance variables.
	qpwgraph_canvas *m_canvas;

	bool m_activated;
	bool m_exclusive;

	Items m_items;

	int m_dirty;
};


inline uint qHash ( const qpwgraph_patchbay::Item& item )
{
	return qHash(item.node_type)
		 ^ qHash(item.port_type)
		 ^ qHash(item.node1)
		 ^ qHash(item.port1)
		 ^ qHash(item.node2)
		 ^ qHash(item.port2);
}


#endif	// __qpwgraph_patchbay_h

// end of qpwgraph_patchbay.h

