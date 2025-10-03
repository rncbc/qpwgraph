// qpwgraph_canvas.cpp
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

#include "qpwgraph_canvas.h"

#include "qpwgraph_connect.h"
#include "qpwgraph_patchbay.h"
#include "qpwgraph_toposort.h"

#include <QGraphicsScene>
#include <QRegularExpression>
#include <QTransform>

#include <QRubberBand>
#include <QUndoStack>
#include <QSettings>

#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include <QScrollBar>

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include <QGestureEvent>
#include <QPinchGesture>

#include <QQueue>

#include <algorithm>
#include <cmath>
#include <iostream>


// Local constants.
static const char *CanvasGroup      = "/GraphCanvas";
static const char *CanvasRectKey    = "/CanvasRect";
static const char *CanvasZoomKey    = "/CanvasZoom";

static const char *NodePosGroup     = "/GraphNodePos";

static const char *ColorsGroup      = "/GraphColors";

static const char *NodeAliasesGroup = "/GraphNodeAliases";
static const char *PortAliasesGroup = "/GraphPortAliases";


//----------------------------------------------------------------------------
// qpwgraph_canvas -- Canvas graphics scene/view.

// Constructor.
qpwgraph_canvas::qpwgraph_canvas ( QWidget *parent )
	: QGraphicsView(parent), m_state(DragNone), m_item(nullptr),
		m_connect(nullptr), m_port2(nullptr), m_rubberband(nullptr),
		m_zoom(1.0), m_zoomrange(false), m_gesture(false),
		m_commands(nullptr), m_settings(nullptr),
		m_patchbay(nullptr), m_patchbay_edit(false),
		m_patchbay_autopin(true), m_patchbay_autodisconnect(false),
		m_selected_nodes(0), m_repel_overlapping_nodes(false),
		m_rename_item(nullptr), m_rename_editor(nullptr), m_renamed(0),
		m_search_editor(nullptr), m_filter_enabled(false),
		m_merger_enabled(false)
{
	m_scene = new QGraphicsScene();

	m_commands = new QUndoStack();

	m_patchbay = new qpwgraph_patchbay(this);

	QGraphicsView::setScene(m_scene);

	QGraphicsView::setRenderHint(QPainter::Antialiasing);
	QGraphicsView::setRenderHint(QPainter::SmoothPixmapTransform);

	QGraphicsView::setResizeAnchor(QGraphicsView::NoAnchor);
	QGraphicsView::setDragMode(QGraphicsView::NoDrag);

	QPalette pal = QGraphicsView::palette();
	const QPalette::ColorRole role = QPalette::Window;
	const QColor& color = pal.color(role);
	pal.setColor(role, color.darker(120));
	QGraphicsView::setPalette(pal);
	QGraphicsView::setBackgroundRole(role);

	m_rename_editor = new QLineEdit(this);
	m_rename_editor->setFrame(false);
	m_rename_editor->setEnabled(false);
	m_rename_editor->hide();

	QObject::connect(m_rename_editor,
		SIGNAL(textChanged(const QString&)),
		SLOT(renameTextChanged(const QString&)));
	QObject::connect(m_rename_editor,
		SIGNAL(editingFinished()),
		SLOT(renameEditingFinished()));

	QGraphicsView::grabGesture(Qt::PinchGesture);

	m_search_editor = new QLineEdit(this);
	m_search_editor->setClearButtonEnabled(true);
	m_search_editor->setEnabled(false);
	m_search_editor->hide();

	QObject::connect(m_search_editor,
		SIGNAL(textChanged(const QString&)),
		SLOT(searchTextChanged(const QString&)));
	QObject::connect(m_search_editor,
		SIGNAL(editingFinished()),
		SLOT(searchEditingFinished()));
}


// Destructor.
qpwgraph_canvas::~qpwgraph_canvas (void)
{
	clear();

	delete m_search_editor;
	delete m_rename_editor;

	delete m_patchbay;
	delete m_commands;
	delete m_scene;
}


// Accessors.
QGraphicsScene *qpwgraph_canvas::scene (void) const
{
	return m_scene;
}


QUndoStack *qpwgraph_canvas::commands (void) const
{
	return m_commands;
}


void qpwgraph_canvas::setSettings ( QSettings *settings )
{
	m_settings = settings;
}


QSettings *qpwgraph_canvas::settings (void) const
{
	return m_settings;
}


qpwgraph_patchbay *qpwgraph_canvas::patchbay (void) const
{
	return m_patchbay;
}


// Patchbay auto-pin accessors.
void qpwgraph_canvas::setPatchbayAutoPin ( bool on )
{
	m_patchbay_autopin = on;
}


bool qpwgraph_canvas::isPatchbayAutoPin (void) const
{
	return m_patchbay_autopin;
}


// Patchbay auto-disconnect accessors.
void qpwgraph_canvas::setPatchbayAutoDisconnect ( bool on )
{
	m_patchbay_autodisconnect = on;
}


bool qpwgraph_canvas::isPatchbayAutoDisconnect (void) const
{
	return m_patchbay_autodisconnect;
}


// Patchbay edit-mode accessors.
void qpwgraph_canvas::setPatchbayEdit ( bool on )
{
	if (m_patchbay == nullptr)
		return;

	if ((!on && !m_patchbay_edit) ||
		( on &&  m_patchbay_edit))
		return;

	m_patchbay_edit = on;

	patchbayEdit();
}


bool qpwgraph_canvas::isPatchbayEdit (void) const
{
	return (m_patchbay && m_patchbay_edit);
}


void qpwgraph_canvas::patchbayEdit (void)
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qpwgraph_connect::Type) {
			qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
			if (connect) {
				connect->setDimmed(
					m_patchbay_edit && !m_patchbay->findConnect(connect));
			}
		}
	}
}


bool qpwgraph_canvas::canPatchbayPin (void) const
{
	if (m_patchbay == nullptr || !m_patchbay_edit)
		return false;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		if (item->type() == qpwgraph_connect::Type) {
			qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
			if (connect && !m_patchbay->findConnect(connect))
				return true;
		}
	}

	return false;
}


bool qpwgraph_canvas::canPatchbayUnpin (void) const
{
	if (m_patchbay == nullptr || !m_patchbay_edit)
		return false;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		if (item->type() == qpwgraph_connect::Type) {
			qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
			if (connect && m_patchbay->findConnect(connect))
				return true;
		}
	}

	return false;
}


void qpwgraph_canvas::patchbayPin (void)
{
	if (m_patchbay == nullptr || !m_patchbay_edit)
		return;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		if (item->type() == qpwgraph_connect::Type) {
			qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
			if (connect && m_patchbay->connect(connect, true))
				connect->setDimmed(false);
		}
	}
}


void qpwgraph_canvas::patchbayUnpin (void)
{
	if (m_patchbay == nullptr || !m_patchbay_edit)
		return;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		if (item->type() == qpwgraph_connect::Type) {
			qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
			if (connect && m_patchbay->connect(connect, false))
				connect->setDimmed(true);
		}
	}
}


bool qpwgraph_canvas::isPatchbayEmpty (void) const
{
	return (m_patchbay ? m_patchbay->items().isEmpty() : true);
}


// Canvas methods.
void qpwgraph_canvas::addItem ( qpwgraph_item *item )
{
	if (item->type() != qpwgraph_port::Type) // ports are already in nodes
		m_scene->addItem(item);

	if (item->type() == qpwgraph_node::Type) {
		qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
		if (node) {
			m_nodes.append(node);
			addNodeKeys(node);
			if (restoreNode(node))
				emit updated(node);
			else
				emit added(node);
		}
	}
	else
	if (item->type() == qpwgraph_port::Type) {
		qpwgraph_port *port = static_cast<qpwgraph_port *> (item);
		if (port)
			restorePort(port);
	}
	else
	if (item->type() == qpwgraph_connect::Type) {
		qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
		if (connect) {
			connect->setDimmed(m_patchbay_edit &&
				m_patchbay && !m_patchbay->findConnect(connect));
		}
	}
}


