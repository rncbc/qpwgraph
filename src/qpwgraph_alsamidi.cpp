// qpwgraph_alsamidi.cpp
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

#include "qpwgraph_alsamidi.h"


#ifdef CONFIG_ALSA_MIDI

#include "qpwgraph_canvas.h"
#include "qpwgraph_connect.h"

#include <QSocketNotifier>

#include <QMutexLocker>


//----------------------------------------------------------------------------
// qpwgraph_alsamidi -- ALSA graph driver

QMutex qpwgraph_alsamidi::g_mutex;


// Constructor.
qpwgraph_alsamidi::qpwgraph_alsamidi ( qpwgraph_canvas *canvas )
	: qpwgraph_sect(canvas), m_seq(nullptr), m_notifier(nullptr)
{
	resetPortTypeColors();

	open();
}


// Destructor.
qpwgraph_alsamidi::~qpwgraph_alsamidi (void)
{
	close();
}


// Client methods.
bool qpwgraph_alsamidi::open (void)
{
	QMutexLocker locker(&g_mutex);

	if (m_seq)
		return true;

	if (snd_seq_open(&m_seq, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
		m_seq = nullptr;
		return false;
	}

	const int port_id
		= snd_seq_create_simple_port(m_seq,
			"qpwgraph_alsamidi",
			SND_SEQ_PORT_CAP_WRITE
			| SND_SEQ_PORT_CAP_SUBS_WRITE
			| SND_SEQ_PORT_CAP_NO_EXPORT,
			SND_SEQ_PORT_TYPE_APPLICATION
		);

	if (port_id < 0) {
		snd_seq_close(m_seq);
		m_seq = nullptr;
		return false;
	}

	snd_seq_port_subscribe_t *seq_subs;
	snd_seq_addr_t seq_addr;
	struct pollfd seq_fds[1];
	snd_seq_port_subscribe_alloca(&seq_subs);
	seq_addr.client = SND_SEQ_CLIENT_SYSTEM;
	seq_addr.port = SND_SEQ_PORT_SYSTEM_ANNOUNCE;
	snd_seq_port_subscribe_set_sender(seq_subs, &seq_addr);
	seq_addr.client = snd_seq_client_id(m_seq);
	seq_addr.port = port_id;
	snd_seq_port_subscribe_set_dest(seq_subs, &seq_addr);
	snd_seq_subscribe_port(m_seq, seq_subs);
	snd_seq_poll_descriptors(m_seq, seq_fds, 1, POLLIN);

	m_notifier = new QSocketNotifier(seq_fds[0].fd, QSocketNotifier::Read);

	QObject::connect(m_notifier,
		SIGNAL(activated(int)),
		SLOT(changedNotify()));

	return true;
}


void qpwgraph_alsamidi::close (void)
{
	QMutexLocker locker(&g_mutex);

	if (m_seq == nullptr)
		return;

	if (m_notifier) {
		delete m_notifier;
		m_notifier = nullptr;
	}

	snd_seq_close(m_seq);

	m_seq = nullptr;
}


// Callback notifiers.
void qpwgraph_alsamidi::changedNotify (void)
{
	if (m_seq == nullptr)
		return;

	do {
		snd_seq_event_t *seq_event;
		snd_seq_event_input(m_seq, &seq_event);
		snd_seq_free_event(seq_event);
	}
	while (snd_seq_event_input_pending(m_seq, 0) > 0);

	emit changed();
}



// ALSA port (dis)connection.
void qpwgraph_alsamidi::connectPorts (
	qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect )
{
	if (m_seq == nullptr)
		return;

	if (port1 == nullptr || port2 == nullptr)
		return;

	const qpwgraph_node *node1 = port1->portNode();
	const qpwgraph_node *node2 = port2->portNode();

	if (node1 == nullptr || node2 == nullptr)
		return;

	QMutexLocker locker(&g_mutex);

	const int client_id1
		= node1->nodeName().section(':', 0, 0).toInt();
	const int port_id1
		= port1->portName().section(':', 0, 0).toInt();

	const int client_id2
		= node2->nodeName().section(':', 0, 0).toInt();
	const int port_id2
		= port2->portName().section(':', 0, 0).toInt();

#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_alsamidi::connectPorts(%d:%d, %d:%d, %d)",
		client_id1, port_id1, client_id2, port_id2, is_connect);
#endif

	snd_seq_port_subscribe_t *seq_subs;
	snd_seq_addr_t seq_addr;

	snd_seq_port_subscribe_alloca(&seq_subs);

	seq_addr.client = client_id1;
	seq_addr.port = port_id1;
	snd_seq_port_subscribe_set_sender(seq_subs, &seq_addr);

	seq_addr.client = client_id2;
	seq_addr.port = port_id2;
	snd_seq_port_subscribe_set_dest(seq_subs, &seq_addr);

	if (is_connect) {
		snd_seq_subscribe_port(m_seq, seq_subs);
	} else {
		snd_seq_unsubscribe_port(m_seq, seq_subs);
	}
}


