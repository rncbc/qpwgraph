// qpwgraph_pipewire.cpp
//
/****************************************************************************
   Copyright (C) 2021-2024, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qpwgraph_pipewire.h"

#include "qpwgraph_canvas.h"
#include "qpwgraph_connect.h"

#include <pipewire/pipewire.h>

#include <QMutexLocker>
#include <QMultiHash>
#include <QTimer>


// Default port types...
#define DEFAULT_AUDIO_TYPE "32 bit float mono audio"
#define DEFAULT_MIDI_TYPE  "8 bit raw midi"
#define DEFAULT_VIDEO_TYPE "32 bit float RGBA video"


//----------------------------------------------------------------------------
// qpwgraph_pipewire icon cache.


static
QIcon qpwgraph_icon ( const QString& name )
{
	static QHash<QString, QIcon> icon_cache;

	QIcon icon = icon_cache.value(name);
	if (icon.isNull() && !name.isEmpty() && name != "node") {
		if (name.at(0) == ':')
			icon = QIcon(name);
		else
			icon = QIcon::fromTheme(name);
		if (!icon.isNull())
			icon_cache.insert(name, icon);
	}

	return icon;
}


//----------------------------------------------------------------------------
// qpwgraph_pipewire::Data -- PipeWire graph data structs.

struct qpwgraph_pipewire::Proxy
{
	qpwgraph_pipewire *pw;
	struct pw_proxy *proxy;
	void *info;
	pw_destroy_t destroy;
	struct spa_hook proxy_listener;
	struct spa_hook object_listener;
	int pending_seq;
	struct spa_list pending_link;
};

struct qpwgraph_pipewire::Object
{
	enum Type { Node, Port, Link };

	Object(uint oid, Type otype) : id(oid), type(otype), p(nullptr) {}

	virtual ~Object() { destroy_proxy(); }

	void create_proxy(qpwgraph_pipewire *pw);
	void destroy_proxy ();

	uint id;
	Type type;

	Proxy *p;
};

struct qpwgraph_pipewire::Node : public qpwgraph_pipewire::Object
{
	Node (uint node_id) : Object(node_id, Type::Node) {}

	enum NodeType {
		None  = 0,
		Audio = 1,
		Video = 2,
		Midi  = 4
	};

	struct NameKey
	{
		NameKey (Node *node)
			: node_name(node->node_name),
				node_mode(node->node_mode),
				node_type(node->node_type) {}

		NameKey (const NameKey& key)
			: node_name(key.node_name),
				node_mode(key.node_mode),
				node_type(key.node_type) {}

		bool operator== (const NameKey& key) const
		{
			return node_type == key.node_type
				&& node_mode == key.node_mode
				&& node_name == key.node_name;
		}

		QString node_name;
		qpwgraph_item::Mode node_mode;
		uint node_type;
	};

	QString node_name;
	QString node_nick;
	qpwgraph_item::Mode node_mode;
	NodeType node_type;
	QList<qpwgraph_pipewire::Port *> node_ports;
	QIcon node_icon;
	QString media_name;
	bool node_changed;
	bool node_ready;
	uint name_num;
};

struct qpwgraph_pipewire::Port : public qpwgraph_pipewire::Object
{
	Port (uint port_id) : Object(port_id, Type::Port) {}

	enum Flags {
		None     = 0,
		Physical = 1,
		Terminal = 2,
		Monitor  = 4,
		Control  = 8
	};

	uint node_id;
	QString port_name;
	qpwgraph_item::Mode port_mode;
	uint port_type;
	Flags port_flags;
	QList<qpwgraph_pipewire::Link *> port_links;
};

struct qpwgraph_pipewire::Link : public qpwgraph_pipewire::Object
{
	Link (uint link_id) : Object(link_id, Type::Link) {}

	uint port1_id;
	uint port2_id;
};

struct qpwgraph_pipewire::Data
{
	struct pw_thread_loop *loop;
	struct pw_context *context;

	struct pw_core *core;
	struct spa_hook core_listener;

	struct pw_registry *registry;
	struct spa_hook registry_listener;

	int pending_seq;
	struct spa_list pending;
	int last_seq;
	int last_res;
	bool error;

	typedef QMultiHash<Node::NameKey, uint> NodeNames;

	NodeNames *node_names;
};

inline uint qHash ( const qpwgraph_pipewire::Node::NameKey& key )
{
	return qHash(key.node_name) ^ qHash(uint(key.node_mode)) ^ qHash(key.node_type);
}


// sync-methods...
static
void qpwgraph_add_pending ( qpwgraph_pipewire::Proxy *p )
{
	qpwgraph_pipewire *pw = p->pw;
	qpwgraph_pipewire::Data *pd = pw->data();
	if (p->pending_seq == 0)
		spa_list_append(&pd->pending, &p->pending_link);
	p->pending_seq = pw_core_sync(pd->core, 0, p->pending_seq);
	pd->pending_seq = p->pending_seq;
}

static
void qpwgraph_remove_pending ( qpwgraph_pipewire::Proxy *p )
{
	if (p->pending_seq != 0) {
		spa_list_remove(&p->pending_link);
		p->pending_seq = 0;
	}
}
// sync-methods.


// node-events...
static
void qpwgraph_node_event_info ( void *data, const struct pw_node_info *info )
{
	qpwgraph_pipewire::Object *object
		= static_cast<qpwgraph_pipewire::Object *> (data);
	if (object && object->p) {
		info = pw_node_info_update((struct pw_node_info *)object->p->info, info);
		object->p->info = (void *)info;
		// Get node icon and media.name, if any...
		if (info && (info->change_mask & PW_NODE_CHANGE_MASK_PROPS)) {
			qpwgraph_pipewire::Node *node
				= static_cast<qpwgraph_pipewire::Node *> (object);
			if (node) {
				QIcon node_icon;
				const char *icon_name
					= spa_dict_lookup(info->props, PW_KEY_APP_ICON_NAME);
				if (icon_name && ::strlen(icon_name) > 0)
					node_icon = qpwgraph_icon(icon_name);
				if (node_icon.isNull())
					node_icon = qpwgraph_icon(node->node_name.toLower());
				if (node_icon.isNull()) {
					const char *client_api
						= spa_dict_lookup(info->props, PW_KEY_CLIENT_API);
					if (client_api && ::strlen(client_api) > 0) {
						if (::strcmp(client_api, "jack") == 0 ||
							::strcmp(client_api, "pipewire-jack") == 0) {
							node_icon = qpwgraph_icon(":images/itemJack.png");
						}
						else
						if (::strcmp(client_api, "pulse") == 0 ||
							::strcmp(client_api, "pipewire-pulse") == 0) {
							node_icon = qpwgraph_icon(":images/itemPulse.png");
						}
					}
				}
				if (!node_icon.isNull())
					node->node_icon = node_icon;
				const char *media_name
					= spa_dict_lookup(info->props, PW_KEY_MEDIA_NAME);
				if (media_name && ::strlen(media_name) > 0) {
					node->media_name = media_name;
					if (node->node_nick.isEmpty()) {
						qpwgraph_pipewire *pw = nullptr;
						if (node->p)
							pw = (node->p)->pw;
						qpwgraph_pipewire::Data::NodeNames *node_names = nullptr;
						if (pw && pw->data())
							node_names = (pw->data())->node_names;
						if (node_names) {
							node_names->remove(
								qpwgraph_pipewire::Node::NameKey(node),
								node->name_num);
							node->node_name = node->media_name;
							node->media_name.clear();
							node_names->insert(
								qpwgraph_pipewire::Node::NameKey(node),
								node->name_num);
						}
					}
				}
				node->node_changed = true;
				node->node_ready = true;
				if (object->p->pw)
					object->p->pw->changedNotify();
			}
		}
	}
}

static
const struct pw_node_events qpwgraph_node_events = {
	.version = PW_VERSION_NODE_EVENTS,
	.info = qpwgraph_node_event_info,
};
// node-events.


// port-events...
static
void qpwgraph_port_event_info ( void *data, const struct pw_port_info *info )
{
	qpwgraph_pipewire::Object *object
		= static_cast<qpwgraph_pipewire::Object *> (data);
	if (object && object->p) {
		info = pw_port_info_update((struct pw_port_info *)object->p->info, info);
		object->p->info = (void *)info;
	}
}

static
const struct pw_port_events qpwgraph_port_events = {
	.version = PW_VERSION_PORT_EVENTS,
	.info = qpwgraph_port_event_info,
};
// port-events.


// link-events...
static
void qpwgraph_link_event_info ( void *data, const struct pw_link_info *info )
{
	qpwgraph_pipewire::Object *object
		= static_cast<qpwgraph_pipewire::Object *> (data);
	if (object && object->p) {
		info = pw_link_info_update((struct pw_link_info *)object->p->info, info);
		object->p->info = (void *)info;
	}
}

static
const struct pw_link_events qpwgraph_link_events = {
	.version = PW_VERSION_LINK_EVENTS,
	.info = qpwgraph_link_event_info,
};
// link-events.


// proxy-events...
static void
qpwgraph_proxy_removed ( void *data )
{
	qpwgraph_pipewire::Object *object
		= static_cast<qpwgraph_pipewire::Object *> (data);
	if (object && object->p && object->p->proxy) {
		struct pw_proxy *proxy = object->p->proxy;
		object->p->proxy = nullptr;
		pw_proxy_destroy(proxy);
	}
}

static void
qpwgraph_proxy_destroy ( void *data )
{
	qpwgraph_pipewire::Object *object
		= static_cast<qpwgraph_pipewire::Object *> (data);
	if (object)
		object->destroy_proxy();
}

static
const struct pw_proxy_events qpwgraph_proxy_events = {
	.version = PW_VERSION_PROXY_EVENTS,
	.destroy = qpwgraph_proxy_destroy,
	.removed = qpwgraph_proxy_removed,
};
// proxy-events.

// proxy-methods...
void qpwgraph_pipewire::Object::create_proxy ( qpwgraph_pipewire *pw )
{
	if (p) return;

	const char *proxy_type = nullptr;
	uint32_t version = 0;
	pw_destroy_t destroy = nullptr;
	const void *events = nullptr;

	switch (type) {
	case Node:
		proxy_type = PW_TYPE_INTERFACE_Node;
		version = PW_VERSION_NODE;
		destroy = (pw_destroy_t) pw_node_info_free;
		events = &qpwgraph_node_events;
		break;
	case Port:
		proxy_type = PW_TYPE_INTERFACE_Port;
		version = PW_VERSION_PORT;
		destroy = (pw_destroy_t) pw_port_info_free;
		events = &qpwgraph_port_events;
		break;
	case Link:
		proxy_type = PW_TYPE_INTERFACE_Link;
		version = PW_VERSION_LINK;
		destroy = (pw_destroy_t) pw_link_info_free;
		events = &qpwgraph_link_events;
		break;
	}

	struct pw_proxy *proxy = (struct pw_proxy *)pw_registry_bind(
		pw->data()->registry, id, proxy_type, version, sizeof(Proxy));
	if (proxy)
		p = (Proxy *)pw_proxy_get_user_data(proxy);
	if (p) {
		p->pw = pw;
		p->proxy = proxy;
		p->destroy = destroy;
		p->pending_seq = 0;
		pw_proxy_add_object_listener(proxy,
			&p->object_listener, events, this);
		pw_proxy_add_listener(proxy,
			&p->proxy_listener, &qpwgraph_proxy_events, this);
	}
}

void qpwgraph_pipewire::Object::destroy_proxy (void)
{
	if (p == nullptr)
		return;

	spa_hook_remove(&p->object_listener);
	spa_hook_remove(&p->proxy_listener);

	qpwgraph_remove_pending(p);

	if (p->info && p->destroy) {
		p->destroy(p->info);
		p->info = nullptr;
	}

	if (p->proxy) {
		pw_proxy_destroy(p->proxy);
		p->proxy = nullptr;
	}

	p = nullptr;
}
// proxy-methods.


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
	if (props == nullptr)
		return;

	qpwgraph_pipewire *pw = static_cast<qpwgraph_pipewire *> (data);
#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_registry_event_global[%p]: id:%u type:%s/%u", pw, id, type, version);
#endif

	int nchanged = 0;

	if (::strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
		QString node_name;
		const char *str = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
		const char *nick = spa_dict_lookup(props, PW_KEY_NODE_NICK);
		if (str == nullptr || ::strlen(str) < 1)
			str = nick;
		if (str == nullptr || ::strlen(str) < 1)
			str = nick = spa_dict_lookup(props, PW_KEY_NODE_NAME);
		if (str == nullptr || ::strlen(str) < 1)
			str = "node";
		const char *app = spa_dict_lookup(props, PW_KEY_APP_NAME);
		if (app && ::strlen(app) > 0 && ::strcmp(app, str) != 0) {
			node_name += app;
			node_name += '/';
		}
		node_name += str;
		const QString node_nick(nick ? nick : str);
		qpwgraph_item::Mode node_mode = qpwgraph_item::None;
		uint node_types = qpwgraph_pipewire::Node::None;
		str = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
		if (str) {
			const QString media_class(str);
			if (media_class.contains("Source") ||
				media_class.contains("Output"))
				node_mode = qpwgraph_item::Output;
			else
			if (media_class.contains("Sink") ||
				media_class.contains("Input"))
				node_mode = qpwgraph_item::Input;
			if (media_class.contains("Audio"))
				node_types |= qpwgraph_pipewire::Node::Audio;
			if (media_class.contains("Video"))
				node_types |= qpwgraph_pipewire::Node::Video;
			if (media_class.contains("Midi"))
				node_types |= qpwgraph_pipewire::Node::Midi;
		}
		if (node_mode == qpwgraph_item::None) {
			str = spa_dict_lookup(props, PW_KEY_MEDIA_CATEGORY);
			if (str) {
				const QString media_category(str);
				if (media_category.contains("Duplex"))
					node_mode = qpwgraph_item::Duplex;
			}
		}
		if (pw->createNode(id, node_name, node_nick, node_mode, node_types))
			++nchanged;
	}
	else
	if (::strcmp(type, PW_TYPE_INTERFACE_Port) == 0) {
		const char *str = spa_dict_lookup(props, PW_KEY_NODE_ID);
		const uint node_id = (str ? uint(::atoi(str)) : 0);
		QString port_name;
		str = spa_dict_lookup(props, PW_KEY_PORT_ALIAS);
		if (str == nullptr)
			str = spa_dict_lookup(props, PW_KEY_PORT_NAME);
		if (str == nullptr)
			str = "port";
		port_name += str;
		qpwgraph_pipewire::Node *n = pw->findNode(node_id);
		uint port_type = qpwgraph_pipewire::otherPortType();
		str = spa_dict_lookup(props, PW_KEY_FORMAT_DSP);
		if (str)
			port_type = qpwgraph_item::itemType(str);
		else
		if (n && n->node_type == qpwgraph_pipewire::Node::Video)
			port_type = qpwgraph_pipewire::videoPortType();
		qpwgraph_item::Mode port_mode = qpwgraph_item::None;
		str = spa_dict_lookup(props, PW_KEY_PORT_DIRECTION);
		if (str) {
			if (::strcmp(str, "in") == 0)
				port_mode = qpwgraph_item::Input;
			else
			if (::strcmp(str, "out") == 0)
				port_mode = qpwgraph_item::Output;
		}
		uint port_flags = qpwgraph_pipewire::Port::None;
		if (n && (n->node_mode != qpwgraph_item::Duplex))
			port_flags |= qpwgraph_pipewire::Port::Terminal;
		str = spa_dict_lookup(props, PW_KEY_PORT_PHYSICAL);
		if (str && pw_properties_parse_bool(str))
			port_flags |= qpwgraph_pipewire::Port::Physical;
		str = spa_dict_lookup(props, PW_KEY_PORT_TERMINAL);
		if (str && pw_properties_parse_bool(str))
			port_flags |= qpwgraph_pipewire::Port::Terminal;
		str = spa_dict_lookup(props, PW_KEY_PORT_MONITOR);
		if (str && pw_properties_parse_bool(str))
			port_flags |= qpwgraph_pipewire::Port::Monitor;
		str = spa_dict_lookup(props, PW_KEY_PORT_CONTROL);
		if (str && pw_properties_parse_bool(str))
			port_flags |= qpwgraph_pipewire::Port::Control;
		if (pw->createPort(id, node_id, port_name, port_mode, port_type, port_flags))
			++nchanged;
	}
	else
	if (::strcmp(type, PW_TYPE_INTERFACE_Link) == 0) {
		const char *str = spa_dict_lookup(props, PW_KEY_LINK_OUTPUT_PORT);
		const uint port1_id = (str ? uint(pw_properties_parse_int(str)) : 0);
		str = spa_dict_lookup(props, PW_KEY_LINK_INPUT_PORT);
		const uint port2_id = (str ? uint(pw_properties_parse_int(str)) : 0);
		if (pw->createLink(id, port1_id, port2_id))
			++nchanged;
	}

	if (nchanged > 0)
		pw->changedNotify();
}

static
void qpwgraph_registry_event_global_remove ( void *data, uint32_t id )
{
	qpwgraph_pipewire *pw = static_cast<qpwgraph_pipewire *> (data);
#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_registry_event_global_remove[%p]: id:%u", pw, id);
#endif

	pw->removeObjectEx(id);
	pw->changedNotify();
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
	qpwgraph_pipewire *pw = static_cast<qpwgraph_pipewire *> (data);
	qpwgraph_pipewire::Data *pd = pw->data();
#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_core_event_done[%p]: id:%u seq:%d", pd, id, seq);
#endif

	struct qpwgraph_pipewire::Proxy *p, *q;
	spa_list_for_each_safe(p, q, &pd->pending, pending_link) {
		if (p->pending_seq == seq)
			qpwgraph_remove_pending(p);
	}

	if (id == PW_ID_CORE) {
		pd->last_seq = seq;
		if (pd->pending_seq == seq)
			pw_thread_loop_signal(pd->loop, false);
	}
}

static
void qpwgraph_core_event_error (
	void *data, uint32_t id, int seq, int res, const char *message )
{
	qpwgraph_pipewire *pw = static_cast<qpwgraph_pipewire *> (data);
	qpwgraph_pipewire::Data *pd = pw->data();
#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_core_event_error[%p]: id:%u seq:%d res:%d : %s", pd, id, seq, res, message);
#endif

	if (id == PW_ID_CORE) {
		pd->last_res = res;
		if (res == -EPIPE)
			pd->error = true;
	}

	pw_thread_loop_signal(pd->loop, false);
}

static
const struct pw_core_events qpwgraph_core_events = {
	.version = PW_VERSION_CORE_EVENTS,
	.info = nullptr,
	.done = qpwgraph_core_event_done,
	.error = qpwgraph_core_event_error,
};
// core-events.


// link-events...
static
int qpwgraph_link_proxy_sync ( qpwgraph_pipewire *pw )
{
	qpwgraph_pipewire::Data *pd = pw->data();

	if (pw_thread_loop_in_thread(pd->loop))
		return 0;

	pd->pending_seq = pw_proxy_sync((struct pw_proxy *)pd->core, pd->pending_seq);

	while (true) {
		pw_thread_loop_wait(pd->loop);
		if (pd->error)
			return pd->last_res;
		if (pd->pending_seq == pd->last_seq)
			break;
	}

	return 0;
}

static
void qpwgraph_link_proxy_error ( void *data, int seq, int res, const char *message )
{
#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_link_proxy_error: seq:%d res:%d : %s", seq, res, message);
#endif

	int *link_res = (int *)data;
	*link_res = res;
}

static
const struct pw_proxy_events qpwgraph_link_proxy_events = {
	.version = PW_VERSION_PROXY_EVENTS,
	.error = qpwgraph_link_proxy_error,
};
// link-events.


//----------------------------------------------------------------------------
// qpwgraph_pipewire -- PipeWire graph driver

// Constructor.
qpwgraph_pipewire::qpwgraph_pipewire ( qpwgraph_canvas *canvas )
	: qpwgraph_sect(canvas), m_data(nullptr)
{
	resetPortTypeColors();

	if (!open())
		QTimer::singleShot(3000, this, SLOT(reset()));
}


// Destructor.
qpwgraph_pipewire::~qpwgraph_pipewire (void)
{
	close();
}


// Client methods.
bool qpwgraph_pipewire::open (void)
{
	QMutexLocker locker1(&m_mutex1);

	pw_init(nullptr, nullptr);

	m_data = new Data;
	spa_zero(*m_data);
	spa_list_init(&m_data->pending);
	m_data->pending_seq = 0;

	m_data->loop = pw_thread_loop_new("qpwgraph_thread_loop", nullptr);
	if (m_data->loop == nullptr) {
		qDebug("pw_thread_loop_new: Can't create thread loop.");
		delete m_data;
		pw_deinit();
		return false;
	}

	pw_thread_loop_lock(m_data->loop);

	struct pw_loop *loop = pw_thread_loop_get_loop(m_data->loop);
	m_data->context = pw_context_new(loop,
		nullptr /*properties*/, 0 /*user_data size*/);
	if (m_data->context == nullptr) {
		qDebug("pw_context_new: Can't create context.");
		pw_thread_loop_unlock(m_data->loop);
		pw_thread_loop_destroy(m_data->loop);
		delete m_data;
		m_data = nullptr;
		pw_deinit();
		return false;
	}

	m_data->core = pw_context_connect(m_data->context,
		nullptr /*properties*/, 0 /*user_data size*/);
	if (m_data->core == nullptr) {
		qDebug("pw_context_connect: Can't connect context.");
		pw_thread_loop_unlock(m_data->loop);
		pw_context_destroy(m_data->context);
		pw_thread_loop_destroy(m_data->loop);
		delete m_data;
		m_data = nullptr;
		pw_deinit();
		return false;
	}

	pw_core_add_listener(m_data->core,
		 &m_data->core_listener, &qpwgraph_core_events, this);

	m_data->registry = pw_core_get_registry(m_data->core,
		PW_VERSION_REGISTRY, 0 /*user_data size*/);

	pw_registry_add_listener(m_data->registry,
		&m_data->registry_listener, &qpwgraph_registry_events, this);

	m_data->pending_seq = 0;
	m_data->last_seq = 0;
	m_data->error = false;
	m_data->node_names = new Data::NodeNames;

	pw_thread_loop_start(m_data->loop);
	pw_thread_loop_unlock(m_data->loop);
	return true;
}