void qpwgraph_canvas::removeItem ( qpwgraph_item *item )
{
	if (item->type() == qpwgraph_node::Type) {
		qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
		if (node && saveNode(node)) {
			emit removed(node);
			node->removePorts();
			removeNodeKeys(node);
		}
		if (node)
			m_nodes.removeAll(node);
	}
	else
	if (item->type() == qpwgraph_port::Type) {
		qpwgraph_port *port = static_cast<qpwgraph_port *> (item);
		if (port)
			savePort(port);
	}

	// Do not remove items from the scene
	// as they shall be removed upon delete...
	//
	//	m_scene->removeItem(item);
}


// Current item accessor.
qpwgraph_item *qpwgraph_canvas::currentItem (void) const
{
	qpwgraph_item *item = m_item;

	if (item && item->type() == qpwgraph_connect::Type)
		item = nullptr;

	if (item == nullptr) {
		foreach (QGraphicsItem *item2, m_scene->selectedItems()) {
			if (item2->type() == qpwgraph_connect::Type)
				continue;
			item = static_cast<qpwgraph_item *> (item2);
			if (item2->type() == qpwgraph_node::Type)
				break;
		}
	}

	return item;
}


// Connection predicates.
bool qpwgraph_canvas::canConnect (void) const
{
	int nins = 0;
	int nouts = 0;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		if (item->type() == qpwgraph_node::Type) {
			qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
			if (node) {
				if (node->nodeMode() & qpwgraph_item::Input)
					++nins;
				else
			//	if (node->nodeMode() & qpwgraph_item::Output)
					++nouts;
			}
		}
		else
		if (item->type() == qpwgraph_port::Type) {
			qpwgraph_port *port = static_cast<qpwgraph_port *> (item);
			if (port) {
				if (port->isInput())
					++nins;
				else
			//	if (port->isOutput())
					++nouts;
			}
		}
		if (nins > 0 && nouts > 0)
			return true;
	}

	return false;
}


bool qpwgraph_canvas::canDisconnect (void) const
{
	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		switch (item->type()) {
		case qpwgraph_connect::Type:
			return true;
		case qpwgraph_node::Type: {
			qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
			foreach (qpwgraph_port *port, node->ports()) {
				if (!port->connects().isEmpty())
					return true;
			}
			// Fall-thru...
		}
		default:
			break;
		}
	}

	return false;
}


// Edit predicates.
bool qpwgraph_canvas::canRenameItem (void) const
{
	qpwgraph_item *item = currentItem();

	return (item && (
		item->type() == qpwgraph_node::Type ||
		item->type() == qpwgraph_port::Type));
}


bool qpwgraph_canvas::canSearchItem (void) const
{
	return !m_nodes.isEmpty();
}


// Zooming methods.
void qpwgraph_canvas::setZoom ( qreal zoom )
{
	if (zoom < 0.1)
		zoom = 0.1;
	else
	if (zoom > 1.9)
		zoom = 1.9;

	const qreal scale = zoom / m_zoom;
	QGraphicsView::scale(scale, scale);

	QFont font = m_rename_editor->font();
	font.setPointSizeF(scale * font.pointSizeF());
	m_rename_editor->setFont(font);
	updateRenameEditor();

	m_zoom = zoom;

	emit changed();
}


qreal qpwgraph_canvas::zoom (void) const
{
	return m_zoom;
}


void qpwgraph_canvas::setZoomRange ( bool zoomrange )
{
	m_zoomrange = zoomrange;
}


bool qpwgraph_canvas::isZoomRange (void) const
{
	return m_zoomrange;
}


// Clean-up all un-marked nodes...
void qpwgraph_canvas::resetNodes ( uint node_type )
{
	QList<qpwgraph_node *> nodes;

	foreach (qpwgraph_node *node, m_nodes) {
		if (node->nodeType() == node_type) {
			if (node->isMarked()) {
				node->resetPorts();
				node->setMarked(false);
			} else {
				nodes.append(node);
			}
		}
	}

	foreach (qpwgraph_node *node, nodes)
		removeItem(node);

	qDeleteAll(nodes);
}


void qpwgraph_canvas::clearNodes ( uint node_type )
{
	QList<qpwgraph_node *> nodes;

	foreach (qpwgraph_node *node, m_nodes) {
		if (node->nodeType() == node_type)
			nodes.append(node);
	}

	foreach (qpwgraph_node *node, nodes) {
		removeNodeKeys(node);
		m_nodes.removeAll(node);
	}

	qDeleteAll(nodes);
}


// Special node finders.
qpwgraph_node *qpwgraph_canvas::findNode (
	const qpwgraph_node::NodeIdKey& node_key ) const
{
	return m_node_ids.value(node_key, nullptr);
}

qpwgraph_node *qpwgraph_canvas::findNode (
	uint id, qpwgraph_item::Mode mode, uint type ) const
{
	return findNode(qpwgraph_node::NodeIdKey(id, mode, type));
}


QList<qpwgraph_node *> qpwgraph_canvas::findNodes (
	const qpwgraph_node::NodeNameKey& name_key ) const
{
	struct CompareNodeId {
		bool operator()(qpwgraph_node *node1, qpwgraph_node *node2) const
			{ return (node1->nodeId() < node2->nodeId()); }
	};

	QList<qpwgraph_node *> nodes = m_node_names.values(name_key);
	std::sort(nodes.begin(), nodes.end(), CompareNodeId());
	return nodes;
}


QList<qpwgraph_node *> qpwgraph_canvas::findNodes (
	const QString& name, qpwgraph_item::Mode mode, uint type ) const
{
	return findNodes(qpwgraph_node::NodeNameKey(name, mode, type));
}


// Whether it's in the middle of something...
bool qpwgraph_canvas::isBusy (void) const
{
	return (m_state != DragNone || m_connect != nullptr
		||  m_item  != nullptr  || m_rename_item != nullptr);
}


void qpwgraph_canvas::releaseNode ( qpwgraph_node *node )
{
	removeNodeKeys(node);

	node->setMarked(false);
}


// Port (dis)connections dispatcher.
void qpwgraph_canvas::emitConnectPorts (
	qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect )
{
	if (m_patchbay_edit && !m_patchbay_autopin && is_connect) {
		qpwgraph_connect *connect = port1->findConnect(port2);
		if (connect)
			connect->setDimmed(true);
	}

	if (m_patchbay && (m_patchbay_autopin
		|| (!is_connect && m_patchbay->isActivated())))
		m_patchbay->connectPorts(port1, port2, is_connect);

	if (is_connect)
		emitConnected(port1, port2);
	else
		emitDisconnected(port1, port2);
}


// Port (dis)connections notifiers.
void qpwgraph_canvas::emitConnected (
	qpwgraph_port *port1, qpwgraph_port *port2 )
{
	emit connected(port1, port2);
}


void qpwgraph_canvas::emitDisconnected (
	qpwgraph_port *port1, qpwgraph_port *port2 )
{
	emit disconnected(port1, port2);
}


// Rename notifier.
void qpwgraph_canvas::emitRenamed ( qpwgraph_item *item, const QString& name )
{
	emit renamed(item, name);
}


// Other generic notifier.
void qpwgraph_canvas::emitChanged (void)
{
	emit changed();
}


// Item finder (internal).
qpwgraph_item *qpwgraph_canvas::itemAt ( const QPointF& pos ) const
{
	const QList<QGraphicsItem *>& items
		= m_scene->items(QRectF(pos - QPointF(2, 2), QSizeF(5, 5)));

	foreach (QGraphicsItem *item, items) {
		if (item->type() >= QGraphicsItem::UserType)
			return static_cast<qpwgraph_item *> (item);
	}

	return nullptr;
}


