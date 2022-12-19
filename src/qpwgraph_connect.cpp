// qpwgraph_connect.cpp
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

#include "qpwgraph_connect.h"

#include "qpwgraph_node.h"

#include <QGraphicsScene>

#include <QStyleOptionGraphicsItem>

#include <QPainter>
#include <QPalette>

#if 0//Disable drop-shadow effect...
#include <QGraphicsDropShadowEffect>
#endif

#include <cmath>


//----------------------------------------------------------------------------
// qpwgraph_connect -- Connection-line graphics item.

// Constructor.
qpwgraph_connect::qpwgraph_connect (void)
	: qpwgraph_item(nullptr), m_port1(nullptr), m_port2(nullptr), m_dimmed(false)
{
	QGraphicsPathItem::setZValue(-1.0);

	QGraphicsPathItem::setFlag(QGraphicsItem::ItemIsSelectable);

	qpwgraph_item::setBackground(qpwgraph_item::foreground());

#if 0//Disable drop-shadow effect...
	const QPalette pal;
	const bool is_darkest = (pal.base().color().value() < 24);
	QColor shadow_color = (is_darkest ? Qt::white : Qt::black);
	shadow_color.setAlpha(220);
	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setColor(shadow_color);
	effect->setBlurRadius(is_darkest ? 4 : 8);
	effect->setOffset(is_darkest ? 0 : 1);
	QGraphicsPathItem::setGraphicsEffect(effect);
#endif
	QGraphicsPathItem::setAcceptHoverEvents(true);

	qpwgraph_item::raise();
}


// Destructor.
qpwgraph_connect::~qpwgraph_connect (void)
{
	// No actual need to destroy any children here...
	//
	//QGraphicsPathItem::setGraphicsEffect(nullptr);
}


// Accessors.
void qpwgraph_connect::setPort1 ( qpwgraph_port *port )
{
	if (m_port1)
		m_port1->removeConnect(this);

	m_port1 = port;

	if (m_port1)
		m_port1->appendConnect(this);

	if (m_port1 && m_port1->isSelected())
		setSelectedEx(m_port1, true);
}


qpwgraph_port *qpwgraph_connect::port1 (void) const
{
	return m_port1;
}


void qpwgraph_connect::setPort2 ( qpwgraph_port *port )
{
	if (m_port2)
		m_port2->removeConnect(this);

	m_port2 = port;

	if (m_port2)
		m_port2->appendConnect(this);

	if (m_port2 && m_port2->isSelected())
		setSelectedEx(m_port2, true);
}


qpwgraph_port *qpwgraph_connect::port2 (void) const
{
	return m_port2;
}


// Active disconnection.
void qpwgraph_connect::disconnect (void)
{
	if (m_port1) {
		m_port1->removeConnect(this);
		m_port1 = nullptr;
	}

	if (m_port2) {
		m_port2->removeConnect(this);
		m_port2 = nullptr;
	}
}


// Path/shaper updaters.
void qpwgraph_connect::updatePathTo ( const QPointF& pos )
{
	const bool is_out0 = m_port1->isOutput();
	const QPointF pos0 = m_port1->portPos();

	const QPointF d1(1.0, 0.0);
	const QPointF pos1 = (is_out0 ? pos0 + d1 : pos  - d1);
	const QPointF pos4 = (is_out0 ? pos  - d1 : pos0 + d1);

	const QPointF d2(2.0, 0.0);
	const QPointF pos1_2(is_out0 ? pos1 + d2 : pos1 - d2);
	const QPointF pos3_4(is_out0 ? pos4 - d2 : pos4 + d2);

	qpwgraph_node *node1 = m_port1->portNode();
	const QRectF& rect1 = node1->itemRect();
	const qreal h1 = 0.5 * rect1.height();
	const qreal dh = pos0.y() - node1->scenePos().y() - h1;
	const qreal dx = pos3_4.x() - pos1_2.x();
	const qreal x_max = rect1.width() + h1;
	const qreal x_min = qMin(x_max, qAbs(dx));
	const qreal x_offset = (dx > 0.0 ? 0.5 : 1.0) * x_min;

	qreal y_offset = 0.0;
	if (g_connect_through_nodes) {
		// New "normal" connection line curves (inside/through nodes)...
		const qreal h2 = m_port1->itemRect().height();
		const qreal dy = qAbs(pos3_4.y() - pos1_2.y());
		y_offset = (dx > -h2 || dy > h2 ? 0.0 : (dh > 0.0 ? +h2 : -h2));
	} else {
		// Old "weird" connection line curves (outside/around nodes)...
		y_offset = (dx > 0.0 ? 0.0 : (dh > 0.0 ? +x_min : -x_min));
	}

	const QPointF pos2(pos1.x() + x_offset, pos1.y() + y_offset);
	const QPointF pos3(pos4.x() - x_offset, pos4.y() + y_offset);

	QPainterPath path;
	path.moveTo(pos1);
	path.lineTo(pos1_2);
	path.cubicTo(pos2, pos3, pos3_4);
	path.lineTo(pos4);

	const qreal arrow_angle = path.angleAtPercent(0.5) * M_PI / 180.0;
	const QPointF arrow_pos0 = path.pointAtPercent(0.5);
	const qreal arrow_size = 8.0;
	QVector<QPointF> arrow;
	arrow.append(arrow_pos0);
	arrow.append(arrow_pos0 - QPointF(
		::sin(arrow_angle + M_PI / 2.25) * arrow_size,
		::cos(arrow_angle + M_PI / 2.25) * arrow_size));
	arrow.append(arrow_pos0 - QPointF(
		::sin(arrow_angle + M_PI - M_PI / 2.25) * arrow_size,
		::cos(arrow_angle + M_PI - M_PI / 2.25) * arrow_size));
	arrow.append(arrow_pos0);
	path.addPolygon(QPolygonF(arrow));

	/*QGraphicsPathItem::*/setPath(path);
}