void qpwgraph_pipewire::close (void)
{
	if (m_data == nullptr)
		return;

	QMutexLocker locker1(&m_mutex1);

	pw_thread_loop_lock(m_data->loop);

	clearObjects();

	pw_thread_loop_unlock(m_data->loop);

	if (m_data->loop)
		pw_thread_loop_stop(m_data->loop);

	if (m_data->registry) {
		spa_hook_remove(&m_data->registry_listener);
		pw_proxy_destroy((struct pw_proxy*)m_data->registry);
	}

	if (m_data->core) {
		spa_hook_remove(&m_data->core_listener);
		pw_core_disconnect(m_data->core);
	}

	if (m_data->context)
		pw_context_destroy(m_data->context);

	if (m_data->loop)
		pw_thread_loop_destroy(m_data->loop);

	if (m_data->node_names)
		delete m_data->node_names;

	delete m_data;
	m_data = nullptr;

	pw_deinit();
}


// Get a brand new core and context...
void qpwgraph_pipewire::reset (void)
{
	clearItems();

	close();

	if (!open())
		QTimer::singleShot(3000, this, SLOT(reset()));
}


// Callback notifiers.
void qpwgraph_pipewire::changedNotify (void)
{
	emit changed();
}


// PipeWire port (dis)connection.
void qpwgraph_pipewire::connectPorts (
	qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect )
{
	if (m_data == nullptr)
		return;

	if (port1 == nullptr || port2 == nullptr)
		return;

	const qpwgraph_node *node1 = port1->portNode();
	const qpwgraph_node *node2 = port2->portNode();

	if (node1 == nullptr || node2 == nullptr)
		return;

	QMutexLocker locker1(&m_mutex1);

	pw_thread_loop_lock(m_data->loop);

	Port *p1 = findPort(port1->portId());
	Port *p2 = findPort(port2->portId());

	if ((p1 == nullptr || p2 == nullptr) ||
		(p1->port_mode & qpwgraph_item::Output) == 0 ||
		(p2->port_mode & qpwgraph_item::Input)  == 0 ||
		(p1->port_type != p2->port_type)) {
		pw_thread_loop_unlock(m_data->loop);
		return;
	}

	if (!is_connect) {
		// Disconnect ports...
		foreach (Link *link, p1->port_links) {
			if ((link->port1_id == p1->id) &&
				(link->port2_id == p2->id)) {
				pw_registry_destroy(m_data->registry, link->id);
				qpwgraph_link_proxy_sync(this);
				break;
			}
		}
		pw_thread_loop_unlock(m_data->loop);
		return;
	}

	// Connect ports...
	char val[4][16];
	::snprintf(val[0], sizeof(val[0]), "%u", p1->node_id);
	::snprintf(val[1], sizeof(val[1]), "%u", p1->id);
	::snprintf(val[2], sizeof(val[2]), "%u", p2->node_id);
	::snprintf(val[3], sizeof(val[3]), "%u", p2->id);

	struct spa_dict props;
	struct spa_dict_item items[6];
	props = SPA_DICT_INIT(items, 0);
	items[props.n_items++] = SPA_DICT_ITEM_INIT(PW_KEY_LINK_OUTPUT_NODE, val[0]);
	items[props.n_items++] = SPA_DICT_ITEM_INIT(PW_KEY_LINK_OUTPUT_PORT, val[1]);
	items[props.n_items++] = SPA_DICT_ITEM_INIT(PW_KEY_LINK_INPUT_NODE,  val[2]);
	items[props.n_items++] = SPA_DICT_ITEM_INIT(PW_KEY_LINK_INPUT_PORT,  val[3]);
	items[props.n_items++] = SPA_DICT_ITEM_INIT(PW_KEY_OBJECT_LINGER,    "true");
	const char *str = ::getenv("PIPEWIRE_LINK_PASSIVE");
	if (str && pw_properties_parse_bool(str))
		items[props.n_items++] = SPA_DICT_ITEM_INIT(PW_KEY_LINK_PASSIVE, "true");

	struct pw_proxy *proxy = (struct pw_proxy *)pw_core_create_object(m_data->core,
		"link-factory", PW_TYPE_INTERFACE_Link, PW_VERSION_LINK, &props, 0);
	if (proxy) {
		int link_res = 0;
		struct spa_hook listener;
		spa_zero(listener);
		pw_proxy_add_listener(proxy,
			&listener, &qpwgraph_link_proxy_events, &link_res);
		qpwgraph_link_proxy_sync(this);
		spa_hook_remove(&listener);
		pw_proxy_destroy(proxy);
	}

	pw_thread_loop_unlock(m_data->loop);
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
	uint node_id, uint port_id,  qpwgraph_item::Mode port_mode,
	qpwgraph_node **node, qpwgraph_port **port, bool add_new )
{
	qpwgraph_canvas *canvas = qpwgraph_sect::canvas();
	if (canvas == nullptr)
		return false;

	Node *n = findNode(node_id);
	if (n == nullptr)
		return false;
	if (!n->node_ready)
		return false;

	Port *p = findPort(port_id);
	if (p == nullptr)
		return false;

	const uint node_type
		= qpwgraph_pipewire::nodeType();
	qpwgraph_item::Mode node_mode
		= port_mode;
	const uint port_type
		= p->port_type;

	*node = qpwgraph_sect::findNode(node_id, node_mode, node_type);
	*port = nullptr;

	if (*node == nullptr) {
		const uint port_flags = p->port_flags;
		const uint port_flags_mask
			= (Port::Physical | Port::Terminal);
		if ((port_flags & port_flags_mask) != port_flags_mask) {
			node_mode = qpwgraph_item::Duplex;
			*node = qpwgraph_sect::findNode(node_id, node_mode, node_type);
		}
	}

	if (*node && m_recycled_nodes.value(qpwgraph_node::NodeIdKey(*node), nullptr))
		return false;

	if (*node && n->node_changed) {
		canvas->releaseNode(*node);
		*node = nullptr;
	}

	if (*node)
		*port = (*node)->findPort(port_id, port_mode, port_type);

	if (*port && m_recycled_ports.value(qpwgraph_port::PortIdKey(*port), nullptr))
		return false;

	if (add_new && *node == nullptr && !canvas->isFilterNodes(n->node_name)) {
		QString node_name = n->node_name;
		if ((p->port_flags & Port::Physical) == Port::None) {
			if (n->name_num > 0) {
				node_name += '-';
				node_name += QString::number(n->name_num);
			}
			if (p->port_flags & Port::Monitor) {
				node_name += ' ';
				node_name += "[Monitor]";
			}
			if (p->port_flags & Port::Control) {
				node_name += ' ';
				node_name += "[Control]";
			}
		}
		*node = new qpwgraph_node(node_id, node_name, node_mode, node_type);
		(*node)->setNodeIcon(n->node_icon);
		(*node)->setNodeLabel(n->media_name);
		(*node)->setNodePrefix(n->node_nick);
		n->node_changed = false;
		qpwgraph_sect::addItem(*node);
	}

	if (add_new && *port == nullptr && *node) {
		*port = (*node)->addPort(port_id, p->port_name, port_mode, port_type);
		(*port)->updatePortTypeColors(canvas);
		qpwgraph_sect::addItem(*port);
	}

	return (*node && *port);
}


