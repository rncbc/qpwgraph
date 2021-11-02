// qpwgraph_pipewire.cpp
//
/****************************************************************************
   Copyright (C) 2021, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qpwgraph.h"
#include "qpwgraph_pipewire.h"

#include "qpwgraph_canvas.h"
#include "qpwgraph_connect.h"

#include <pipewire/pipewire.h>

#include <QMutexLocker>


// Default port types...
#define DEFAULT_AUDIO_TYPE "32 bit float mono audio"
#define DEFAULT_MIDI_TYPE  "8 bit raw midi"
#define DEFAULT_VIDEO_TYPE "32 bit float RGBA video"


//----------------------------------------------------------------------------
// qpwgraph_pipewire::Data -- PipeWire graph driver

struct qpwgraph_pipewire::Data
{
	struct pw_thread_loop *loop;
	struct pw_context *context;

	struct pw_core *core;
	struct spa_hook core_listener;

	struct pw_registry *registry;
	struct spa_hook registry_listener;
};


// registry-events...
static
void qpwgraph_registry_event_global (
	void *data,
	uint32_t id,
	uint32_t permissions,
	const char *type,
	uint32_t version,
	const struct spa_dict *props )
{
	qpwgraph_pipewire::Data *d = static_cast<qpwgraph_pipewire::Data *> (data);
#ifdef CONFIG_DEBUG
	qDebug("gpwgraph_registry_event_global[%p]: id:%u type:%s/%u", d, id, type, version);
#endif

	// TODO: ?...
	//
}

static
void qpwgraph_registry_event_global_remove ( void *data, uint32_t id )
{
	qpwgraph_pipewire::Data *d = static_cast<qpwgraph_pipewire::Data *> (data);
#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_registry_event_global_remove[%p]: id:%u", d, id);
#endif

	// TODO: ?...
	//
}

static
const struct pw_registry_events qpwgraph_registry_events = {
	.version = PW_VERSION_REGISTRY_EVENTS,
	.global = qpwgraph_registry_event_global,
	.global_remove = qpwgraph_registry_event_global_remove,
};
// registry-events.


// core-events...
static
void qpwgraph_core_event_done ( void *data, uint32_t id, int seq )
{
	qpwgraph_pipewire::Data *d = static_cast<qpwgraph_pipewire::Data *> (data);
#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_core_event_done[%p]: id:%u seq:%d", d, id, seq);
#endif

	// TODO: ?...
	//
}

static
void qpwgraph_core_event_error (
	void *data, uint32_t id, int seq, int res, const char *message )
{
	qpwgraph_pipewire::Data *d = static_cast<qpwgraph_pipewire::Data *> (data);
#ifdef CONFIG_DEBUG
	qDebug("gpwgraph_core_event_error[%p]: id:%u seq:%d res:%d : %s", d, id, seq, res, message);
#endif

	// TODO: ?...
	//
}

static
const struct pw_core_events qpwgraph_core_events = {
	.version = PW_VERSION_CORE_EVENTS,
	.info = nullptr,
	.done = qpwgraph_core_event_done,
	.error = qpwgraph_core_event_error,
};
// core-events.


//----------------------------------------------------------------------------
// qpwgraph_pipewire -- PipeWire graph driver

QMutex qpwgraph_pipewire::g_mutex;


// Constructor.
qpwgraph_pipewire::qpwgraph_pipewire ( qpwgraph_canvas *canvas )
	: qpwgraph_sect(canvas), m_data(nullptr)
{
	resetPortTypeColors();

	open();
}


// Destructor.
qpwgraph_pipewire::~qpwgraph_pipewire (void)
{
	close();
}


// Client methods.
bool qpwgraph_pipewire::open (void)
{
	QMutexLocker locker(&g_mutex);

	pw_init(nullptr, nullptr);

	m_data = new Data;
	spa_zero(*m_data);

	m_data->loop = pw_thread_loop_new("gpwgraph_thread_loop", nullptr);
	if (m_data->loop == nullptr) {
		qDebug("pw_thread_loop_new: Can't create thread loop.");
		close();
		return false;
	}

	struct pw_loop *loop = pw_thread_loop_get_loop(m_data->loop);
	m_data->context = pw_context_new(loop,
		nullptr /*properties*/, 0 /*user_data size*/);
	if (m_data->context == nullptr) {
		qDebug("pw_context_new: Can't create context.");
		close();
		return false;
	}

	m_data->core = pw_context_connect(m_data->context,
		nullptr /*properties*/, 0 /*user_data size*/);
	if (m_data->core == nullptr) {
		qDebug("pw_context_connect: Can't connect context.");
		close();
		return false;
	}

	pw_core_add_listener(m_data->core,
		 &m_data->core_listener, &qpwgraph_core_events, this);

	m_data->registry = pw_core_get_registry(m_data->core,
		PW_VERSION_REGISTRY, 0 /*user_data size*/);
	pw_registry_add_listener(m_data->registry,
		&m_data->registry_listener, &qpwgraph_registry_events, this);

	pw_thread_loop_start(m_data->loop);
	return true;
}