void qpwgraph_connect::updatePath (void)
{
	if (m_port2) 
		updatePathTo(m_port2->portPos());
}


void qpwgraph_connect::paint ( QPainter *painter,
	const QStyleOptionGraphicsItem *option, QWidget */*widget*/ )
{
	QColor color;
	if (QGraphicsPathItem::isSelected())
		color = option->palette.highlight().color();
	else
	if (qpwgraph_item::isHighlight() || QGraphicsPathItem::isUnderMouse())
		color = qpwgraph_item::foreground().lighter();
	else
		color = qpwgraph_item::foreground();
	color.setAlpha(m_dimmed ? 160 : 255);

	const QPalette pal;
	const bool is_darkest = (pal.base().color().value() < 24);
	QColor shadow_color = (is_darkest ? Qt::white : Qt::black);
	shadow_color.setAlpha(m_dimmed ? 40 : 80);

	const QPainterPath& path
		= QGraphicsPathItem::path();
	painter->setBrush(Qt::NoBrush);
	painter->setPen(QPen(shadow_color, 3));
	painter->drawPath(path.translated(+1.0, +1.0));
	painter->setPen(QPen(color, 2));
	painter->drawPath(path);
}


QVariant qpwgraph_connect::itemChange (
	GraphicsItemChange change, const QVariant& value )
{
	if (change == QGraphicsItem::ItemSelectedHasChanged) {
		const bool is_selected = value.toBool();
		qpwgraph_item::setHighlight(is_selected);
		if (m_port1)
			m_port1->setSelectedEx(is_selected);
		if (m_port2)
			m_port2->setSelectedEx(is_selected);
	}

	return value;
}


QPainterPath qpwgraph_connect::shape (void) const
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 1, 0)) && (__cplusplus < 201703L)
	return QGraphicsPathItem::shape();
#else
	const QPainterPathStroker stroker
		= QPainterPathStroker(QPen(QColor(), 2));
	return stroker.createStroke(path());
#endif
}


// Selection propagation method...
void qpwgraph_connect::setSelectedEx ( qpwgraph_port *port, bool is_selected )
{
	setHighlightEx(port, is_selected);

	if (QGraphicsPathItem::isSelected() != is_selected) {
	#if 0//OLD_SELECT_BEHAVIOR
		QGraphicsPathItem::setSelected(is_selected);
		if (is_selected) {
			if (m_port1 && m_port1 != port)
				m_port1->setSelectedEx(is_selected);
			if (m_port2 && m_port2 != port)
				m_port2->setSelectedEx(is_selected);
		}
	#else
		if (!is_selected || (m_port1 && m_port2
			&& m_port1->isSelected() && m_port2->isSelected())) {
			QGraphicsPathItem::setSelected(is_selected);
		}
	#endif
	}
}

// Highlighting propagation method...
void qpwgraph_connect::setHighlightEx ( qpwgraph_port *port, bool is_highlight )
{
	qpwgraph_item::setHighlight(is_highlight);

	if (m_port1 && m_port1 != port)
		m_port1->setHighlight(is_highlight);
	if (m_port2 && m_port2 != port)
		m_port2->setHighlight(is_highlight);
}


// Special port-type color business.
void qpwgraph_connect::updatePortTypeColors (void)
{
	if (m_port1) {
		const QColor& color
			= m_port1->background().lighter();
		qpwgraph_item::setForeground(color);
		qpwgraph_item::setBackground(color);
	}
}


	// Dim/transparency option.
void qpwgraph_connect::setDimmed ( bool dimmed )
{
	m_dimmed = dimmed;

	update();
}

int qpwgraph_connect::isDimmed (void) const
{
	return m_dimmed;
}


// Connector curve draw style (through vs. around nodes)
//
bool qpwgraph_connect::g_connect_through_nodes = false;

void qpwgraph_connect::setConnectThroughNodes ( bool on )
{
	g_connect_through_nodes = on;
}

bool qpwgraph_connect::isConnectThroughNodes (void)
{
	return g_connect_through_nodes;
}


// end of qpwgraph_connect.cpp
