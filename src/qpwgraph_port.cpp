// qpwgraph_port.cpp
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

#include "qpwgraph_port.h"

#include "qpwgraph_canvas.h"
#include "qpwgraph_node.h"
#include "qpwgraph_connect.h"

#include <QGraphicsScene>

#include <QStyleOptionGraphicsItem>

#include <QPainter>
#include <QPalette>

#include <QLinearGradient>


//----------------------------------------------------------------------------
// qpwgraph_port -- Port graphics item.

// Constructor.
qpwgraph_port::qpwgraph_port ( qpwgraph_node *node, uint id,
	const QString& name, qpwgraph_item::Mode mode, uint type )
	: qpwgraph_item(node), m_node(node), m_id(id),
		m_name(name), m_mode(mode), m_type(type),
		m_index(node->ports().count()),
		m_selectx(0), m_hilitex(0)
{
	QGraphicsPathItem::setZValue(+1.0);

	const QPalette pal;
	setForeground(pal.buttonText().color());
	setBackground(pal.button().color());

	m_text = new QGraphicsTextItem(this);

	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsSelectable);
	QGraphicsPathItem::setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

	QGraphicsPathItem::setAcceptHoverEvents(true);

	QGraphicsPathItem::setToolTip(m_name);

	setPortTitle(m_name);
}


// Destructor.
qpwgraph_port::~qpwgraph_port (void)
{
	removeConnects();

	// No actual need to destroy any children here...
	//
	//delete m_text;
}


// Accessors.
qpwgraph_node *qpwgraph_port::portNode (void) const
{
	return m_node;
}


uint qpwgraph_port::portId (void) const
{
	return m_id;
}


void qpwgraph_port::setPortName ( const QString& name )
{
	m_name = name;

	QGraphicsPathItem::setToolTip(m_name);
}


const QString& qpwgraph_port::portName (void) const
{
	return m_name;
}


void qpwgraph_port::setPortMode ( qpwgraph_item::Mode mode )
{
	m_mode = mode;
}


qpwgraph_item::Mode qpwgraph_port::portMode (void) const
{
	return m_mode;
}


bool qpwgraph_port::isInput (void) const
{
	return (m_mode & Input);
}


bool qpwgraph_port::isOutput (void) const
{
	return (m_mode & Output);
}


void qpwgraph_port::setPortType ( uint type )
{
	m_type = type;
}


uint qpwgraph_port::portType (void) const
{
	return m_type;
}


void qpwgraph_port::setPortTitle ( const QString& title )
{
	m_title = (title.isEmpty() ? m_name : title);

	m_text->setPlainText(m_title);

	QPainterPath path;
	const QRectF& rect
		= m_text->boundingRect().adjusted(0, +2, 0, -2);
	path.addRoundedRect(rect, 5, 5);
	/*QGraphicsPathItem::*/setPath(path);
}


const QString& qpwgraph_port::portTitle (void) const
{
	return m_title;
}


void qpwgraph_port::setPortIndex ( int index )
{
	m_index = index;
}


int qpwgraph_port::portIndex (void) const
{
	return m_index;
}


QPointF qpwgraph_port::portPos (void) const
{
	QPointF pos = QGraphicsPathItem::scenePos();

	const QRectF& rect = itemRect();
	if (m_mode == Output)
		pos.setX(pos.x() + rect.width());
	pos.setY(pos.y() + rect.height() / 2);

	return pos;
}


// Connection-list methods.
void qpwgraph_port::appendConnect ( qpwgraph_connect *connect )
{
	m_connects.append(connect);
}


void qpwgraph_port::removeConnect ( qpwgraph_connect *connect )
{
	m_connects.removeAll(connect);
}


void qpwgraph_port::removeConnects (void)
{
	foreach (qpwgraph_connect *connect, m_connects) {
		if (connect->port1() != this)
			connect->setPort1(nullptr);
		if (connect->port2() != this)
			connect->setPort2(nullptr);
	}

	// Do not delete connects here as they are owned elsewhere...
	//
	//	qDeleteAll(m_connects);
	m_connects.clear();
}


qpwgraph_connect *qpwgraph_port::findConnect ( qpwgraph_port *port ) const
{
	foreach (qpwgraph_connect *connect, m_connects) {
		if (connect->port1() == port || connect->port2() == port)
			return connect;
	}

	return nullptr;
}


// Connect-list accessor.
const QList<qpwgraph_connect *>& qpwgraph_port::connects (void) const
{
	return m_connects;
}


void qpwgraph_port::paint ( QPainter *painter,
	const QStyleOptionGraphicsItem *option, QWidget */*widget*/ )
{
	const QPalette& pal = option->palette;

	const QRectF& port_rect = itemRect();
	QLinearGradient port_grad(0, port_rect.top(), 0, port_rect.bottom());
	QColor port_color;
	if (QGraphicsPathItem::isSelected()) {
		m_text->setDefaultTextColor(pal.highlightedText().color());
		painter->setPen(pal.highlightedText().color());
		port_color = pal.highlight().color();
	} else {
		const QColor& foreground
			= qpwgraph_item::foreground();
		const QColor& background
			= qpwgraph_item::background();
		const bool is_dark
			= (background.value() < 128);
		m_text->setDefaultTextColor(is_dark
			? foreground.lighter()
			: foreground.darker());
		if (qpwgraph_item::isHighlight() || QGraphicsPathItem::isUnderMouse()) {
			painter->setPen(foreground.lighter());
			port_color = background.lighter();
		} else {
			painter->setPen(foreground);
			port_color = background;
		}
	}
	port_grad.setColorAt(0.0, port_color);
	port_grad.setColorAt(1.0, port_color.darker(120));
	painter->setBrush(port_grad);

	painter->drawPath(QGraphicsPathItem::path());
}