// Port (dis)connection command.
void qpwgraph_canvas::connectPorts (
	qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect )
{
#if 0 // Sure the sect will check to this instead...?
	const bool is_connected // already connected?
		= (port1->findConnect(port2) != nullptr);
	if (( is_connect &&  is_connected) ||
		(!is_connect && !is_connected))
		return;
#endif
	if (port1->isOutput()) {
		m_commands->push(
			new qpwgraph_connect_command(this, port1, port2, is_connect));
	} else {
		m_commands->push(
			new qpwgraph_connect_command(this, port2, port1, is_connect));
	}
}


// Mouse event handlers.
void qpwgraph_canvas::mousePressEvent ( QMouseEvent *event )
{
	if (m_gesture)
		return;

	m_state = DragNone;
	m_item = nullptr;
	m_pos = QGraphicsView::mapToScene(event->pos());

	if (event->button() == Qt::LeftButton) {
		m_item = itemAt(m_pos);
		m_state = DragStart;
	}

	if (m_item == nullptr
		&& (((event->button() == Qt::LeftButton)
		  && (event->modifiers() & Qt::ControlModifier))
		  || (event->button() == Qt::MiddleButton))) {
	#if 1//NEW_DRAG_SCROLL_MODE
		// HACK: When about to drag-scroll,
		// always fake a left-button press...
		QGraphicsView::setDragMode(ScrollHandDrag);
		QMouseEvent event2(event->type(),
		#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
			event->position(), event->globalPosition(),
		#else
			event->localPos(), event->globalPos(),
		#endif
			Qt::LeftButton, Qt::LeftButton,
			event->modifiers() | Qt::ControlModifier);
		QGraphicsView::mousePressEvent(&event2);
	#else
		QGraphicsView::setCursor(Qt::ClosedHandCursor)
	#endif
		m_state = DragScroll;
	}
}


void qpwgraph_canvas::mouseMoveEvent ( QMouseEvent *event )
{
	if (m_gesture)
		return;

	int nchanged = 0;

	QPointF pos = QGraphicsView::mapToScene(event->pos());

	switch (m_state) {
	case DragStart:
		if ((pos - m_pos).manhattanLength() > 8.0) {
			m_state = DragMove;
			if (m_item) {
				// Start new connection line...
				if (m_item->type() == qpwgraph_port::Type) {
					qpwgraph_port *port = static_cast<qpwgraph_port *> (m_item);
					if (port) {
						QGraphicsView::setCursor(Qt::DragLinkCursor);
						m_selected_nodes = 0;
						m_scene->clearSelection();
						m_connect = new qpwgraph_connect();
						m_connect->setPort1(port);
						m_connect->setSelected(true);
						m_connect->raise();
						m_scene->addItem(m_connect);
						m_item = nullptr;
						++m_selected_nodes;
						++nchanged;
					}
				}
				else
				// Start moving nodes around...
				if (m_item->type() == qpwgraph_node::Type) {
					QGraphicsView::setCursor(Qt::SizeAllCursor);
					if (!m_item->isSelected()) {
						if ((event->modifiers()
							 & (Qt::ShiftModifier | Qt::ControlModifier)) == 0) {
							m_selected_nodes = 0;
							m_scene->clearSelection();
						}
						m_item->setSelected(true);
						++nchanged;
					}
					// Original node position (for move command)...
					m_pos1 = snapPos(m_pos);
				}
				else m_item = nullptr;
			}
			// Otherwise start lasso rubber-banding...
			if (m_rubberband == nullptr && m_item == nullptr && m_connect == nullptr) {
				QGraphicsView::setCursor(Qt::CrossCursor);
				m_rubberband = new QRubberBand(QRubberBand::Rectangle, this);
			}
			// Set allowed auto-scroll margins/limits...
			boundingRect(true);
		}
		break;
	case DragMove:
		// Allow auto-scroll only if within allowed margins/limits...
		boundingPos(pos);
		QGraphicsView::ensureVisible(QRectF(pos, QSizeF(2, 2)), 8, 8);
		// Move new connection line...
		if (m_connect)
			m_connect->updatePathTo(pos);
		// Move rubber-band lasso...
		if (m_rubberband) {
			const QRect rect(
				QGraphicsView::mapFromScene(m_pos),
				QGraphicsView::mapFromScene(pos));
			m_rubberband->setGeometry(rect.normalized());
			m_rubberband->show();
			if (!m_zoomrange) {
				if (event->modifiers()
					& (Qt::ControlModifier | Qt::ShiftModifier)) {
					foreach (QGraphicsItem *item, m_selected) {
						item->setSelected(!item->isSelected());
						++nchanged;
					}
					m_selected.clear();
				} else {
					m_selected_nodes = 0;
					m_scene->clearSelection();
					++nchanged;
				}
				const QRectF range_rect(m_pos, pos);
				foreach (QGraphicsItem *item,
						m_scene->items(range_rect.normalized())) {
					if (item->type() >= QGraphicsItem::UserType) {
						if (item->type() != qpwgraph_node::Type)
							++m_selected_nodes;
						else
						if (m_selected_nodes > 0)
							continue;
						const bool is_selected = item->isSelected();
						if (event->modifiers() & Qt::ControlModifier) {
							m_selected.append(item);
							item->setSelected(!is_selected);
						}
						else
						if (!is_selected) {
							if (event->modifiers() & Qt::ShiftModifier)
								m_selected.append(item);
							item->setSelected(true);
						}
						++nchanged;
					}
				}
			}
		}
		// Move current selected nodes...
		if (m_item && m_item->type() == qpwgraph_node::Type) {
			pos = snapPos(pos);
			const QPointF delta = (pos - m_pos);
			foreach (QGraphicsItem *item, m_scene->selectedItems()) {
				if (item->type() == qpwgraph_node::Type) {
					qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
					if (node)
						node->setPos(snapPos(node->pos() + delta));
				}
			}
			m_pos = pos;
		}
		else
		if (m_connect) {
			// Hovering ports high-lighting...
			const qreal zval = m_connect->zValue();
			m_connect->setZValue(-1.0);
			if (m_port2) {
				m_port2->setHighlight(false);
				m_port2 = nullptr;
			}
			QGraphicsItem *item = itemAt(pos);
			if (item && item->type() == qpwgraph_port::Type) {
				qpwgraph_port *port1 = m_connect->port1();
				qpwgraph_port *port2 = static_cast<qpwgraph_port *> (item);
				if (port1 && port2 && !port2->isHighlight() &&
					port1->portType() == port2->portType() &&
					port1->portMode() != port2->portMode()) {
					m_port2 = port2;
					m_port2->setHighlight(true);
				}
			}
			m_connect->setZValue(zval);
		}
		break;
	case DragScroll: {
	#if 1//NEW_DRAG_SCROLL_MODE
		QGraphicsView::mouseMoveEvent(event);
	#else
		QScrollBar *hbar = QGraphicsView::horizontalScrollBar();
		QScrollBar *vbar = QGraphicsView::verticalScrollBar();
		const QPoint delta = (pos - m_pos).toPoint();
		hbar->setValue(hbar->value() - delta.x());
		vbar->setValue(vbar->value() - delta.y());
		m_pos = pos;
	#endif
		break;
	}
	default:
		break;
	}

	if (nchanged > 0)
		emit changed();
}