void qpwgraph_pipewire::close (void)
{
	QMutexLocker locker(&g_mutex);

	if (m_data && m_data->registry)
		pw_proxy_destroy((struct pw_proxy*)m_data->registry);

	if (m_data && m_data->core)
		pw_core_disconnect(m_data->core);

	if (m_data && m_data->context)
		pw_context_destroy(m_data->context);

	if (m_data && m_data->loop)
		pw_thread_loop_destroy(m_data->loop);

	if (m_data) {
		delete m_data;
		m_data = nullptr;
	}

	pw_deinit();
}


// Callback notifiers.
void qpwgraph_pipewire::changedNotify (void)
{
	emit changed();
}


// PipeWire port (dis)connection.
void qpwgraph_pipewire::connectPorts (
	qpwgraph_port *port1, qpwgraph_port *port2, bool connect )
{
	if (port1 == nullptr || port2 == nullptr)
		return;

	const qpwgraph_node *node1 = port1->portNode();
	const qpwgraph_node *node2 = port2->portNode();

	if (node1 == nullptr || node2 == nullptr)
		return;

	QMutexLocker locker(&g_mutex);

	// TODO: ?...
	//
}


// PipeWire node type inquirer. (static)
bool qpwgraph_pipewire::isNodeType ( uint node_type )
{
	return (node_type == qpwgraph_pipewire::nodeType());
}


// PipeWire node type.
uint qpwgraph_pipewire::nodeType (void)
{
	static
	const uint PipeWireNodeType
		= qpwgraph_item::itemType("PIPEWIRE_NODE_TYPE");

	return PipeWireNodeType;
}


// PipeWire port type(s) inquirer. (static)
bool qpwgraph_pipewire::isPortType ( uint port_type )
{
	return port_type == audioPortType()
		|| port_type == midiPortType()
		|| port_type == videoPortType()
		|| port_type == otherPortType();
}

uint qpwgraph_pipewire::audioPortType (void)
{
	return qpwgraph_item::itemType(DEFAULT_AUDIO_TYPE);
}

uint qpwgraph_pipewire::midiPortType (void)
{
	return qpwgraph_item::itemType(DEFAULT_MIDI_TYPE);
}

uint qpwgraph_pipewire::videoPortType (void)
{
	return qpwgraph_item::itemType(DEFAULT_VIDEO_TYPE);
}

uint qpwgraph_pipewire::otherPortType (void)
{
	return qpwgraph_item::itemType("PIPEWIRE_PORT_TYPE");
}


// PipeWire node:port finder and creator if not existing.
bool qpwgraph_pipewire::findNodePort (
	uint32_t node_id, uint32_t port_id,  qpwgraph_item::Mode port_mode,
	qpwgraph_node **node, qpwgraph_port **port, bool add_new )
{
	// TODO: ?...
	//

	return (*node && *port);
}


// Special port-type colors defaults (virtual).
void qpwgraph_pipewire::resetPortTypeColors (void)
{
	qpwgraph_canvas *canvas = qpwgraph_sect::canvas();
	if (canvas) {
		canvas->setPortTypeColor(
			qpwgraph_pipewire::audioPortType(),
			QColor(Qt::darkGreen).darker(120));
		canvas->setPortTypeColor(
			qpwgraph_pipewire::midiPortType(),
			QColor(Qt::darkRed).darker(120));
		canvas->setPortTypeColor(
			qpwgraph_pipewire::videoPortType(),
			QColor(Qt::darkCyan).darker(120));
		canvas->setPortTypeColor(
			qpwgraph_pipewire::otherPortType(),
			QColor(Qt::darkYellow).darker(120));
	}
}


// Node/port renaming method (virtual override).
void qpwgraph_pipewire::renameItem (
	qpwgraph_item *item, const QString& name )
{
	// TODO: ?...
	//

	qpwgraph_sect::renameItem(item, name);
}


// end of qpwgraph_pipewire.cpp
