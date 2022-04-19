// qpwgraph_node.cpp
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

#include "qpwgraph_node.h"

#include <QGraphicsScene>

#include <QStyleOptionGraphicsItem>

#include <QPainter>
#include <QPalette>

#include <QLinearGradient>

#include <QGraphicsDropShadowEffect>

#include <algorithm>


//----------------------------------------------------------------------------
// qpwgraph_node -- Node graphics item.

// Constructor.
qpwgraph_node::qpwgraph_node (
	uint id, const QString& name, qpwgraph_item::Mode mode, uint type )
	: qpwgraph_item(nullptr),
		m_id(id), m_name(name), m_mode(mode), m_type(type)
{
	QGraphicsPathItem::setZValue(0.0);

	const QPalette pal;
	const int base_value = pal.base().color().value();
	const bool is_dark = (base_value < 128);

	const QColor& text_color = pal.text().color();
	QColor foreground_color(is_dark
		? text_color.darker()
		: text_color);
	qpwgraph_item::setForeground(foreground_color);

	const QColor& window_color = pal.window().color();
	QColor background_color(is_dark
		? window_color.lighter()
		: window_color);
	background_color.setAlpha(160);
	qpwgraph_item::setBackground(background_color);

	m_pixmap = new QGraphicsPixmapItem(this);
	m_text = new QGraphicsTextItem(this);

	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsMovable);
	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsSelectable);

	QGraphicsPathItem::setToolTip(m_name);
	setNodeTitle(m_name);

	const bool is_darkest = (base_value < 24);
	QColor shadow_color = (is_darkest ? Qt::white : Qt::black);
	shadow_color.setAlpha(180);

	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setColor(shadow_color);
	effect->setBlurRadius(is_darkest ? 8 : 16);
	effect->setOffset(is_darkest ? 0 : 2);
	QGraphicsPathItem::setGraphicsEffect(effect);

	qpwgraph_item::raise();
}


// Destructor.
qpwgraph_node::~qpwgraph_node (void)
{
	removePorts();

	// No actual need to destroy any children here...
	//
	//	QGraphicsPathItem::setGraphicsEffect(nullptr);

	//	delete m_text;
	//	delete m_pixmap;
}


// accessors.
uint qpwgraph_node::nodeId (void) const
{
	return m_id;
}

void qpwgraph_node::setNodeName ( const QString& name )
{
	m_name = name;

	QGraphicsPathItem::setToolTip(m_name);
}


const QString& qpwgraph_node::nodeName (void) const
{
	return m_name;
}


void qpwgraph_node::setNodeMode ( qpwgraph_item::Mode mode )
{
	m_mode = mode;
}


qpwgraph_item::Mode qpwgraph_node::nodeMode (void) const
{
	return m_mode;
}


void qpwgraph_node::setNodeType ( uint type )
{
	m_type = type;
}


uint qpwgraph_node::nodeType (void) const
{
	return m_type;
}


void qpwgraph_node::setNodeIcon ( const QIcon& icon )
{
	m_icon = icon;

	m_pixmap->setPixmap(m_icon.pixmap(24, 24));
}


const QIcon& qpwgraph_node::nodeIcon (void) const
{
	return m_icon;
}


void qpwgraph_node::setNodeTitle ( const QString& title )
{
	const QFont& font = m_text->font();
	m_text->setFont(QFont(font.family(), font.pointSize(), QFont::Bold));
	m_title = (title.isEmpty() ? m_name : title);

	static const int MAX_TITLE_LENGTH = 29;
	static const QString ellipsis(3, '.');

	QString text = m_title;
	if (text.length() >= MAX_TITLE_LENGTH  + ellipsis.length())
		text = text.left(MAX_TITLE_LENGTH) + ellipsis;

	m_text->setPlainText(text);
}


QString qpwgraph_node::nodeTitle (void) const
{
	return m_title;	// m_text->toPlainText();
}


// Port-list methods.
qpwgraph_port *qpwgraph_node::addPort (
	uint id, const QString& name, qpwgraph_item::Mode mode, int type )
{
	qpwgraph_port *port = new qpwgraph_port(this, id, name, mode, type);

	m_ports.append(port);
	m_port_ids.insert(qpwgraph_port::PortIdKey(port), port);
	m_port_keys.insert(qpwgraph_port::PortNameKey(port), port);

	updatePath();

	return port;
}


qpwgraph_port *qpwgraph_node::addInputPort (
	uint id, const QString& name, int type )
{
	return addPort(id, name, qpwgraph_item::Input, type);
}


qpwgraph_port *qpwgraph_node::addOutputPort (
	uint id, const QString& name, int type )
{
	return addPort(id, name, qpwgraph_item::Output, type);
}


void qpwgraph_node::removePort ( qpwgraph_port *port )
{
	m_port_keys.remove(qpwgraph_port::PortNameKey(port));
	m_port_ids.remove(qpwgraph_port::PortIdKey(port));
	m_ports.removeAll(port);

	updatePath();
}