void qpwgraph_canvas::mouseReleaseEvent ( QMouseEvent *event )
{
	if (m_gesture)
		return;

	int nchanged = 0;

	switch (m_state) {
	case DragStart:
		// Make individual item (de)selections...
		if ((event->modifiers()
			& (Qt::ShiftModifier | Qt::ControlModifier)) == 0) {
			m_selected_nodes = 0;
			m_scene->clearSelection();
			++nchanged;
		}
		if (m_item) {
			bool is_selected = true;
			if (event->modifiers() & Qt::ControlModifier)
				is_selected = !m_item->isSelected();
			m_item->setSelected(is_selected);
			if (m_item->type() != qpwgraph_node::Type && is_selected)
				++m_selected_nodes;
			m_item = nullptr; // Not needed anymore!
			++nchanged;
		}
		// Fall thru...
	case DragMove:
		// Close new connection line...
		if (m_connect) {
			m_connect->setZValue(-1.0);
			const QPointF& pos
				= QGraphicsView::mapToScene(event->pos());
			qpwgraph_item *item = itemAt(pos);
			if (item && item->type() == qpwgraph_port::Type) {
				qpwgraph_port *port1 = m_connect->port1();
				qpwgraph_port *port2 = static_cast<qpwgraph_port *> (item);
				if (port1 && port2
				//	&& port1->portNode() != port2->portNode()
					&& port1->portMode() != port2->portMode()
					&& port1->portType() == port2->portType()
					&& port1->findConnect(port2) == nullptr) {
					port2->setSelected(true);
				#if 1 // Sure the sect will commit to this instead...?
					m_connect->setPort2(port2);
					m_connect->updatePortTypeColors();
					m_connect->updatePathTo(port2->portPos());
					emit connected(m_connect);
					m_connect = nullptr;
					++m_selected_nodes;
				#else
				//	m_selected_nodes = 0;
				//	m_scene->clearSelection();
				#endif
					// Submit command; notify eventual observers...
					m_commands->beginMacro(tr("Connect"));
					connectPorts(port1, port2, true);
					m_commands->endMacro();
					++nchanged;
				}
			}
			// Done with the hovering connection...
			if (m_port2) {
				m_port2->setHighlight(false);
				m_port2 = nullptr;
			}
			if (m_connect) {
				m_connect->disconnect();
				delete m_connect;
				m_connect = nullptr;
			}
		}
		// Maybe some node(s) were moved...
		if (m_item && m_item->type() == qpwgraph_node::Type) {
			const QPointF& pos
				= QGraphicsView::mapToScene(event->pos());
			QList<qpwgraph_node *> nodes;
			foreach (QGraphicsItem *item, m_scene->selectedItems()) {
				if (item->type() == qpwgraph_node::Type) {
					qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
					if (node)
						nodes.append(node);
				}
			}
			m_commands->push(
				new qpwgraph_move_command(this, nodes, m_pos1, m_pos));
			++nchanged;
		}
		// Close rubber-band lasso...
		if (m_rubberband) {
			delete m_rubberband;
			m_rubberband = nullptr;
			m_selected.clear();
			// Zooming in range?...
			if (m_zoomrange) {
				const QRectF range_rect(m_pos,
					QGraphicsView::mapToScene(event->pos()));
				zoomFitRange(range_rect);
				nchanged = 0;
			}
		}
		break;
	case DragScroll:
	default:
		break;
	}

#if 1//NEW_DRAG_SCROLL_MODE
	if (QGraphicsView::dragMode() == ScrollHandDrag) {
		QGraphicsView::mouseReleaseEvent(event);
		QGraphicsView::setDragMode(NoDrag);
	}
#endif
	m_state = DragNone;
	m_item = nullptr;

	// Reset cursor...
	QGraphicsView::setCursor(Qt::ArrowCursor);

	if (nchanged > 0)
		emit changed();
}


void qpwgraph_canvas::mouseDoubleClickEvent ( QMouseEvent *event )
{
	m_pos  = QGraphicsView::mapToScene(event->pos());
	m_item = itemAt(m_pos);

	if (m_item && canRenameItem()) {
		renameItem();
	} else {
		QGraphicsView::centerOn(m_pos);
	}
}


void qpwgraph_canvas::wheelEvent ( QWheelEvent *event )
{
	if (event->modifiers() & Qt::ControlModifier) {
		const int delta
		#if QT_VERSION < 0x050000
			= event->delta();
		#else
			= event->angleDelta().y();
		#endif
		setZoom(zoom() + qreal(delta) / 1200.0);
	}
	else QGraphicsView::wheelEvent(event);
}


// Keyboard event handler.
void qpwgraph_canvas::keyPressEvent ( QKeyEvent *event )
{
	if (event->key() == Qt::Key_Escape) {
		m_scene->clearSelection();
		m_search_editor->editingFinished();
		clear();
		emit changed();
	}
	else
	if (!m_search_editor->isEnabled()
		&& (event->modifiers() & Qt::ControlModifier) == 0
		&& !event->text().trimmed().isEmpty()) {
		startSearchEditor(event->text());
	}
}


// Connect selected items.
void qpwgraph_canvas::connectItems (void)
{
	QList<qpwgraph_port *> outs;
	QList<qpwgraph_port *> ins;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		if (item->type() == qpwgraph_port::Type) {
			qpwgraph_port *port = static_cast<qpwgraph_port *> (item);
			if (port) {
				if (port->isOutput())
					outs.append(port);
				else
					ins.append(port);
			}
		}
	}

	if (outs.isEmpty() || ins.isEmpty())
		return;

//	m_selected_nodes = 0;
//	m_scene->clearSelection();

	std::sort(outs.begin(), outs.end(), qpwgraph_port::ComparePos());
	std::sort(ins.begin(),  ins.end(),  qpwgraph_port::ComparePos());

	QListIterator<qpwgraph_port *> iter1(outs);
	QListIterator<qpwgraph_port *> iter2(ins);

	m_commands->beginMacro(tr("Connect"));

	const int nports = qMax(outs.count(), ins.count());
	for (int n = 0; n < nports; ++n) {
		// Wrap a'round...
		if (!iter1.hasNext())
			iter1.toFront();
		if (!iter2.hasNext())
			iter2.toFront();
		// Submit command; notify eventual observers...
		qpwgraph_port *port1 = iter1.next();
		qpwgraph_port *port2 = iter2.next();
		// Skip over non-matching port-types...
		bool wrapped = false;
		while (port1 && port2 && port1->portType() != port2->portType()) {
			if (!iter2.hasNext()) {
				if (wrapped)
					break;
				iter2.toFront();
				wrapped = true;
			}
			port2 = iter2.next();
		}
		// Submit command; notify eventual observers...
		if (!wrapped && port1 && port2 && port1->portNode() != port2->portNode())
			connectPorts(port1, port2, true);
	}

	m_commands->endMacro();
}


// Disconnect selected items.
void qpwgraph_canvas::disconnectItems (void)
{
	QList<qpwgraph_connect *> connects;
	QList<qpwgraph_node *> nodes;

	foreach (QGraphicsItem *item, m_scene->selectedItems()) {
		switch (item->type()) {
		case qpwgraph_connect::Type: {
			qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
			if (!connects.contains(connect))
				connects.append(connect);
			break;
		}
		case qpwgraph_node::Type:
			nodes.append(static_cast<qpwgraph_node *> (item));
			// Fall thru...
		default:
			break;
		}
	}

	if (connects.isEmpty()) {
		foreach (qpwgraph_node *node, nodes) {
			foreach (qpwgraph_port *port, node->ports()) {
				foreach (qpwgraph_connect *connect, port->connects()) {
					if (!connects.contains(connect))
						connects.append(connect);
				}
			}
		}
	}

	if (connects.isEmpty())
		return;

//	m_selected_nodes = 0;
//	m_scene->clearSelection();

	m_item = nullptr;

	m_commands->beginMacro(tr("Disconnect"));

	foreach (qpwgraph_connect *connect, connects) {
		// Submit command; notify eventual observers...
		qpwgraph_port *port1 = connect->port1();
		qpwgraph_port *port2 = connect->port2();
		if (port1 && port2)
			connectPorts(port1, port2, false);
	}

	m_commands->endMacro();
}


// Select actions.
void qpwgraph_canvas::selectAll (void)
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qpwgraph_node::Type)
			item->setSelected(true);
		else
			++m_selected_nodes;
	}

	emit changed();
}


void qpwgraph_canvas::selectNone (void)
{
	m_selected_nodes = 0;
	m_scene->clearSelection();

	emit changed();
}


void qpwgraph_canvas::selectInvert (void)
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qpwgraph_node::Type)
			item->setSelected(!item->isSelected());
		else
			++m_selected_nodes;
	}

	emit changed();
}


