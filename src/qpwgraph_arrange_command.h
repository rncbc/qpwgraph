// qpwgraph_arrange_command.h -- Undo/redo action for topological node arrangement
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

#ifndef __qpwgraph_arrange_command_h
#define __qpwgraph_arrange_command_h

#include "qpwgraph_command.h"

// Undo/redo action for the Arrange Nodes command.  This should be created
// before applying the new positions to the nodes.
class qpwgraph_arrange_command : public qpwgraph_move_command
{
public:

	qpwgraph_arrange_command(qpwgraph_canvas *canvas, QMap<qpwgraph_node *, QPointF> newPositions, qpwgraph_command *parent);

protected:

	bool execute(bool is_undo) override;

private:

	qpwgraph_canvas *canvas;
	QList<qpwgraph_node *> nodes;
	QMap<qpwgraph_node *, QPointF> newPositions;
};

#endif // __qpwgraph_arrange_command_h