void qpwgraph_node::removePorts (void)
{
	foreach (qpwgraph_port *port, m_ports)
		port->removeConnects();

	// Do not delete ports here as they are node's child items...
	//
	//qDeleteAll(m_ports);
	m_ports.clear();
	m_port_ids.clear();
	m_port_keys.clear();
}


// Port finder (by id/name, mode and type)
qpwgraph_port *qpwgraph_node::findPort (
	uint id, qpwgraph_item::Mode mode, uint type )
{
	return static_cast<qpwgraph_port *> (
		m_port_ids.value(qpwgraph_port::IdKey(id, mode, type), nullptr));
}


QList<qpwgraph_port *> qpwgraph_node::findPorts (
	const QString& name, qpwgraph_item::Mode mode, uint type )
{
	return m_port_keys.values(qpwgraph_port::PortNameKey(name, mode, type));
}


// Port-list accessor.
const QList<qpwgraph_port *>& qpwgraph_node::ports (void) const
{
	return m_ports;
}


// Reset port markings, destroy if unmarked.
void qpwgraph_node::resetMarkedPorts (void)
{
	QList<qpwgraph_port *> ports;

	foreach (qpwgraph_port *port, m_ports) {
		if (port->isMarked()) {
			port->setMarked(false);
		} else {
			ports.append(port);
		}
	}

	foreach (qpwgraph_port *port, ports) {
		port->removeConnects();
		removePort(port);
		delete port;
	}
}


// Path/shape updater.
void qpwgraph_node::updatePath (void)
{
	const QRectF& rect = m_text->boundingRect();
	int width = rect.width() / 2 + 24;
	int wi, wo;
	wi = wo = width;
	foreach (qpwgraph_port *port, m_ports) {
		const int w = port->itemRect().width();
		if (port->isOutput()) {
			if (wo < w) wo = w;
		} else {
			if (wi < w) wi = w;
		}
	}
	width = wi + wo;

	std::sort(m_ports.begin(), m_ports.end(), qpwgraph_port::Compare());

	int height = rect.height() + 2;
	int type = 0;
	int yi, yo;
	yi = yo = height;
	foreach (qpwgraph_port *port, m_ports) {
		const QRectF& port_rect = port->itemRect();
		const int w = port_rect.width();
		const int h = port_rect.height() + 1;
		if (type - port->portType()) {
			type = port->portType();
			height += 2;
			yi = yo = height;
		}
		if (port->isOutput()) {
			port->setPos(+width / 2 + 6 - w, yo);
			yo += h;
			if (height < yo)
				height = yo;
		} else {
			port->setPos(-width / 2 - 6, yi);
			yi += h;
			if (height < yi)
				height = yi;
		}
	}

	QPainterPath path;
	path.addRoundedRect(-width / 2, 0, width, height + 6, 5, 5);
	/*QGraphicsPathItem::*/setPath(path);
}


void qpwgraph_node::paint ( QPainter *painter,
	const QStyleOptionGraphicsItem *option, QWidget */*widget*/ )
{
	const QPalette& pal = option->palette;

	const QRectF& node_rect = itemRect();
	QLinearGradient node_grad(0, node_rect.top(), 0, node_rect.bottom());
	QColor node_color;
	if (QGraphicsPathItem::isSelected()) {
		const QColor& hilitetext_color = pal.highlightedText().color();
		m_text->setDefaultTextColor(hilitetext_color);
		painter->setPen(hilitetext_color);
		node_color = pal.highlight().color();
	} else {
		const QColor& foreground
			= qpwgraph_item::foreground();
		const QColor& background
			= qpwgraph_item::background();
		const bool is_dark
			= (background.value() < 192);
		m_text->setDefaultTextColor(is_dark
			? foreground.lighter()
			: foreground.darker());
		painter->setPen(foreground);
		node_color = background;
	}
	node_color.setAlpha(180);
	node_grad.setColorAt(0.6, node_color);
	node_grad.setColorAt(1.0, node_color.darker(120));
	painter->setBrush(node_grad);

	painter->drawPath(QGraphicsPathItem::path());

	m_pixmap->setPos(node_rect.x() + 4, node_rect.y() + 4);

	const QRectF& text_rect = m_text->boundingRect();
	m_text->setPos(- text_rect.width() / 2, text_rect.y() + 2);
}


QVariant qpwgraph_node::itemChange (
	GraphicsItemChange change, const QVariant& value )
{
	if (change == QGraphicsItem::ItemSelectedHasChanged) {
		const bool is_selected = value.toBool();
		foreach (qpwgraph_port *port, m_ports)
			port->setSelected(is_selected);
	}

	return value;
}


// Rectangular editor extents.
QRectF qpwgraph_node::editorRect (void) const
{
	return m_text->sceneBoundingRect();
}


// end of qpwgraph_node.cpp