// Edit actions.
void qpwgraph_canvas::renameItem (void)
{
	qpwgraph_item *item = currentItem();

	if (item && item->type() == qpwgraph_node::Type) {
		qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
		if (node) {
			QPalette pal;
			const QColor& foreground
				= node->foreground();
			QColor background = node->background();
			const bool is_dark
				= (background.value() < 192);
			pal.setColor(QPalette::Text, is_dark
				? foreground.lighter()
				: foreground.darker());
			background.setAlpha(255);
			pal.setColor(QPalette::Base, background);
			m_rename_editor->setPalette(pal);
			QFont font = m_rename_editor->font();
			font.setBold(true);
			m_rename_editor->setFont(font);
			m_rename_editor->setPlaceholderText(node->nodeName());
			m_rename_editor->setText(node->nodeTitle());
		}
	}
	else
	if (item && item->type() == qpwgraph_port::Type) {
		qpwgraph_port *port = static_cast<qpwgraph_port *> (item);
		if (port) {
			QPalette pal;
			const QColor& foreground
				= port->foreground();
			const QColor& background
				= port->background();
			const bool is_dark
				= (background.value() < 128);
			pal.setColor(QPalette::Text, is_dark
				? foreground.lighter()
				: foreground.darker());
			pal.setColor(QPalette::Base, background.lighter());
			m_rename_editor->setPalette(pal);
			QFont font = m_rename_editor->font();
			font.setBold(false);
			m_rename_editor->setFont(font);
			m_rename_editor->setPlaceholderText(port->portName());
			m_rename_editor->setText(port->portTitle());
		}
	}
	else return;

	m_selected_nodes = 0;
	m_scene->clearSelection();

	m_rename_editor->show();
	m_rename_editor->setEnabled(true);
	m_rename_editor->selectAll();
	m_rename_editor->setFocus();
	m_renamed = 0;

	m_rename_item = item;

	updateRenameEditor();
}


void qpwgraph_canvas::searchItem (void)
{
	if (!m_search_editor->isEnabled())
		startSearchEditor();
}


// Update editors position and size.
void qpwgraph_canvas::updateRenameEditor (void)
{
	if (m_rename_item
		&& m_rename_editor->isEnabled()
		&& m_rename_editor->isVisible()) {
		const QRectF& rect
			= m_rename_item->editorRect().adjusted(+2.0, +2.0, -2.0, -2.0);
		const QPoint& pos1
			= QGraphicsView::mapFromScene(rect.topLeft());
		const QPoint& pos2
			= QGraphicsView::mapFromScene(rect.bottomRight());
		m_rename_editor->setGeometry(
			pos1.x(),  pos1.y(),
			pos2.x() - pos1.x(),
			pos2.y() - pos1.y());
	}
}


void qpwgraph_canvas::updateSearchEditor (void)
{
	// Position the search editor to the bottom-right of the canvas...
	const QSize& size = QGraphicsView::viewport()->size();
	const QSize& hint = m_search_editor->sizeHint();
	const int w = hint.width() * 2;
	const int h = hint.height();
	m_search_editor->setGeometry(size.width() - w, size.height() - h, w, h);
}


// Discrete zooming actions.
void qpwgraph_canvas::zoomIn (void)
{
	setZoom(zoom() + 0.1);
}


void qpwgraph_canvas::zoomOut (void)
{
	setZoom(zoom() - 0.1);
}


void qpwgraph_canvas::zoomFit (void)
{
	zoomFitRange(m_scene->itemsBoundingRect());
}


void qpwgraph_canvas::zoomReset (void)
{
	setZoom(1.0);
}


// Update all nodes.
void qpwgraph_canvas::updateNodes (void)
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qpwgraph_node::Type) {
			qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
			if (node)
				node->updatePath();
		}
	}
}


// Update all connectors.
void qpwgraph_canvas::updateConnects (void)
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qpwgraph_connect::Type) {
			qpwgraph_connect *connect = static_cast<qpwgraph_connect *> (item);
			if (connect)
				connect->updatePath();
		}
	}
}


// Zoom in rectangle range.
void qpwgraph_canvas::zoomFitRange ( const QRectF& range_rect )
{
	QGraphicsView::fitInView(
		range_rect, Qt::KeepAspectRatio);

	const QTransform& transform
		= QGraphicsView::transform();
	if (transform.isScaling()) {
		qreal zoom = transform.m11();
		if (zoom < 0.1) {
			const qreal scale = 0.1 / zoom;
			QGraphicsView::scale(scale, scale);
			zoom = 0.1;
		}
		else
		if (zoom > 2.0) {
			const qreal scale = 2.0 / zoom;
			QGraphicsView::scale(scale, scale);
			zoom = 2.0;
		}
		m_zoom = zoom;
	}

	emit changed();
}


// Graph node/port state methods.
bool qpwgraph_canvas::restoreNode ( qpwgraph_node *node )
{
	if (m_settings == nullptr || node == nullptr)
		return false;

	// Assume node name-keys have been added before this...
	//
	const int n = nodeNum(node);
	const QString& node_key = nodeKey(node, n);

	m_settings->beginGroup(NodeAliasesGroup);
	const QString& node_title
		= m_settings->value('/' + node_key).toString();
	m_settings->endGroup();

	if (!node_title.isEmpty())
		node->setNodeTitle(node_title);

	m_settings->beginGroup(NodePosGroup);
	QPointF node_pos = m_settings->value('/' + node_key).toPointF();
	m_settings->endGroup();

	if (node_pos.isNull())
		return false;

	boundingPos(node_pos);

	node->setPos(node_pos);
	return true;
}


bool qpwgraph_canvas::saveNode ( qpwgraph_node *node ) const
{
	if (m_settings == nullptr || node == nullptr)
		return false;

	// Sure node keys hasn't been released before?...
	if (findNode(qpwgraph_node::NodeIdKey(node)) != node)
		return false;

	// Assume node name-keys are to be removed after this...
	//
	const int n = nodeNum(node);
	if (n < 1)
		return true;

	const QString& node_key = nodeKey(node, n);

	m_settings->beginGroup(NodeAliasesGroup);
	if (node->nodeNameLabel() != node->nodeTitle()) {
		m_settings->setValue('/' + node_key, node->nodeTitle());
	} else {
		m_settings->remove('/' + node_key);
	}
	m_settings->endGroup();

	m_settings->beginGroup(NodePosGroup);
	m_settings->setValue('/' + node_key, node->pos());
	m_settings->endGroup();

	return true;
}


bool qpwgraph_canvas::restorePort ( qpwgraph_port *port )
{
	if (m_settings == nullptr || port == nullptr)
		return false;

	const QString& port_key = portKey(port);

	m_settings->beginGroup(PortAliasesGroup);
	const QString& port_title
		= m_settings->value('/' + port_key).toString();
	m_settings->endGroup();

	if (port_title.isEmpty())
		return false;

	port->setPortTitle(port_title);

	qpwgraph_node *node = port->portNode();
	if (node)
		node->updatePath();

	return true;
}


bool qpwgraph_canvas::savePort ( qpwgraph_port *port ) const
{
	if (m_settings == nullptr || port == nullptr)
		return false;

	const QString& port_key = portKey(port);

	m_settings->beginGroup(PortAliasesGroup);
	if (port->portNameLabel() != port->portTitle())
		m_settings->setValue('/' + port_key, port->portTitle());
	else
		m_settings->remove('/' + port_key);
	m_settings->endGroup();

	return true;
}