// ALSA node type inquirer. (static)
bool qpwgraph_alsamidi::isNodeType ( uint node_type )
{
	return (node_type == qpwgraph_alsamidi::nodeType());
}


// ALSA node type.
uint qpwgraph_alsamidi::nodeType (void)
{
	static
	const uint AlsaNodeType
		= qpwgraph_item::itemType("ALSA_NODE_TYPE");

	return AlsaNodeType;
}


// ALSA port type inquirer. (static)
bool qpwgraph_alsamidi::isPortType ( uint port_type )
{
	return (port_type == qpwgraph_alsamidi::midiPortType());
}


// ALSA port type.
uint qpwgraph_alsamidi::midiPortType (void)
{
	static
	const uint AlsaMidiPortType
		= qpwgraph_item::itemType("ALSA_PORT_TYPE");

	return AlsaMidiPortType;
}


// ALSA client:port finder and creator if not existing.
bool qpwgraph_alsamidi::findClientPort (
	snd_seq_client_info_t *client_info,
	snd_seq_port_info_t *port_info,
	qpwgraph_item::Mode port_mode,
	qpwgraph_node **node,
	qpwgraph_port **port,
	bool add_new )
{
	const int client_id
		= snd_seq_client_info_get_client(client_info);
	const int client_port_id
		= snd_seq_port_info_get_port(port_info);

	const QString& node_name
		= QString::number(client_id) + ':'
		+ QString::fromUtf8(snd_seq_client_info_get_name(client_info));
	const QString& port_name
		= QString::number(client_port_id) + ':'
		+ QString::fromUtf8(snd_seq_port_info_get_name(port_info));

	const uint node_type
		= qpwgraph_alsamidi::nodeType();
	const uint port_type
		= qpwgraph_alsamidi::midiPortType();

	qpwgraph_item::Mode node_mode = port_mode;

	const uint node_id = qHash(node_name);
	const uint port_id = qHash(port_name) ^ node_id;

	*node = qpwgraph_sect::findNode(node_id, node_mode, node_type);
	*port = nullptr;

	if (*node == nullptr && client_id >= 128) {
		node_mode = qpwgraph_item::Duplex;
		*node = qpwgraph_sect::findNode(node_id, node_mode, node_type);
	}

	if (*node)
		*port = (*node)->findPort(port_id, port_mode, port_type);

	if (add_new && *node == nullptr) {
		*node = new qpwgraph_node(node_id, node_name, node_mode, node_type);
		(*node)->setNodeIcon(QIcon(":/images/itemAlsamidi.png"));
		qpwgraph_sect::addItem(*node);
	}

	if (add_new && *port == nullptr && *node) {
		*port = (*node)->addPort(port_id, port_name, port_mode, port_type);
		(*port)->updatePortTypeColors(qpwgraph_sect::canvas());
		qpwgraph_sect::addItem(*port);
	}

	return (*node && *port);
}


