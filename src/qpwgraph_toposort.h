// qpwgraph_toposort.h -- Topological sort and related code for graph nodes
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

#include <string>

#include <QtContainerFwd>


// Contains code for topologically sorting nodes, as well as arranging those
// nodes based on the sort.
//
// This class is non-reentrant, so use each instance just once from a single
// function.
class qpwgraph_toposort
{
public:

	qpwgraph_toposort(QList<qpwgraph_node *> nodes);

	// Call this to perform ranking and node arrangement.  Returns the map
	// of new positions without applying those positions to the nodes.
	QMap<qpwgraph_node *, QPointF> arrange();

private:
	void rankAndSort();
	void visitNode(QSet<qpwgraph_node *> path, qpwgraph_node *n);

	void sortNodesByRank(QList<qpwgraph_node *> &nodes);
	void sortNodesByConnectionY(QList<qpwgraph_node *> &nodes);

	static qsizetype countInputPorts(qpwgraph_node *n);
	static qsizetype countOutputPorts(qpwgraph_node *n);

	static qsizetype countExternalConnections(qpwgraph_node *n, bool input);
	static qsizetype countInputConnections(qpwgraph_node *n);
	static qsizetype countOutputConnections(qpwgraph_node *n);

	static bool nodeIsTrueSource(qpwgraph_node *n);
	static bool nodeIsEffectiveSource(qpwgraph_node *n);
	static bool nodeIsTrueSink(qpwgraph_node *n);

	static QSet<qpwgraph_node *> childNodes(qpwgraph_node *n);

	QSet<qpwgraph_port *> connectedParentPorts(qpwgraph_node *n);
	QSet<qpwgraph_port *> connectedInputPorts(qpwgraph_node *n);
	QSet<qpwgraph_port *> connectedOutputPorts(qpwgraph_node *n);

	qreal meanPortY(QSet<qpwgraph_port *> ports, QMap<qpwgraph_node *, QPointF> positions);
	qreal meanParentPortY(QList<qpwgraph_node *> nodes, QMap<qpwgraph_node *, QPointF> positions);
	qreal meanInputPortY(QList<qpwgraph_node *> nodes, QMap<qpwgraph_node *, QPointF> positions);

	bool compareNodes(qpwgraph_node *n1, qpwgraph_node *n2);

	// Instance variables
	QList<qpwgraph_node *> inputNodes;
	QList<qpwgraph_node *> unvisitedNodes;

	QMap<qpwgraph_node *, QPointF> newPositions;
	QMap<qpwgraph_node *, int> nodeRanks;
};

#endif /* __qpwgraph_toposort_h */