bool qpwgraph_canvas::restoreState (void)
{
	if (m_settings == nullptr)
		return false;

#ifdef CONFIG_CLEANUP_NODE_NAMES
	cleanupNodeNames(NodePosGroup);
	cleanupNodeNames(NodeAliasesGroup);
#endif
	m_settings->beginGroup(ColorsGroup);
	const QRegularExpression rx("^0x");
	QStringListIterator key(m_settings->childKeys());
	while (key.hasNext()) {
		const QString& sKey = key.next();
		const QColor& color = QString(m_settings->value(sKey).toString());
		if (color.isValid()) {
			QString sx(sKey);
			bool ok = false;
			const uint port_type = sx.remove(rx).toUInt(&ok, 16);
			if (ok) m_port_colors.insert(port_type, color);
		}
	}
	m_settings->endGroup();

	m_settings->beginGroup(CanvasGroup);
	const QRectF& rect = m_settings->value(CanvasRectKey).toRectF();
	const qreal zoom = m_settings->value(CanvasZoomKey, 1.0).toReal();
	m_settings->endGroup();

	if (rect.isValid())
		m_rect1 = rect; // QGraphicsView::setSceneRect(rect);

	setZoom(zoom);

	return true;
}


bool qpwgraph_canvas::saveState (void) const
{
	if (m_settings == nullptr)
		return false;

	QList<qpwgraph_node *> nodes;

	const QList<QGraphicsItem *> items(m_scene->items());
	foreach (QGraphicsItem *item, items) {
		if (item->type() == qpwgraph_node::Type) {
			qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
			if (node && !nodes.contains(node)) {
				int n = 0;
				const QList<qpwgraph_node *>& nodes2
					= findNodes(qpwgraph_node::NodeNameKey(node));
				foreach (qpwgraph_node *node2, nodes2) {
					const QString& node2_key = nodeKey(node2, ++n);
					m_settings->beginGroup(NodePosGroup);
					m_settings->setValue('/' + node2_key, node2->pos());
					m_settings->endGroup();
					m_settings->beginGroup(NodeAliasesGroup);
					if (node2->nodeNameLabel() != node2->nodeTitle())
						m_settings->setValue('/' + node2_key, node2->nodeTitle());
					else
						m_settings->remove('/' + node2_key);
					m_settings->endGroup();
					nodes.append(node);
				}
			}
		}
		else
		if (item->type() == qpwgraph_port::Type) {
			qpwgraph_port *port = static_cast<qpwgraph_port *> (item);
			if (port) {
				const QString& port_key = portKey(port);
				m_settings->beginGroup(PortAliasesGroup);
				if (port && port->portNameLabel() != port->portTitle())
					m_settings->setValue('/' + port_key, port->portTitle());
				else
					m_settings->remove('/' + port_key);
				m_settings->endGroup();
			}
		}
	}

	m_settings->beginGroup(CanvasGroup);
	m_settings->setValue(CanvasZoomKey, zoom());
	m_settings->setValue(CanvasRectKey, m_rect1);
	m_settings->endGroup();

	m_settings->beginGroup(ColorsGroup);
	QStringListIterator key(m_settings->childKeys());
	while (key.hasNext()) m_settings->remove(key.next());
	QHash<uint, QColor>::ConstIterator iter = m_port_colors.constBegin();
	const QHash<uint, QColor>::ConstIterator& iter_end = m_port_colors.constEnd();
	for ( ; iter != iter_end; ++iter) {
		const uint port_type = iter.key();
		const QColor& color = iter.value();
		m_settings->setValue("0x" + QString::number(port_type, 16), color.name());
	}
	m_settings->endGroup();

	return true;
}


// Node number if multiple (else count).
int qpwgraph_canvas::nodeNum ( qpwgraph_node *node ) const
{
	int n = 0;

	const QList<qpwgraph_node *>& nodes
		= findNodes(qpwgraph_node::NodeNameKey(node));
	const QList<qpwgraph_node *>::ConstIterator& iter_end = nodes.constEnd();
	QList<qpwgraph_node *>::ConstIterator iter = nodes.constBegin();
	while (iter != iter_end) {
		++n;
		if ((*iter)->nodeId() == node->nodeId())
			break;
		++iter;
	}

	return n;
}


// Graph node/port key helpers.
QString qpwgraph_canvas::nodeKey ( qpwgraph_node *node, int n ) const
{
	QString node_key = node->nodeName();
	if (n > 1) {
		node_key += '_';
		node_key += QString::number(n - 1);
	}

	switch (node->nodeMode()) {
	case qpwgraph_item::Input:
		node_key += ":Input";
		break;
	case qpwgraph_item::Output:
		node_key += ":Output";
		break;
	default:
		break;
	}

	return node_key;
}


QString qpwgraph_canvas::portKey ( qpwgraph_port *port ) const
{
	QString port_key;

	qpwgraph_node *node = port->portNode();
	if (node == nullptr)
		return port_key;

	port_key += node->nodeName();
	port_key += ':';
	port_key += port->portName();

	switch (port->portMode()) {
	case qpwgraph_item::Input:
		port_key += ":Input";
		break;
	case qpwgraph_item::Output:
		port_key += ":Output";
		break;
	default:
		break;
	}

	return port_key;
}


void qpwgraph_canvas::addNodeKeys ( qpwgraph_node *node )
{
	m_node_ids.insert(qpwgraph_node::NodeIdKey(node), node);
	m_node_names.insert(qpwgraph_node::NodeNameKey(node), node);
}


void qpwgraph_canvas::removeNodeKeys ( qpwgraph_node *node )
{
	m_node_names.remove(qpwgraph_node::NodeNameKey(node), node);
	m_node_ids.remove(qpwgraph_node::NodeIdKey(node), node);
}


// Graph port colors management.
void qpwgraph_canvas::setPortTypeColor (
	uint port_type, const QColor& port_color )
{
	m_port_colors.insert(port_type, port_color);
}


const QColor& qpwgraph_canvas::portTypeColor ( uint port_type )
{
	return m_port_colors[port_type];
}


void qpwgraph_canvas::updatePortTypeColors ( uint port_type )
{
	foreach (QGraphicsItem *item, m_scene->items()) {
		if (item->type() == qpwgraph_port::Type) {
			qpwgraph_port *port = static_cast<qpwgraph_port *> (item);
			if (port && (0 >= port_type || port->portType() == port_type)) {
				port->updatePortTypeColors(this);
				port->update();
			}
		}
	}
}


void qpwgraph_canvas::clearPortTypeColors (void)
{
	m_port_colors.clear();
}


// Clear all selection.
void qpwgraph_canvas::clearSelection (void)
{
	m_item = nullptr;
	m_selected_nodes = 0;
	m_scene->clearSelection();

	m_rename_item = nullptr;
	m_rename_editor->setEnabled(false);
	m_rename_editor->hide();
	m_renamed = 0;
}


// Clear all state.
void qpwgraph_canvas::clear (void)
{
	m_selected_nodes = 0;
	if (m_rubberband) {
		delete m_rubberband;
		m_rubberband = nullptr;
		m_selected.clear();
	}
	if (m_port2) {
		m_port2->setHighlight(false);
		m_port2 = nullptr;
	}
	if (m_connect) {
		m_connect->disconnect();
		delete m_connect;
		m_connect = nullptr;
	}
	if (m_state == DragScroll)
		QGraphicsView::setDragMode(QGraphicsView::NoDrag);
	m_state = DragNone;
	m_item = nullptr;
	m_rename_item = nullptr;
	m_rename_editor->setEnabled(false);
	m_rename_editor->hide();
	m_renamed = 0;

	// Reset cursor...
	QGraphicsView::setCursor(Qt::ArrowCursor);
}


// Rename item slots.
void qpwgraph_canvas::renameTextChanged ( const QString& /* text */)
{
	if (m_rename_item
		&& m_rename_editor->isEnabled()
		&& m_rename_editor->isVisible())
		++m_renamed;
}


void qpwgraph_canvas::renameEditingFinished (void)
{
	if (m_rename_item
		&& m_rename_editor->isEnabled()
		&& m_rename_editor->isVisible()) {
		// If changed then notify...
		if (m_renamed > 0) {
			m_commands->push(
				new qpwgraph_rename_command(this,
					m_rename_item, m_rename_editor->text()));
		}
		// Reset all renaming stuff...
		m_rename_item = nullptr;
		m_rename_editor->setEnabled(false);
		m_rename_editor->hide();
		m_renamed = 0;
	}
}