QVariant qpwgraph_port::itemChange (
	GraphicsItemChange change, const QVariant& value )
{
	if (change == QGraphicsItem::ItemScenePositionHasChanged) {
		foreach (qpwgraph_connect *connect, m_connects) {
			connect->updatePath();
		}
	}
	else
	if (change == QGraphicsItem::ItemSelectedHasChanged && m_selectx < 1) {
		const bool is_selected = value.toBool();
		setHighlightEx(is_selected);
		foreach (qpwgraph_connect *connect, m_connects)
			connect->setSelectedEx(this, is_selected);
	}

	return value;
}


// Selection propagation method...
void qpwgraph_port::setSelectedEx ( bool is_selected )
{
	if (!is_selected) {
		foreach (qpwgraph_connect *connect, m_connects) {
			if (connect->isSelected()) {
				setHighlightEx(true);
				return;
			}
		}
	}

	++m_selectx;

	setHighlightEx(is_selected);

	if (QGraphicsPathItem::isSelected() != is_selected)
		QGraphicsPathItem::setSelected(is_selected);

	--m_selectx;
}


// Highlighting propagation method...
void qpwgraph_port::setHighlightEx ( bool is_highlight )
{
	if (m_hilitex > 0)
		return;

	++m_hilitex;

	qpwgraph_item::setHighlight(is_highlight);

	foreach (qpwgraph_connect *connect, m_connects)
		connect->setHighlightEx(this, is_highlight);

	--m_hilitex;
}


// Special port-type color business.
void qpwgraph_port::updatePortTypeColors ( qpwgraph_canvas *canvas )
{
	if (canvas) {
		const QColor& color = canvas->portTypeColor(m_type);
		if (color.isValid()) {
			const bool is_dark = (color.value() < 128);
			qpwgraph_item::setForeground(is_dark
				? color.lighter(180)
				: color.darker());
			qpwgraph_item::setBackground(color);
			if (m_mode & Output) {
				foreach (qpwgraph_connect *connect, m_connects) {
					connect->updatePortTypeColors();
					connect->update();
				}
			}
		}
	}
}


// Port sorting type.
qpwgraph_port::SortType  qpwgraph_port::g_sort_type  = qpwgraph_port::PortName;

void qpwgraph_port::setSortType ( SortType sort_type )
{
	g_sort_type = sort_type;
}

qpwgraph_port::SortType qpwgraph_port::sortType (void)
{
	return g_sort_type;
}


// Port sorting order.
qpwgraph_port::SortOrder qpwgraph_port::g_sort_order = qpwgraph_port::Ascending;

void qpwgraph_port::setSortOrder( SortOrder sort_order )
{
	g_sort_order = sort_order;
}

qpwgraph_port::SortOrder qpwgraph_port::sortOrder (void)
{
	return g_sort_order;
}


// Natural decimal sorting comparator (static)
bool qpwgraph_port::lessThan ( qpwgraph_port *port1, qpwgraph_port *port2 )
{
	const int port_type_diff
		= int(port1->portType()) - int(port2->portType());
	if (port_type_diff)
		return (port_type_diff > 0);

	if (g_sort_order == Descending) {
		qpwgraph_port *port = port1;
		port1 = port2;
		port2 = port;
	}

	if (g_sort_type == PortIndex) {
		const int port_index_diff
			= port1->portIndex() - port2->portIndex();
		if (port_index_diff)
			return (port_index_diff < 0);
	}

	switch (g_sort_type) {
	case PortTitle:
		return qpwgraph_port::lessThan(port1->portTitle(), port2->portTitle());
	case PortName:
	default:
		return qpwgraph_port::lessThan(port1->portName(), port2->portName());
	}
}

bool qpwgraph_port::lessThan ( const QString& s1, const QString& s2 )
{
	const int n1 = s1.length();
	const int n2 = s2.length();

	int i1, i2;

	for (i1 = i2 = 0; i1 < n1 && i2 < n2; ++i1, ++i2) {

		// Skip (white)spaces...
		while (s1.at(i1).isSpace())
			++i1;
		while (s2.at(i2).isSpace())
			++i2;

		// Normalize (to uppercase) the next characters...
		QChar c1 = s1.at(i1).toUpper();
		QChar c2 = s2.at(i2).toUpper();

		if (c1.isDigit() && c2.isDigit()) {
			// Find the whole length numbers...
			int j1 = i1++;
			while (i1 < n1 && s1.at(i1).isDigit())
				++i1;
			int j2 = i2++;
			while (i2 < n2 && s2.at(i2).isDigit())
				++i2;
			// Compare as natural decimal-numbers...
			j1 = s1.mid(j1, i1 - j1).toInt();
			j2 = s2.mid(j2, i2 - j2).toInt();
			if (j1 != j2)
				return (j1 < j2);
			// Never go out of bounds...
			if (i1 >= n1 || i2 >= n2)
				break;
			// Go on with this next char...
			c1 = s1.at(i1).toUpper();
			c2 = s2.at(i2).toUpper();
		}

		// Compare this char...
		if (c1 != c2)
			return (c1 < c2);
	}

	// Probable exact match.
	return false;
}


// Rectangular editor extents.
QRectF qpwgraph_port::editorRect (void) const
{
	return QGraphicsPathItem::sceneBoundingRect();
}


// end of qpwgraph_port.cpp
