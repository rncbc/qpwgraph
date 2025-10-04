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

class qpwgraph_toposort
{
public:

	qpwgraph_toposort(QList<qpwgraph_node *> nodes);

	QList<qpwgraph_node *> sort();

	static qsizetype countInputPorts(qpwgraph_node *n);
	static qsizetype countOutputPorts(qpwgraph_node *n);

	static qsizetype countExternalConnections(qpwgraph_node *n, bool input);
	static qsizetype countInputConnections(qpwgraph_node *n);
	static qsizetype countOutputConnections(qpwgraph_node *n);

	static bool nodeIsTrueSource(qpwgraph_node *n);
	static bool nodeIsEffectiveSource(qpwgraph_node *n);
	static bool nodeIsTrueSink(qpwgraph_node *n);

	static QSet<qpwgraph_node *> childNodes(qpwgraph_node *n);

	static QSet<qpwgraph_port *> connectedParentPorts(qpwgraph_node *n);
	static QSet<qpwgraph_port *> connectedInputPorts(qpwgraph_node *n);
	static QSet<qpwgraph_port *> connectedOutputPorts(qpwgraph_node *n);

	static bool compareNodes(qpwgraph_node *n1, qpwgraph_node *n2);

	static QString modeName(qpwgraph_item::Mode mode);
	static std::string debugNode(qpwgraph_node *n);
	static std::string debugConnection(qpwgraph_connect *c);
	static std::string debugPath(QSet<qpwgraph_node *> path);

private:
	void visitNode(QSet<qpwgraph_node *> path, qpwgraph_node *n);

	// Instance variables
	QList<qpwgraph_node *> inputNodes;
	QList<qpwgraph_node *> unvisitedNodes;
	QSet<qpwgraph_node *> visitedNodes;
};

#endif /* __qpwgraph_toposort_h */
