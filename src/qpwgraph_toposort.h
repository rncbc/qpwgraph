// qpwgraph_toposort.h
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


#ifndef __qpwgraph_toposort_h
#define __qpwgraph_toposort_h

#include "qpwgraph_node.h"

#include <QSet>


//----------------------------------------------------------------------------
// qpwgraph_toposort -- Topological sort and related code for graph nodes decl.
//

// Contains code for topologically sorting nodes, as well as arranging those
// nodes based on the sort.
//
// This class is non-reentrant, so use each instance just once from a single
// function.
class qpwgraph_toposort
{
public:

	//Constructor.
	qpwgraph_toposort(const QList<qpwgraph_node *>& nodes);

	// Call this to perform ranking and node arrangement.  Returns the map
	// of new positions without applying those positions to the nodes.
	const QHash<qpwgraph_node *, QPointF>& arrange();

protected:

	void rankAndSort();
	void visitNode(const QSet<qpwgraph_node *>& path, qpwgraph_node *n);

	void sortNodesByRank(QList<qpwgraph_node *>& nodes);
	void sortNodesByConnectionY(QList<qpwgraph_node *>& nodes);

	static qsizetype countInputPorts(qpwgraph_node *n);
	static qsizetype countOutputPorts(qpwgraph_node *n);

	static qsizetype countExternalConnections(qpwgraph_node *n, bool input);
	static qsizetype countInputConnections(qpwgraph_node *n);

	static bool nodeIsTrueSource(qpwgraph_node *n);
	static bool nodeIsEffectiveSource(qpwgraph_node *n);
	static bool nodeIsTrueSink(qpwgraph_node *n);

	static QSet<qpwgraph_node *> childNodes(qpwgraph_node *n);

	QSet<qpwgraph_port *> connectedParentPorts(qpwgraph_node *n);
	QSet<qpwgraph_port *> connectedInputPorts(qpwgraph_node *n);
	QSet<qpwgraph_port *> connectedOutputPorts(qpwgraph_node *n);

	qreal meanPortY(const QSet<qpwgraph_port *>& ports, const QHash<qpwgraph_node *, QPointF>& positions);
	qreal meanParentPortY(const QList<qpwgraph_node *>& nodes, const QHash<qpwgraph_node *, QPointF>& positions);
	qreal meanInputPortY(const QList<qpwgraph_node *>& nodes, const QHash<qpwgraph_node *, QPointF>& positions);

	bool compareNodes(qpwgraph_node *n1, qpwgraph_node *n2);
	bool compareNodesByConnectionLocation(qpwgraph_node *n1, qpwgraph_node *n2);

private:

	// Instance variables
	QList<qpwgraph_node *> m_inputNodes;
	QList<qpwgraph_node *> m_unvisitedNodes;

	QHash<qpwgraph_node *, QPointF> m_newPositions;
	QHash<qpwgraph_node *, int> m_nodeRanks;
};

#endif /* __qpwgraph_toposort_h */

// end of qpwgraph_toposort.h
