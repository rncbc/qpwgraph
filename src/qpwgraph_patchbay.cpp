// qpwgraph_patchbay.cpp
//
/****************************************************************************
   Copyright (C) 2021-2025, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "config.h"
#include "qpwgraph_patchbay.h"

#include "qpwgraph_canvas.h"
#include "qpwgraph_connect.h"

#include "qpwgraph_port.h"
#include "qpwgraph_node.h"

#include "qpwgraph_pipewire.h"
#include "qpwgraph_alsamidi.h"

#include <QDomDocument>
#include <QTextStream>
#include <QFileInfo>


// Deprecated QTextStreamFunctions/Qt namespaces workaround.
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#define endl	Qt::endl
#endif


//----------------------------------------------------------------------------
// qpwgraph_patchbay -- Persistant connections patchbay impl.


// Manage connection rules.
bool qpwgraph_patchbay::Items::addItem ( const Item& item )
{
	bool ret = false;

	ConstIterator iter = constFind(item);
	if (iter == constEnd()) {
		insert(item, new Item(item));
		ret = true;
	}

	return ret;
}


bool qpwgraph_patchbay::Items::removeItem ( const Item& item )
{
	bool ret = false;

	ConstIterator iter = constFind(item);
	if (iter != constEnd()) {
		delete iter.value();
		erase(iter);
		ret = true;
	}

	return ret;
}


// Copy all patchbay rules and cache.
void qpwgraph_patchbay::Items::copyItems ( const Items& items )
{
	clearItems();

	Items::ConstIterator iter = items.constBegin();
	const Items::ConstIterator& iter_end = items.constEnd();
	for ( ; iter != iter_end; ++iter)
		addItem(iter.key());
}


// Clear all patchbay rules and cache.
void qpwgraph_patchbay::Items::clearItems (void)
{
	ConstIterator iter = constBegin();
	const ConstIterator& iter_end = constEnd();
	for ( ; iter != iter_end; ++iter)
		delete iter.value();

	clear();
}


void qpwgraph_patchbay::clear (void)
{
	m_items.clearItems();

	m_dirty = 0;
}


// Snapshot of all current graph connections...
void qpwgraph_patchbay::snap (void)
{
//	clear();

	if (m_canvas == nullptr)
		return;

	QGraphicsScene *scene = m_canvas->scene();
	if (scene == nullptr)
		return;

	foreach (QGraphicsItem *item, scene->items()) {
		if (item->type() == qpwgraph_connect::Type) {
			qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
			if (connect) {
				qpwgraph_port *port1 = connect->port1();
				qpwgraph_port *port2 = connect->port2();
				if (port1 && port2) {
					qpwgraph_node *node1 = port1->portNode();
					qpwgraph_node *node2 = port2->portNode();
					if (node1 && node2) {
						m_items.addItem(Item(
							node1->nodeType(),
							port1->portType(),
							node1->nodeNameEx(),
							port1->portName(),
							node2->nodeNameEx(),
							port2->portName()));
					}
				}
			}
		}
	}
}


// Patchbay rules file I/O methods.
bool qpwgraph_patchbay::load ( const QString& filename )
{
	// Open file...
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	QDomDocument doc("patchbay");
	if (!doc.setContent(&file)) {
		file.close();
		return false;
	}
	file.close();

//	clear();

	QDomElement edoc = doc.documentElement();
	for (QDomNode nroot = edoc.firstChild();
		 !nroot.isNull();
		 	nroot = nroot.nextSibling()) {
		QDomElement eroot = nroot.toElement();
		if (eroot.isNull())
			continue;
	#ifdef CONFIG_CLEANUP_NODE_NAMES
		const bool cleanup
			= (eroot.attribute("version") < "0.5.0");
	#endif
		if (eroot.tagName() == "items") {
			for (QDomNode nitem = eroot.firstChild();
				!nitem.isNull();
					 nitem = nitem.nextSibling()) {
				QDomElement eitem = nitem.toElement();
				if (eitem.isNull())
					continue;
				if (eitem.tagName() == "item") {
					const uint node_type = nodeTypeFromText(eitem.attribute("node-type"));
					const uint port_type = portTypeFromText(eitem.attribute("port-type"));
					QString node1, port1, node2, port2;
					for (QDomNode nitem2 = eitem.firstChild();
						!nitem2.isNull();
					 		nitem2 = nitem2.nextSibling()) {
						QDomElement eitem2 = nitem2.toElement();
						if (eitem2.isNull())
							continue;
						if (eitem2.tagName() == "output") {
							node1 = eitem2.attribute("node");
							port1 = eitem2.attribute("port");
						}
						else
						if (eitem2.tagName() == "input") {
							node2 = eitem2.attribute("node");
							port2 = eitem2.attribute("port");
						}
					}
				#ifdef CONFIG_CLEANUP_NODE_NAMES
					if (cleanup) { // FIXME: Cleanup legacy node names...
						if (qpwgraph_canvas::cleanupNodeName(node1))
							++m_dirty;
						if (qpwgraph_canvas::cleanupNodeName(node2))
							++m_dirty;
					}
				#endif
					if (node_type > 0 && port_type > 0
						&& !node1.isEmpty() && !port1.isEmpty()
						&& !node2.isEmpty() && !port2.isEmpty()) {
						m_items.addItem(Item(
							node_type, port_type,
							node1, port1, node2, port2));
					}
				}
			}
		}
	}

	return true;
}


bool qpwgraph_patchbay::save ( const QString& filename )
{
	if (m_canvas == nullptr)
		return false;

	QFileInfo fi(filename);
	const char *root_name = "patchbay";
	QDomDocument doc(root_name);
	QDomElement eroot = doc.createElement(root_name);
	eroot.setAttribute("name", fi.baseName());
	eroot.setAttribute("version", PROJECT_VERSION);
	doc.appendChild(eroot);

	QDomElement eitems = doc.createElement("items");
	Items::ConstIterator iter = m_items.constBegin();
	const Items::ConstIterator& iter_end = m_items.constEnd();
	for ( ; iter != iter_end; ++iter) {
		Item *item = iter.value();
		QDomElement eitem = doc.createElement("item");
		eitem.setAttribute("node-type", textFromNodeType(item->node_type));
		eitem.setAttribute("port-type", textFromPortType(item->port_type));
		QDomElement eitem1 = doc.createElement("output");
		eitem1.setAttribute("node", item->node1);
		eitem1.setAttribute("port", item->port1);
		eitem.appendChild(eitem1);
		QDomElement eitem2 = doc.createElement("input");
		eitem2.setAttribute("node", item->node2);
		eitem2.setAttribute("port", item->port2);
		eitem.appendChild(eitem2);
		eitems.appendChild(eitem);
	}
	eroot.appendChild(eitems);

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;

	QTextStream ts(&file);
	ts << doc.toString() << endl;
	file.close();

	m_dirty = 0;

	return true;
}


// Execute and apply rules to graph.
bool qpwgraph_patchbay::scan (void)
{
	if (m_canvas == nullptr)
		return false;

	QGraphicsScene *scene = m_canvas->scene();
	if (scene == nullptr)
		return false;

	QHash<Item, qpwgraph_connect *> disconnects;

	Items::ConstIterator iter = m_items.constBegin();
	const Items::ConstIterator& iter_end = m_items.constEnd();
	for ( ; iter != iter_end; ++iter) {
		Item *item = iter.value();
		QList<qpwgraph_node *> nodes1
			= m_canvas->findNodes(
				item->node1,
				qpwgraph_item::Output,
				item->node_type);
		if (nodes1.isEmpty())
			nodes1 = m_canvas->findNodes(
				item->node1,
				qpwgraph_item::Duplex,
				item->node_type);
		if (nodes1.isEmpty())
			continue;
		foreach (qpwgraph_node *node1, nodes1) {
			qpwgraph_port *port1
				= node1->findPort(
					item->port1,
					qpwgraph_item::Output,
					item->port_type);
			if (port1 == nullptr)
				continue;
			QList<qpwgraph_node *> nodes2
				= m_canvas->findNodes(
					item->node2,
					qpwgraph_item::Input,
					item->node_type);
			if (nodes2.isEmpty())
				nodes2 = m_canvas->findNodes(
					item->node2,
					qpwgraph_item::Duplex,
					item->node_type);
			if (nodes2.isEmpty())
				continue;
			const bool node1_exclusive
				= m_canvas->isMergerNodes(node1->nodeName());
			foreach (qpwgraph_node *node2, nodes2) {
				qpwgraph_port *port2
					= node2->findPort(
						item->port2,
						qpwgraph_item::Input,
						item->port_type);
				if (port2 == nullptr)
					continue;
				if (m_activated && (m_exclusive || node1_exclusive)) {
					foreach (qpwgraph_connect *connect12, port1->connects()) {
						qpwgraph_port *port12 = connect12->port2();
						if (port12 == nullptr)
							continue;
						if (port12 != port2) {
							qpwgraph_node *node12 = port12->portNode();
							if (node12 == nullptr)
								continue;
							const Item item12(
								node1->nodeType(),
								port1->portType(),
								node1->nodeNameEx(),
								port1->portName(),
								node12->nodeNameEx(),
								port12->portName());
							if (m_items.constFind(item12) == iter_end)
								disconnects.insert(item12, connect12);
						}
					}
					foreach (qpwgraph_connect *connect21, port2->connects()) {
						qpwgraph_port *port21 = connect21->port1();
						if (port21 == nullptr)
							continue;
						if (port21 != port1) {
							qpwgraph_node *node21 = port21->portNode();
							if (node21 == nullptr)
								continue;
							const Item item21(
								node21->nodeType(),
								port21->portType(),
								node21->nodeNameEx(),
								port21->portName(),
								node2->nodeNameEx(),
								port2->portName());
							if (m_items.constFind(item21) == iter_end) {
								disconnects.insert(item21, connect21);
							}
						}
					}
				}
				qpwgraph_connect *connect12 = port1->findConnect(port2);
				if (connect12 == nullptr && m_activated)
					m_canvas->emitConnected(port1, port2);
				else
				if (!m_activated && m_canvas->isPatchbayAutoDisconnect()) {
					const Item item12(
						node1->nodeType(),
						port1->portType(),
						node1->nodeNameEx(),
						port1->portName(),
						node2->nodeNameEx(),
						port2->portName());
					disconnects.insert(item12, connect12);
				}
			}
		}
	}

	QHash<Item, qpwgraph_connect *>::ConstIterator iter2
		= disconnects.constBegin();
	const QHash<Item, qpwgraph_connect *>::ConstIterator& iter2_end
		= disconnects.constEnd();
	for (; iter2 != iter2_end; ++iter2) {
		qpwgraph_connect *connect = iter2.value();
		if (connect)
			m_canvas->emitDisconnected(connect->port1(), connect->port2());
	}

	return true;
}


// Update rules on demand.
bool qpwgraph_patchbay::connectPorts (
	qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect )
{
	if (port1 == nullptr || port2 == nullptr)
		return false;

	bool ret = false;

	qpwgraph_node *node1 = port1->portNode();
	qpwgraph_node *node2 = port2->portNode();
	if (node1 && node2) {
		const Item item(
			node1->nodeType(),
			port1->portType(),
			node1->nodeNameEx(),
			port1->portName(),
			node2->nodeNameEx(),
			port2->portName());
		if (is_connect)
			ret = m_items.addItem(item);
		else
			ret = m_items.removeItem(item);
	}

	if (ret) ++m_dirty;

	return ret;
}


bool qpwgraph_patchbay::connect ( qpwgraph_connect *connect, bool is_connect )
{
	return connectPorts(connect->port1(), connect->port2(), is_connect);
}


// Find a connection rule.
qpwgraph_patchbay::Item *qpwgraph_patchbay::findConnectPorts (
	qpwgraph_port *port1, qpwgraph_port *port2 ) const
{
	Item *ret = nullptr;

	if (port1 && port2) {
		qpwgraph_node *node1 = port1->portNode();
		qpwgraph_node *node2 = port2->portNode();
		if (node1 && node2) {
			const Item item(
				node1->nodeType(),
				port1->portType(),
				node1->nodeNameEx(),
				port1->portName(),
				node2->nodeNameEx(),
				port2->portName());
			ret = m_items.value(item, nullptr);
		}
	}

	return ret;
}

qpwgraph_patchbay::Item *qpwgraph_patchbay::findConnect (
	qpwgraph_connect *connect ) const
{
	return findConnectPorts(connect->port1(), connect->port2());
}


// Dirty status flag.
void qpwgraph_patchbay::setItems ( const Items& items )
{
	m_items.copyItems(items);

	++m_dirty;

	m_canvas->patchbayEdit();
	emit m_canvas->changed();
}


// Node and port type to text helpers.
uint qpwgraph_patchbay::nodeTypeFromText ( const QString& text )
{
	if (text == "pipewire")
		return qpwgraph_pipewire::nodeType();
	else
#ifdef CONFIG_ALSA_MIDI
	if (text == "alsa")
		return qpwgraph_alsamidi::nodeType();
	else
#endif
	return 0;
}


const char *qpwgraph_patchbay::textFromNodeType ( uint node_type )
{
	if (node_type == qpwgraph_pipewire::nodeType())
		return "pipewire";
	else
#ifdef CONFIG_ALSA_MIDI
	if (node_type == qpwgraph_alsamidi::nodeType())
		return "alsa";
	else
#endif
	return nullptr;
}


uint qpwgraph_patchbay::portTypeFromText ( const QString& text )
{
	if (text == "pipewire-audio")
		return qpwgraph_pipewire::audioPortType();
	else
	if (text == "pipewire-midi")
		return qpwgraph_pipewire::midiPortType();
	else
	if (text == "pipewire-video")
		return qpwgraph_pipewire::videoPortType();
	else
	if (text == "pipewire-other")
		return qpwgraph_pipewire::otherPortType();
	else
#ifdef CONFIG_ALSA_MIDI
	if (text == "alsa-midi")
		return qpwgraph_alsamidi::midiPortType();
	else
#endif
	return 0;
}


const char *qpwgraph_patchbay::textFromPortType ( uint port_type )
{
	if (port_type == qpwgraph_pipewire::audioPortType())
		return "pipewire-audio";
	else
	if (port_type == qpwgraph_pipewire::midiPortType())
		return "pipewire-midi";
	else
	if (port_type == qpwgraph_pipewire::videoPortType())
		return "pipewire-video";
	else
	if (port_type == qpwgraph_pipewire::otherPortType())
		return "pipewire-other";
	else
#ifdef CONFIG_ALSA_MIDI
	if (port_type == qpwgraph_alsamidi::midiPortType())
		return "alsa-midi";
	else
#endif
	return nullptr;
}


// end of qpwgraph_patchbay.cpp