// ALSA graph updater.
void qpwgraph_alsamidi::updateItems (void)
{
	QMutexLocker locker(&g_mutex);

	if (m_seq == nullptr)
		return;

#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_alsamidi::updateItems()");
#endif

	// 1. Client/ports inventory...
	//
	snd_seq_client_info_t *client_info1;
	snd_seq_port_info_t *port_info1;

	snd_seq_client_info_alloca(&client_info1);
	snd_seq_port_info_alloca(&port_info1);

	snd_seq_client_info_set_client(client_info1, -1);

	while (snd_seq_query_next_client(m_seq, client_info1) >= 0) {
		const int client_id
			= snd_seq_client_info_get_client(client_info1);
		if (0 >= client_id)	// Skip 0:System client...
			continue;
		snd_seq_port_info_set_client(port_info1, client_id);
		snd_seq_port_info_set_port(port_info1, -1);
		while (snd_seq_query_next_port(m_seq, port_info1) >= 0) {
			const unsigned int port_caps1
				= snd_seq_port_info_get_capability(port_info1);
			if (port_caps1 & SND_SEQ_PORT_CAP_NO_EXPORT)
				continue;
			qpwgraph_item::Mode port_mode1 = qpwgraph_item::None;
			const unsigned int port_is_input
				= (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE);
			if ((port_caps1 & port_is_input) == port_is_input) {
				port_mode1 = qpwgraph_item::Input;
				qpwgraph_node *node1 = nullptr;
				qpwgraph_port *port1 = nullptr;
				if (findClientPort(client_info1, port_info1,
						port_mode1, &node1, &port1, true)) {
					node1->setMarked(true);
					port1->setMarked(true);
				}
			}
			const unsigned int port_is_output
				= (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ);
			if ((port_caps1 & port_is_output) == port_is_output) {
				port_mode1 = qpwgraph_item::Output;
				qpwgraph_node *node1 = nullptr;
				qpwgraph_port *port1 = nullptr;
				if (findClientPort(client_info1, port_info1,
						port_mode1, &node1, &port1, true)) {
					node1->setMarked(true);
					port1->setMarked(true);
				}
			}
		}
	}

	// 2. Connections inventory...
	//
	snd_seq_client_info_t *client_info2;
	snd_seq_port_info_t *port_info2;

	snd_seq_client_info_alloca(&client_info2);
	snd_seq_port_info_alloca(&port_info2);

	snd_seq_query_subscribe_t *seq_subs;
	snd_seq_addr_t seq_addr;

	snd_seq_query_subscribe_alloca(&seq_subs);

	snd_seq_client_info_set_client(client_info1, -1);

	while (snd_seq_query_next_client(m_seq, client_info1) >= 0) {
		const int client_id
			= snd_seq_client_info_get_client(client_info1);
		if (0 >= client_id)	// Skip 0:system client...
			continue;
		snd_seq_port_info_set_client(port_info1, client_id);
		snd_seq_port_info_set_port(port_info1, -1);
		while (snd_seq_query_next_port(m_seq, port_info1) >= 0) {
			const unsigned int port_caps1
				= snd_seq_port_info_get_capability(port_info1);
			if (port_caps1 & SND_SEQ_PORT_CAP_NO_EXPORT)
				continue;
			if (port_caps1 & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) {
				const qpwgraph_item::Mode port_mode1
					= qpwgraph_item::Output;
				qpwgraph_node *node1 = nullptr;
				qpwgraph_port *port1 = nullptr;
				if (!findClientPort(client_info1, port_info1,
						port_mode1, &node1, &port1, false))
					continue;
				snd_seq_query_subscribe_set_type(seq_subs, SND_SEQ_QUERY_SUBS_READ);
				snd_seq_query_subscribe_set_index(seq_subs, 0);
				seq_addr.client = client_id;
				seq_addr.port = snd_seq_port_info_get_port(port_info1);
				snd_seq_query_subscribe_set_root(seq_subs, &seq_addr);
				while (snd_seq_query_port_subscribers(m_seq, seq_subs) >= 0) {
					seq_addr = *snd_seq_query_subscribe_get_addr(seq_subs);
					if (snd_seq_get_any_client_info(m_seq,
							seq_addr.client, client_info2) >= 0 &&
						snd_seq_get_any_port_info(m_seq,
							seq_addr.client, seq_addr.port, port_info2) >= 0) {
						const qpwgraph_item::Mode port_mode2
							= qpwgraph_item::Input;
						qpwgraph_node *node2 = nullptr;
						qpwgraph_port *port2 = nullptr;
						if (findClientPort(client_info2, port_info2,
								port_mode2, &node2, &port2, false)) {
							qpwgraph_connect *connect = port1->findConnect(port2);
							if (connect == nullptr) {
								connect = new qpwgraph_connect();
								connect->setPort1(port1);
								connect->setPort2(port2);
								connect->updatePortTypeColors();
								connect->updatePath();
								qpwgraph_sect::addItem(connect);
							}
							if (connect)
								connect->setMarked(true);
						}
					}
					snd_seq_query_subscribe_set_index(seq_subs,
						snd_seq_query_subscribe_get_index(seq_subs) + 1);
				}
			}
		}
	}

	// 3. Clean-up all un-marked items...
	//
	qpwgraph_sect::resetItems(qpwgraph_alsamidi::nodeType());
}


void qpwgraph_alsamidi::clearItems (void)
{
	QMutexLocker locker(&g_mutex);

#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_alsamidi::clearItems()");
#endif

	qpwgraph_sect::clearItems(qpwgraph_alsamidi::nodeType());
}


// Special port-type colors defaults (virtual).
void qpwgraph_alsamidi::resetPortTypeColors (void)
{
	qpwgraph_canvas *canvas = qpwgraph_sect::canvas();
	if (canvas) {
		canvas->setPortTypeColor(
			qpwgraph_alsamidi::midiPortType(),
			QColor(Qt::darkMagenta).darker(120));
	}
}


#endif	// CONFIG_ALSA_MIDI


// end of qpwgraph_alsamidi.cpp
