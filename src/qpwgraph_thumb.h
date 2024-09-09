// qpwgraph_thumb.h
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

#ifndef __qpwgraph_thumb_h
#define __qpwgraph_thumb_h

#include <QFrame>


// Forward decls.
class qpwgraph_canvas;


//----------------------------------------------------------------------------
// qpwgraph_thumb -- Thumb graphics scene/view.

class qpwgraph_thumb : public QFrame
{
	Q_OBJECT

public:

	// Corner position.
	enum Position { None = 0, TopLeft, TopRight, BottomLeft, BottomRight };

	// Constructor.
	qpwgraph_thumb(qpwgraph_canvas *canvas, Position position = BottomLeft);

	// Destructor.
	~qpwgraph_thumb();

	// Accessors.
	qpwgraph_canvas *canvas() const;

	void setPosition(Position position);
	Position position() const;

	// Emit context-menu request.
	void requestContextMenu(const QPoint& pos);

	// Request re-positioning.
	void requestPosition(Position position);

signals:

	// Context-menu request.
	void contextMenuRequested(const QPoint& pos);

	// Re-positioning request.
	void positionRequested(int position);

public slots:

	// Update view slot.
	void updateView();

protected:

	// Update position.
	void updatePosition();

	// Forward decl.
	class View;

private:

	// Inatance members.
	qpwgraph_canvas *m_canvas;
	Position m_position;
	View *m_view;
};


#endif	// __qpwgraph_thumb_h

// end of qpwgraph_thumb.h
