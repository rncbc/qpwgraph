// qpwgraph_arrange_command.cpp -- Undo/redo action for topological node arrangement
//
/****************************************************************************
   Copyright (C) 2025, Mike Bourgeous. All rights reserved.

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

#include "qpwgraph_arrange_command.h"
#include "qpwgraph_canvas.h"

qpwgraph_arrange_command::qpwgraph_arrange_command(qpwgraph_canvas *canvas, QMap<qpwgraph_node *, QPointF> newPositions, qpwgraph_command *parent) :
	qpwgraph_move_command(canvas, QList<qpwgraph_node *>(), QPointF(), QPointF(), parent)
{
	setText(QObject::tr("Arrange %1 Nodes").arg(newPositions.size()));

	foreach (qpwgraph_node *n, newPositions.keys()) {
		addItem(n, n->pos(), newPositions[n]);
	}
}

bool qpwgraph_arrange_command::execute(bool is_undo)
{
	bool ret = qpwgraph_move_command::execute(is_undo);

	qpwgraph_canvas *canvas = qpwgraph_command::canvas();
	if (canvas != nullptr) {
		canvas->update();
		canvas->centerView();
	}

	return ret;
}
