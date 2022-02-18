// qpwgraph_patchbay.cpp
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


// Clear all patchbay rules and cache.
void qpwgraph_patchbay::clear (void)
{
	QHash<Item, Item *>::ConstIterator iter = m_items.constBegin();
	const QHash<Item, Item *>::ConstIterator& iter_end = m_items.constEnd();
	for ( ; iter != iter_end; ++iter)
		delete iter.value();

	m_items.clear();
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
					if (node_type > 0 && port_type > 0
						&& !node1.isEmpty() && !port1.isEmpty()
						&& !node2.isEmpty() && !port2.isEmpty()) {
						Item *item = new Item(
							node_type, port_type,
							node1, port1, node2, port2);
						m_items.insert(*item, item);
					}
				}
			}
		}
	}

	return true;
}


bool qpwgraph_patchbay::save ( const QString& filename ) const
{
	if (m_canvas == nullptr)
		return false;

	QGraphicsScene *scene = m_canvas->scene();
	if (scene == nullptr)
		return false;

	QFileInfo fi(filename);
	const char *root_name = "patchbay";
	QDomDocument doc(root_name);
	QDomElement eroot = doc.createElement(root_name);
	eroot.setAttribute("name", fi.baseName());
	eroot.setAttribute("version", PROJECT_VERSION);
	doc.appendChild(eroot);

	QDomElement eitems = doc.createElement("items");
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
						QDomElement eitem = doc.createElement("item");
						eitem.setAttribute("node-type", textFromNodeType(node1->nodeType()));
						eitem.setAttribute("port-type", textFromPortType(port1->portType()));
						QDomElement eitem1 = doc.createElement("output");
						eitem1.setAttribute("node", node1->nodeName());
						eitem1.setAttribute("port", port1->portName());
						eitem.appendChild(eitem1);
						QDomElement eitem2 = doc.createElement("input");
						eitem2.setAttribute("node", node2->nodeName());
						eitem2.setAttribute("port", port2->portName());
						eitem.appendChild(eitem2);
						eitems.appendChild(eitem);
					}
				}
			}
		}
	}
	eroot.appendChild(eitems);

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;

	QTextStream ts(&file);
	ts << doc.toString() << endl;
	file.close();

	return true;
}


// Execute and apply rules to graph.
bool qpwgraph_patchbay::scan (void)
{
	if (!m_activated)
		return false;

	if (m_canvas == nullptr)
		return false;

	QGraphicsScene *scene = m_canvas->scene();
	if (scene == nullptr)
		return false;

	QHash<Item, qpwgraph_connect *> connects;

	QHash<Item, Item *>::ConstIterator iter = m_items.constBegin();
	const QHash<Item, Item *>::ConstIterator& iter_end = m_items.constEnd();
	for ( ; iter != iter_end; ++iter) {
		Item *item = iter.value();
		qpwgraph_node *node1
			= m_canvas->findNode(
				item->node1,
				qpwgraph_item::Output,
				item->node_type);
		if (node1 == nullptr)
			node1 = m_canvas->findNode(
				item->node1,
				qpwgraph_item::Duplex,
				item->node_type);
		if (node1 == nullptr)
			continue;
		qpwgraph_port *port1
			= node1->findPort(
				item->port1,
				qpwgraph_item::Output,
				item->port_type);
		if (port1 == nullptr)
			continue;
		qpwgraph_node *node2
			= m_canvas->findNode(
				item->node2,
				qpwgraph_item::Input,
				item->node_type);
		if (node2 == nullptr)
			node2 = m_canvas->findNode(
				item->node2,
				qpwgraph_item::Duplex,
				item->node_type);
		if (node2 == nullptr)
			continue;
		qpwgraph_port *port2
			= node2->findPort(
				item->port2,
				qpwgraph_item::Input,
				item->port_type);
		if (port2 == nullptr)
			continue;
		if (!port1->findConnect(port2)) {
			if (m_exclusive) {
				foreach (qpwgraph_connect *connect, port1->connects()) {
					if (connect->port2() != port2) {
						port2 = connect->port2();
						if (port2 == nullptr)
							continue;
						node2 = port2->portNode();
						if (node2 == nullptr)
							continue;
						const Item item2(
							node1->nodeType(),
							port1->portType(),
							node1->nodeName(),
							port1->portName(),
							node2->nodeName(),
							port2->portName());
						connects.insert(item2, connect);
					}
				}
			}
			m_canvas->emitConnected(port1, port2);
		}
	}

	QHash<Item, qpwgraph_connect *>::Iterator iter3 = connects.begin();
	const QHash<Item, qpwgraph_connect *>::Iterator& iter3_end = connects.end();
	for (; iter3 != iter3_end; ++iter3) {
		qpwgraph_connect *connect = iter3.value();
		if (connect)
			m_canvas->emitDisconnected(connect->port1(), connect->port2());
	}

	return true;
}


// Update rules on demand.
void qpwgraph_patchbay::connectPorts ( qpwgraph_port *port1, qpwgraph_port *port2, bool connect )
{
	if (port1 == nullptr || port2 == nullptr)
		return;

	qpwgraph_node *node1 = port1->portNode();
	qpwgraph_node *node2 = port2->portNode();
	if (node1 && node2) {
		const Item item(
			node1->nodeType(),
			port1->portType(),
			node1->nodeName(),
			port1->portName(),
			node2->nodeName(),
			port2->portName());
		QHash<Item, Item *>::ConstIterator iter = m_items.constFind(item);
		const QHash<Item, Item *>::ConstIterator& iter_end = m_items.constEnd();
		if (iter == iter_end && connect) {
			m_items.insert(item, new Item(item));
		}
		else
		if (iter != iter_end && !connect) {
			delete iter.value();
			m_items.erase(iter);
		}
	}
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
