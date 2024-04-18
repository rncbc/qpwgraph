// qpwgraph_thumb.cpp
//
/****************************************************************************
   Copyright (C) 2018-2024, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qpwgraph_thumb.h"

#include "qpwgraph_canvas.h"

#include <QVBoxLayout>
#include <QScrollBar>
#include <QMouseEvent>

#include <QApplication>
#include <QMenu>


//----------------------------------------------------------------------------
// qpwgraph_thumb::View -- Thumb graphics scene/view.

class qpwgraph_thumb::View : public QGraphicsView
{
public:

	// Constructor.
	View(qpwgraph_canvas *canvas)
		: QGraphicsView(canvas->viewport()),
			m_canvas(canvas), m_drag_state(DragNone)
	{
	//	QGraphicsView::setInteractive(false);
		QGraphicsView::setRenderHints(QPainter::Antialiasing);
		QGraphicsView::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		QGraphicsView::setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		QGraphicsView::setScene(m_canvas->scene());

		QPalette pal = QGraphicsView::palette();
		const QColor& base = pal.base().color();
		pal.setColor(QPalette::Base, base.darker(120));
		QGraphicsView::setPalette(pal);
	}

protected:

	// Compute the view(port) rectangle.
	QRect viewRect() const
	{
		const QRect& vrect
			= m_canvas->viewport()->rect();
		const QRectF srect(
			m_canvas->mapToScene(vrect.topLeft()),
			m_canvas->mapToScene(vrect.bottomRight()));
		return QGraphicsView::viewport()->rect().intersected(QRect(
			QGraphicsView::mapFromScene(srect.topLeft()),
			QGraphicsView::mapFromScene(srect.bottomRight())))
			.adjusted(0, 0, -1, -1);
	}

	// View paint method.
	void paintEvent(QPaintEvent *event)
	{
		QGraphicsView::paintEvent(event);

		QPainter painter(QGraphicsView::viewport());
		painter.setPen(Qt::darkGray);
		painter.drawRect(viewRect());
	}

	// Handle mouse events.
	//
	void mousePressEvent(QMouseEvent *event)
	{
		QGraphicsView::mousePressEvent(event);

		if (event->button() == Qt::LeftButton) {
			m_drag_pos = event->pos();
			if (viewRect().contains(m_drag_pos))
				m_drag_state = DragStart;
			else
				m_drag_state = DragClick;
		}
	}

	void mouseMoveEvent(QMouseEvent *event)
	{
		QGraphicsView::mouseMoveEvent(event);

		const QPoint& pos = event->pos();
		if (m_drag_state == DragStart
			&& (pos - m_drag_pos).manhattanLength()
				> QApplication::startDragDistance()) {
			m_drag_state = DragMove;
		}
		else
		if (m_drag_state == DragMove) {
			m_canvas->centerOn(
				QGraphicsView::mapToScene(pos));
		}
	}

	void mouseReleaseEvent(QMouseEvent *event)
	{
		QGraphicsView::mouseReleaseEvent(event);

		if (m_drag_state != DragNone) {
			m_canvas->centerOn(
				QGraphicsView::mapToScene(event->pos()));
			m_drag_state = DragNone;
		}
	}

private:

	// Instance members.
	qpwgraph_canvas *m_canvas;

	enum { DragNone = 0, DragStart, DragMove, DragClick } m_drag_state;

	QPoint m_drag_pos;
};


//----------------------------------------------------------------------------
// qpwgraph_thumb -- Thumb graphics scene/view.

// Constructor.
qpwgraph_thumb::qpwgraph_thumb ( qpwgraph_canvas *canvas, Position position )
	: QFrame(canvas), m_canvas(canvas), m_position(position), m_view(nullptr)
{
	m_view = new View(m_canvas);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_view);
	QFrame::setLayout(layout);

	QFrame::setFrameStyle(QFrame::Panel);
	QFrame::setForegroundRole(QPalette::Window);

	QObject::connect(m_canvas->horizontalScrollBar(),
		SIGNAL(valueChanged(int)), SLOT(updateView()));
	QObject::connect(m_canvas->verticalScrollBar(),
		SIGNAL(valueChanged(int)), SLOT(updateView()));
}


// Destructor.
qpwgraph_thumb::~qpwgraph_thumb (void)
{
}


// Accessors.
qpwgraph_canvas *qpwgraph_thumb::canvas (void) const
{
	return m_canvas;
}


void qpwgraph_thumb::setPosition ( Position position )
{
	m_position = position;

	updatePosition();
}


qpwgraph_thumb::Position qpwgraph_thumb::position (void) const
{
	return m_position;
}


// Update view.
void qpwgraph_thumb::updatePosition (void)
{
	const QRect& rect = m_canvas->viewport()->rect();
	const int w = rect.width()  / 4;
	const int h = rect.height() / 4;
	QFrame:setFixedSize(w, h);

	switch (m_position) {
	case TopLeft:
		QFrame::move(0, 0);
		break;
	case TopRight:
		QFrame::move(rect.width() - w, 0);
		break;
	case BottomLeft:
		QFrame::move(0, rect.height() - h);
		break;
	case BottomRight:
		QFrame::move(rect.width() - w, rect.height() - h);
		break;
	case None:
	default:
		break;
	}
}


// Update view.
void qpwgraph_thumb::updateView (void)
{
	updatePosition();

	const qreal m = 24.0;
	m_view->fitInView(
		m_canvas->scene()->itemsBoundingRect()
			.marginsAdded(QMarginsF(m, m, m, m)),
		Qt::KeepAspectRatio);
}


// end of qpwgraph_thumb.cpp