// Search item slots.
void qpwgraph_canvas::searchTextChanged ( const QString& text )
{
	clearSelection();

	if (text.isEmpty())
		return;

	const QRegularExpression& rx
		= QRegularExpression(text, QRegularExpression::CaseInsensitiveOption);
	if (!rx.isValid())
		return;

	for (qpwgraph_node *node : m_nodes) {
		if (rx.match(node->nodeTitle()).hasMatch()) {
			node->setSelected(true);
			QGraphicsView::ensureVisible(node);
		}
	}

}


void qpwgraph_canvas::searchEditingFinished (void)
{
	m_search_editor->setEnabled(false);
	m_search_editor->hide();
	m_search_editor->clearFocus();

	QGraphicsView::setFocus();
}


// Start search editor...
void qpwgraph_canvas::startSearchEditor ( const QString& text )
{
	m_search_editor->setEnabled(true);
	m_search_editor->setText(text);
	m_search_editor->raise();
	m_search_editor->show();
	m_search_editor->setFocus();
}


void qpwgraph_canvas::resizeEvent( QResizeEvent *event )
{
	QGraphicsView::resizeEvent(event);

	updateSearchEditor();
}


// Repel overlapping nodes...
void qpwgraph_canvas::setRepelOverlappingNodes ( bool on )
{
	m_repel_overlapping_nodes = on;
}


bool qpwgraph_canvas::isRepelOverlappingNodes (void) const
{
	return m_repel_overlapping_nodes;
}


void qpwgraph_canvas::repelOverlappingNodes ( qpwgraph_node *node,
	qpwgraph_move_command *move_command, const QPointF& delta )
{
	const qreal MIN_NODE_GAP = 8.0f;

	node->setMarked(true);

	QRectF rect1 = node->sceneBoundingRect();
	rect1.adjust(
		-2.0 * MIN_NODE_GAP, -MIN_NODE_GAP,
		+2.0 * MIN_NODE_GAP, +MIN_NODE_GAP);

	foreach (qpwgraph_node *node2, m_nodes) {
		if (node2->isMarked())
			continue;
		const QPointF& pos1
			= node2->pos();
		QPointF pos2 = pos1;
		const QRectF& rect2
			= node2->sceneBoundingRect();
		const QRectF& recti
			= rect2.intersected(rect1);
		if (!recti.isNull()) {
			const QPointF delta2
				= (delta.isNull() ? rect2.center() - rect1.center() : delta);
			if (recti.width() < (1.5 * recti.height())) {
				qreal dx = recti.width();
				if ((delta2.x() < 0.0 && recti.width() >= rect1.width()) ||
					(delta2.x() > 0.0 && recti.width() >= rect2.width())) {
					dx += qAbs(rect2.right() - rect1.right());
				}
				else
				if ((delta2.x() > 0.0 && recti.width() >= rect1.width()) ||
					(delta2.x() < 0.0 && recti.width() >= rect2.width())) {
					dx += qAbs(rect2.left() - rect1.left());
				}
				if (delta2.x() < 0.0)
					pos2.setX(pos1.x() - dx);
				else
					pos2.setX(pos1.x() + dx);
			} else {
				qreal dy = recti.height();
				if ((delta2.y() < 0.0 && recti.height() >= rect1.height()) ||
					(delta2.y() > 0.0 && recti.height() >= rect2.height())) {
					dy += qAbs(rect2.bottom() - rect1.bottom());
				}
				else
				if ((delta2.y() > 0.0 && recti.height() >= rect1.height()) ||
					(delta2.y() < 0.0 && recti.height() >= rect2.height())) {
					dy += qAbs(rect2.top() - rect1.top());
				}
				if (delta2.y() < 0.0)
					pos2.setY(pos1.y() - dy);
				else
					pos2.setY(pos1.y() + dy);
			}
			// Repel this node...
			node2->setPos(pos2);
			// Add this node for undo/redo...
			if (move_command)
				move_command->addItem(node2, pos1, pos2);
			// Repel this node neighbors, if any...
			repelOverlappingNodes(node2, move_command, delta2);
		}
	}

	node->setMarked(false);
}


void qpwgraph_canvas::repelOverlappingNodesAll (
	qpwgraph_move_command *move_command )
{
	foreach (qpwgraph_node *node, m_nodes)
		repelOverlappingNodes(node, move_command);
}


//////////////////////////////////////////////////////////////////////////////

static qreal meanPortY(QSet<qpwgraph_port *> ports)
{
	if (ports.empty()) {
		return -std::numeric_limits<double>::infinity();
	}

	qreal y = 0;
	foreach (qpwgraph_port *p, ports) {
		y += p->pos().y() + p->portNode()->pos().y();
	}

	return y / ports.size();
}

static qreal meanParentPortY(QList<qpwgraph_node *> nodes)
{
	auto ports = QSet<qpwgraph_port *>();

	foreach (qpwgraph_node *n, nodes) {
		ports |= qpwgraph_toposort::connectedParentPorts(n);
	}

	return meanPortY(ports);
}

static qreal meanInputPortY(QList<qpwgraph_node *> nodes)
{
	auto ports = QSet<qpwgraph_port *>();

	foreach (qpwgraph_node *n, nodes) {
		ports |= qpwgraph_toposort::connectedInputPorts(n);
	}

	return meanPortY(ports);
}

// Rearrange nodes by type and connection using a topological ordering.
// Nodes with no incoming connections go first, nodes with no outgoing
// connections go last.  Nodes are arranged in columns based on their distance
// in the graph from a source node.
//
// Possible future improvements:
// - Try to keep connected nodes near each other in Y
void qpwgraph_canvas::arrangeNodes (void)
{
	if (m_nodes.empty()) {
		std::cout << "TOPO: no nodes to arrange" << std::endl;
		return;
	}

	qpwgraph_toposort topo(m_nodes);
	QList<qpwgraph_node *> sorted = topo.sort();

	QMap<int, QList<qpwgraph_node *>> rankNodes;
	QMap<int, float> rankMaxWidth;
	int maxRank = 0;

	foreach (qpwgraph_node *n, sorted) {
		std::cout << "TOPO: " << qpwgraph_toposort::debugNode(n) << std::endl;

		int rank = n->depth();
		if (!rankNodes.contains(rank)) {
			rankNodes[rank] = QList<qpwgraph_node *>();
		}
		rankNodes[rank] << n;

		if (!rankMaxWidth.contains(rank)) {
			rankMaxWidth[rank] = 0;
		}
		rankMaxWidth[rank] = qMax(rankMaxWidth[rank], n->boundingRect().width());

		maxRank = qMax(maxRank, rank);
	}

	for (int i = 0; i <= maxRank; i++) {
		if (rankMaxWidth.contains(i)) {
			std::cout << "TOPO: rank " << i << ": maxWidth=" << rankMaxWidth[i] << " count=" << rankNodes[i].size() << std::endl;
		}
	}

	// Place nodes based on topological sort
	// TODO: extract spacing values to constants or parameters
	// TODO: never go below 0,0 for xmin/ymin?
	// FIXME: items sometimes end up on very edge of scroll area
	QRectF bounds = m_scene->itemsBoundingRect();
	double xmin = bounds.left();
	double ymin = bounds.top();
	double x = xmin, y = ymin;
	int current_rank = sorted.first()->depth();
	const double ypad = 40;
	foreach (qpwgraph_node *node, sorted) {
		if (node->depth() > current_rank) {
			// End of a rank; reset Y and move to the next column
			std::cout << "TOPO: next rank: from " << current_rank << " to " << node->depth() << std::endl;
			y = ymin;
			x += rankMaxWidth[current_rank] + 120;
			current_rank = node->depth();
		}

		double w = node->boundingRect().width();
		if (current_rank == 0) {
			// Right-align sources
			node->setPos(QPointF(x + (rankMaxWidth[current_rank] - w), y));
		} else if (current_rank == maxRank) {
			// Left-align sinks
			node->setPos(QPointF(x, y));
		} else {
			// Center-align everything else
			node->setPos(QPointF(x + (rankMaxWidth[current_rank] - w) / 2, y));
		}

		y += node->boundingRect().height() + ypad;
	}

	// Move columns vertically for shorter and neater wires
	const int maxIterations = 1;
	for (int iterations = 0; iterations < maxIterations; iterations++) {
		for (int i = 1; i <= maxRank; i++) {
			// Sort each column by average connection source port Y value
			std::sort(rankNodes[i].begin(), rankNodes[i].end(), [](qpwgraph_node *n1, qpwgraph_node *n2) { return meanParentPortY({n1}) < meanParentPortY({n2}); });

			float minY = rankNodes[i].first()->pos().y();
			foreach (qpwgraph_node *n, rankNodes[i]) {
				minY = qMin(minY, n->pos().y());
			}

			float y = minY;
			foreach (qpwgraph_node *n, rankNodes[i]) {
				std::cout << "TOPO: " << qpwgraph_toposort::debugNode(n) << " parent Y is " << meanParentPortY({n}) << std::endl;
				std::cout << "TOPO: Setting " << qpwgraph_toposort::debugNode(n) << " Y from " << n->pos().y() << " to " << y << std::endl;
				n->setPos(n->pos().x(), y);
				y += n->boundingRect().height() + ypad;
			}

			// TODO: center each column's average connection input port Y value on
			// the previous column's average connection output port Y value
		}
	}

	ensureVisible(sorted.last());
	ensureVisible(sorted.first());

	m_scene->invalidate();
}