// PipeWire graph updaters.
void qpwgraph_pipewire::updateItems (void)
{
	if (m_data == nullptr)
		return;

#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_pipewire::updateItems()");
#endif
	QMutexLocker locker1(&m_mutex1);
	QMutexLocker locker2(&m_mutex2);

	// 0. Check for core errors...
	//
	if (m_data->error) {
		QTimer::singleShot(3000, this, SLOT(reset()));
		return;
	}

	// 1. Nodes/ports inventory...
	//
	QList<qpwgraph_port *> ports;

	foreach (Object *object, m_objects) {
		if (object->type != Object::Node)
			continue;
		Node *n1 = static_cast<Node *> (object);
		if (!n1->node_ready)
			continue;
		foreach (const Port *p1, n1->node_ports) {
			const qpwgraph_item::Mode port_mode1
				= p1->port_mode;
			qpwgraph_node *node1 = nullptr;
			qpwgraph_port *port1 = nullptr;
			if (findNodePort(n1->id, p1->id,
					port_mode1, &node1, &port1, true)) {
				node1->setMarked(true);
				port1->setMarked(true);
				if ((port_mode1 & qpwgraph_item::Output)
					&& (!p1->port_links.isEmpty())) {
					ports.append(port1);
				}
			}
		}
	}

	// 2. Links inventory...
	//
	foreach (qpwgraph_port *port1, ports) {
		Port *p1 = findPort(port1->portId());
		if (p1 == nullptr)
			continue;
		foreach (const Link *link, p1->port_links) {
			Port *p2 = findPort(link->port2_id);
			if (p2 == nullptr)
				continue;
			const qpwgraph_item::Mode port_mode2
				= qpwgraph_item::Input;
			qpwgraph_node *node2 = nullptr;
			qpwgraph_port *port2 = nullptr;
			if (findNodePort(p2->node_id, link->port2_id,
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
	}

	// 3. Clean-up all un-marked items...
	//
	qpwgraph_sect::resetItems(qpwgraph_pipewire::nodeType());

	m_recycled_nodes.clear();
	m_recycled_ports.clear();
}


void qpwgraph_pipewire::clearItems (void)
{
	if (m_data == nullptr)
		return;

#ifdef CONFIG_DEBUG
	qDebug("qpwgraph_pipewire::clearItems()");
#endif
	QMutexLocker locker1(&m_mutex1);

	// Clean-up all items...
	//
	qpwgraph_sect::clearItems(qpwgraph_pipewire::nodeType());

	m_recycled_nodes.clear();
	m_recycled_ports.clear();
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


// PipeWire client data struct access.
//
qpwgraph_pipewire::Data *qpwgraph_pipewire::data (void) const
{
	return m_data;
}


// Object methods.
//
qpwgraph_pipewire::Object *qpwgraph_pipewire::findObject ( uint id ) const
{
	return m_objectids.value(id, nullptr);
}


void qpwgraph_pipewire::addObject ( uint id, Object *object )
{
	object->create_proxy(this);

	m_objectids.insert(id, object);
	m_objects.append(object);
}


void qpwgraph_pipewire::removeObject ( uint id )
{
	Object *object = findObject(id);
	if (object == nullptr)
		return;

	m_objectids.remove(id);
	m_objects.removeAll(object);

	if (object->type == Object::Node)
		destroyNode(static_cast<Node *> (object));
	else
	if (object->type == Object::Port)
		destroyPort(static_cast<Port *> (object));
	else
	if (object->type == Object::Link)
		destroyLink(static_cast<Link *> (object));
}


void qpwgraph_pipewire::clearObjects (void)
{
	qDeleteAll(m_objects);
	m_objects.clear();
	m_objectids.clear();
}


void qpwgraph_pipewire::removeObjectEx ( uint id )
{
	QMutexLocker locker2(&m_mutex2);

	removeObject(id);
}


void qpwgraph_pipewire::addObjectEx ( uint id, Object *object )
{
	QMutexLocker locker2(&m_mutex2);

	addObject(id, object);
}


// Node methods.
//
qpwgraph_pipewire::Node *qpwgraph_pipewire::findNode ( uint node_id ) const
{
	Node *node = static_cast<Node *> (findObject(node_id));
	return (node && node->type == Object::Node ? node : nullptr);
}


qpwgraph_pipewire::Node *qpwgraph_pipewire::createNode (
	uint node_id,
	const QString& node_name,
	const QString& node_nick,
	qpwgraph_item::Mode node_mode,
	uint node_type )
{
	recycleNode(node_id, node_mode);

	Node *node = new Node(node_id);
	node->node_name = node_name;
	node->node_nick = node_nick;
	node->node_mode = node_mode;
	node->node_type = Node::NodeType(node_type);
	node->node_icon = qpwgraph_icon(":/images/itemPipewire.png");
	node->node_changed = false;
	node->node_ready = false;
	node->name_num = 0;

	Data::NodeNames *node_names = nullptr;
	if (m_data)
		node_names = m_data->node_names;
	if (node_names) {
		const Node::NameKey name_key(node);
		Data::NodeNames::Iterator name_iter
			= node_names->find(name_key, node->name_num);
		while (name_iter != node_names->end())
			name_iter = node_names->find(name_key, ++(node->name_num));
		node_names->insert(name_key, node->name_num);
	}

	addObjectEx(node_id, node);

	return node;
}


void qpwgraph_pipewire::destroyNode ( Node *node )
{
	Data::NodeNames *node_names = nullptr;
	if (m_data)
		node_names = m_data->node_names;
	if (node_names)
		node_names->remove(Node::NameKey(node), node->name_num);

	foreach (const Port *port, node->node_ports)
		removeObject(port->id);

	node->node_ports.clear();

	delete node;
}


// Port methods.
//
qpwgraph_pipewire::Port *qpwgraph_pipewire::findPort ( uint port_id ) const
{
	Port *port = static_cast<Port *> (findObject(port_id));
	return (port && port->type == Object::Port ? port : nullptr);
}


qpwgraph_pipewire::Port *qpwgraph_pipewire::createPort (
	uint port_id,
	uint node_id,
	const QString& port_name,
	qpwgraph_item::Mode port_mode,
	uint port_type,
	uint port_flags )
{
	recyclePort(port_id, node_id, port_mode, port_type);

	Node *node = findNode(node_id);
	if (node == nullptr)
		return nullptr;

	Port *port = new Port(port_id);
	port->node_id = node_id;
	port->port_name = port_name;
	port->port_mode = port_mode;
	port->port_type = port_type;
	port->port_flags = Port::Flags(port_flags);

	node->node_ports.append(port);

	addObjectEx(port_id, port);

	return port;
}


void qpwgraph_pipewire::destroyPort ( Port *port )
{
	Node *node = findNode(port->node_id);
	if (node == nullptr)
		return;

	foreach (const Link *link, port->port_links)
		removeObject(link->id);

	port->port_links.clear();
	node->node_ports.removeAll(port);

	delete port;
}


// Link methods.
//
qpwgraph_pipewire::Link *qpwgraph_pipewire::findLink ( uint link_id ) const
{
	Link *link = static_cast<Link *> (findObject(link_id));
	return (link && link->type == Object::Link ? link : nullptr);
}


qpwgraph_pipewire::Link *qpwgraph_pipewire::createLink (
	uint link_id, uint port1_id, uint port2_id )
{
	Port *port1 = findPort(port1_id);
	if (port1 == nullptr)
		return nullptr;
	if ((port1->port_mode & qpwgraph_item::Output) == 0)
		return nullptr;

	Port *port2 = findPort(port2_id);
	if (port2 == nullptr)
		return nullptr;
	if ((port2->port_mode & qpwgraph_item::Input) == 0)
		return nullptr;

	Link *link = new Link(link_id);
	link->port1_id = port1_id;
	link->port2_id = port2_id;

	port1->port_links.append(link);

	addObjectEx(link_id, link);

	return link;
}


void qpwgraph_pipewire::destroyLink ( Link *link )
{
	Port *port = findPort(link->port1_id);
	if (port == nullptr)
		return;

	port->port_links.removeAll(link);

	delete link;
}


// Special node finder...
qpwgraph_node *qpwgraph_pipewire::findNode (
	uint node_id, qpwgraph_item::Mode node_mode ) const
{
	const uint node_type = qpwgraph_pipewire::nodeType();
	qpwgraph_node *node = qpwgraph_sect::findNode(node_id, node_mode, node_type);
	if (node == nullptr)
		node = qpwgraph_sect::findNode(node_id, qpwgraph_item::Duplex, node_type);

	return node;
}


// Special node recycler...
void qpwgraph_pipewire::recycleNode (
	uint node_id, qpwgraph_item::Mode node_mode )
{
	qpwgraph_node *node = findNode(node_id, node_mode);
	if (node)
		m_recycled_nodes.insert(qpwgraph_node::NodeIdKey(node), node);
}


// Special port recycler...
void qpwgraph_pipewire::recyclePort (
	uint port_id, uint node_id, qpwgraph_item::Mode port_mode, uint port_type )
{
	qpwgraph_node *node = findNode(node_id, port_mode);
	if (node) {
		qpwgraph_port *port = node->findPort(port_id, port_mode, port_type);
		if (port)
			m_recycled_ports.insert(qpwgraph_port::PortIdKey(port), port);
	}
}


// end of qpwgraph_pipewire.cpp