// Gesture event handlers.
//
bool qpwgraph_canvas::event ( QEvent *event )
{
	if (event->type() == QEvent::Gesture)
		return gestureEvent(static_cast<QGestureEvent *> (event));
	else
		return QGraphicsView::event(event);
}


bool qpwgraph_canvas::gestureEvent ( QGestureEvent *event )
{
	if (QGesture *pinch = event->gesture(Qt::PinchGesture))
		pinchGesture(static_cast<QPinchGesture *> (pinch));

	return true;
}


void qpwgraph_canvas::pinchGesture ( QPinchGesture *pinch )
{
	switch (pinch->state()) {
	case Qt::GestureStarted: {
		const qreal scale_factor = zoom();
		pinch->setScaleFactor(scale_factor);
		pinch->setLastScaleFactor(scale_factor);
		pinch->setTotalScaleFactor(scale_factor);
		m_gesture = true;
		break;
	}
	case Qt::GestureFinished:
		m_gesture = false;
		// Fall thru...
	case Qt::GestureUpdated:
		if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged)
			setZoom(pinch->totalScaleFactor());
		// Fall thru...
	default:
		break;
	}
}


// Bounding margins/limits...
//
const QRectF& qpwgraph_canvas::boundingRect ( bool reset )
{
	if (!m_rect1.isValid() || reset) {
		const QRect& rect = QGraphicsView::rect();
		const qreal mx = 0.33 * rect.width();
		const qreal my = 0.33 * rect.height();
		m_rect1 = m_scene->itemsBoundingRect()
			.marginsAdded(QMarginsF(mx, my, mx, my));
	}

	return m_rect1;
}


void qpwgraph_canvas::boundingPos ( QPointF& pos )
{
	const QRectF& rect = boundingRect();
	if (!rect.contains(pos)) {
		pos.setX(qBound(rect.left(), pos.x(), rect.right()));
		pos.setY(qBound(rect.top(),  pos.y(), rect.bottom()));
	}
}


// Snap into position helpers.
//
QPointF qpwgraph_canvas::snapPos ( const QPointF& pos ) const
{
	return QPointF(
		4.0 * ::round(0.25 * pos.x()),
		4.0 * ::round(0.25 * pos.y()));
}


#ifdef CONFIG_CLEANUP_NODE_NAMES

void qpwgraph_canvas::cleanupNodeNames ( const char *group )
{
	bool cleanup = false;

	m_settings->beginGroup("/CleanupNodeNames");
	cleanup = m_settings->value(group).toBool();
	if (!cleanup)
		m_settings->setValue(group, true);
	m_settings->endGroup();

	if (cleanup)
		return;

	m_settings->beginGroup(group);
	const QRegularExpression rx("\\-([0-9]+).*$");
	QHash<QString, QVariant> keys;
	QStringListIterator iter(m_settings->childKeys());
	while (iter.hasNext()) {
		const QString& key = iter.next();
		const QVariant& value = m_settings->value(key);
		QString key2 = key;
		if (cleanupNodeName(key2)) {
			int n = 0;
			if (keys.find(key2) != keys.end()) {
				const QRegularExpressionMatch mx = rx.match(key2);
				if (mx.hasMatch()) {
					n = mx.captured(1).toInt();
					key2.remove(rx);
				}
				QString key3;
				do { key3 = key2 + '-' + QString::number(++n); }
				while (keys.find(key3) != keys.end());
				key2 = key3;
			}
			if (n == 0) {
				keys.insert(key2, value);
				m_settings->setValue(key2, value);
			}
			m_settings->remove(key);
		} else {
			keys.insert(key, value);
		}
	}
	m_settings->endGroup();
}


bool qpwgraph_canvas::cleanupNodeName ( QString& name )
{
	const QRegularExpression rx("^.+( \\[.+\\])[^ ]*$");
	const QRegularExpressionMatch& mx = rx.match(name);
	if (mx.hasMatch()) {
		name.remove(mx.captured(1));
		return true;
	} else {
		return false;
	}
}

#endif//CONFIG_CLEANUP_NODE_NAMES



// Search placeholder text accessors.
void qpwgraph_canvas::setSearchPlaceholderText ( const QString& text )
{
	m_search_editor->setPlaceholderText(text);
}

QString qpwgraph_canvas::searchPlaceholderText (void) const
{
	return m_search_editor->placeholderText();
}


// Filter/hide list management accessors.
void qpwgraph_canvas::setFilterNodesEnabled ( bool enabled )
{
	m_filter_enabled = enabled;
}

bool qpwgraph_canvas::isFilterNodesEnabled (void) const
{
	return m_filter_enabled;
}


void qpwgraph_canvas::setFilterNodesList ( const QStringList& nodes )
{
	m_filter_nodes = nodes;
}


const QStringList& qpwgraph_canvas::filterNodesList (void) const
{
	return m_filter_nodes;
}


bool qpwgraph_canvas::isFilterNodes ( const QString& node_name ) const
{
	if (!m_filter_enabled)
		return false;

	QStringListIterator iter(m_filter_nodes);
	while (iter.hasNext()) {
		const QString& node_pattern = iter.next();
		const QRegularExpression& rx
			= QRegularExpression(node_pattern,
				QRegularExpression::CaseInsensitiveOption);
		if (rx.isValid()) {
			if (rx.match(node_name).hasMatch())
				return true;
		}
	}

	return false;
}


// Merger/unify list management accessors.
void qpwgraph_canvas::setMergerNodesEnabled ( bool enabled )
{
	m_merger_enabled = enabled;
}

bool qpwgraph_canvas::isMergerNodesEnabled (void) const
{
	return m_merger_enabled;
}


void qpwgraph_canvas::setMergerNodesList ( const QStringList& nodes )
{
	m_merger_nodes = nodes;
}


const QStringList& qpwgraph_canvas::mergerNodesList (void) const
{
	return m_merger_nodes;
}


bool qpwgraph_canvas::isMergerNodes ( const QString& node_name ) const
{
	if (!m_merger_enabled)
		return false;

	QStringListIterator iter(m_merger_nodes);
	while (iter.hasNext()) {
		const QString& node_pattern = iter.next();
		const QRegularExpression& rx
			= QRegularExpression(node_pattern,
				QRegularExpression::CaseInsensitiveOption);
		if (rx.isValid()) {
			if (rx.match(node_name).hasMatch())
				return true;
		}
	}

	return false;
}


// end of qpwgraph_canvas.cpp
