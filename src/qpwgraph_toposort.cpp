// qpwgraph_toposort.cpp -- Topological sort and related code for graph nodes
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

#include "qpwgraph_toposort.h"
#include "qpwgraph_connect.h"
#include "qpwgraph_port.h"
#include "qpwgraph_node.h"
#include "qpwgraph_arrange_command.h"

#include <algorithm>

qpwgraph_toposort::qpwgraph_toposort(QList<qpwgraph_node *> nodes) :
	inputNodes(nodes)
{
}

// Rearrange nodes by type and connection using a topological ordering.
// Nodes with no incoming connections go first, nodes with no outgoing
// connections go last.  Nodes are arranged in columns based on their distance
// in the graph from a source node.
//
// Returns a Map from node pointer to position, without modifying the nodes.
QMap<qpwgraph_node *, QPointF> qpwgraph_toposort::arrange()
{
	// Sort nodes topologically, using heuristics to break cycles.
	rankAndSort();

	// Precompute and store information used during arrangement.
	QMap<int, QList<qpwgraph_node *>> rankNodes;
	QMap<int, float> rankMaxWidth;
	int maxRank = 0;
	foreach (qpwgraph_node *n, inputNodes) {
		int rank = nodeRanks[n];
		if (!rankNodes.contains(rank)) {
			rankNodes[rank] = QList<qpwgraph_node *>();
		}
		rankNodes[rank] << n;

		if (!rankMaxWidth.contains(rank)) {
			rankMaxWidth[rank] = 0;
		}
		rankMaxWidth[rank] = qMax(rankMaxWidth[rank], n->boundingRect().width());

		maxRank = qMax(maxRank, rank);
	}

	// Place nodes based on topological sort
	// TODO: expand graph to fill width of window if it's smaller?
	const qreal xmin = 20, ymin = 20;
	const qreal xpad = 120;
	const qreal ypad = 40;
	qreal x = xmin, y = ymin;
	int current_rank = nodeRanks[inputNodes.first()];
	foreach (qpwgraph_node *node, inputNodes) {
		if (nodeRanks[node] != current_rank) {
			// End of a rank; reset Y and move to the next column
			y = ymin;

			x += rankMaxWidth[current_rank] + xpad;
			current_rank = nodeRanks[node];
		}

		double w = node->boundingRect().width();
		if (current_rank == 0) {
			// Right-align sources
			newPositions[node] = QPointF(x + (rankMaxWidth[current_rank] - w), y);
		} else if (current_rank == maxRank) {
			// Left-align sinks
			newPositions[node] = QPointF(x, y);
		} else {
			// Center-align everything else
			newPositions[node] = QPointF(x + (rankMaxWidth[current_rank] - w) / 2, y);
		}

		y += node->boundingRect().height() + ypad;
	}

	// Move columns vertically for shorter and neater wires
	for (int i = 1; i <= maxRank; i++) {
		if (!rankNodes.contains(i)) {
			continue;
		}

		// Sort each column by average connection source port Y value
		// to place nodes vertically closer to their upstream nodes.
		sortNodesByConnectionY(rankNodes[i]);

		qreal minY = newPositions[rankNodes[i].first()].y();
		foreach (qpwgraph_node *n, rankNodes[i]) {
			minY = qMin(minY, newPositions[n].y());
		}

		qreal y = minY;
		foreach (qpwgraph_node *n, rankNodes[i]) {
			newPositions[n] = QPointF(newPositions[n].x(), y);
			y += n->boundingRect().height() + ypad;
		}

		// Center each column's average input port Y value on the source ports' average Y value
		qreal parentY = meanParentPortY(rankNodes[i], newPositions);
		qreal inputY = meanInputPortY(rankNodes[i], newPositions);
		qreal delta = inputY - parentY;
		if (std::isfinite(delta)) {
			foreach (qpwgraph_node *n, rankNodes[i]) {
				QPointF &pos = newPositions[n];
				newPositions[n] = QPointF(pos.x(), pos.y() - delta);
			}
		}
	}

	return newPositions;
}

// Assigns ranks to and sorts the nodes given to the constructor.  Sort occurs
// in-place in the inputNodes member variable.
void qpwgraph_toposort::rankAndSort()
{
	foreach (qpwgraph_node *n, inputNodes) {
		if (nodeIsTrueSource(n)) {
			nodeRanks[n] = 0;
		} else if (nodeIsTrueSink(n)) {
			nodeRanks[n] = 2;
		} else {
			nodeRanks[n] = 1;
		}
	}

	// Sort before for best cycle-breaking heuristics
	sortNodesByRank(inputNodes);
	unvisitedNodes.clear();
	unvisitedNodes << inputNodes;

	// Visit sources
	foreach (qpwgraph_node *n, inputNodes) {
		if (nodeIsEffectiveSource(n)) {
			visitNode(QSet<qpwgraph_node *>(), n);
		}
	}

	// Break cycles
	while (!unvisitedNodes.empty()) {
		qsizetype initialCount = unvisitedNodes.size();

		qpwgraph_node *n = unvisitedNodes.first();
		visitNode(QSet<qpwgraph_node *>(), n);

		if (initialCount == unvisitedNodes.size()) {
			// This is a bug but we don't want an infinite loop
			break;
		}
	}

	// Place sink nodes at final rank
	int sinkDepth = 2;
	foreach (qpwgraph_node *n, inputNodes) {
		if (nodeIsTrueSink(n)) {
			sinkDepth = qMax(sinkDepth, nodeRanks[n]);
		} else {
			sinkDepth = qMax(sinkDepth, nodeRanks[n] + 1);
		}
	}
	foreach (qpwgraph_node *n, inputNodes) {
		if (nodeIsTrueSink(n)) {
			nodeRanks[n] = qMax(nodeRanks[n], sinkDepth);
		}
	}

	// Sort again with computed ranks
	sortNodesByRank(inputNodes);
}

// Depth-first iteration of all children of the given node to assign ranks.
void qpwgraph_toposort::visitNode(QSet<qpwgraph_node *> path, qpwgraph_node *n)
{
	unvisitedNodes.removeOne(n);

	auto newPath = QSet<qpwgraph_node *>(path);
	newPath += n;

	foreach (qpwgraph_node *next, childNodes(n)) {
		if (newPath.contains(next)) {
			// Cycle detected; skip this child node
			continue;
		}

		int newDepth = qMax(nodeRanks[n] + 1, nodeRanks[next]);
		nodeRanks[next] = newDepth;

		visitNode(newPath, next);
	}
}

void qpwgraph_toposort::sortNodesByRank(QList<qpwgraph_node *> &nodes)
{
	std::sort(nodes.begin(), nodes.end(), [this](qpwgraph_node *n1, qpwgraph_node *n2) { return compareNodes(n1, n2); });
}

void qpwgraph_toposort::sortNodesByConnectionY(QList<qpwgraph_node *> &nodes)
{
	// TODO: still group ports by type
	std::sort(nodes.begin(), nodes.end(), [this](qpwgraph_node *n1, qpwgraph_node *n2) { return meanParentPortY({n1}, newPositions) < meanParentPortY({n2}, newPositions); });
}

qsizetype qpwgraph_toposort::countInputPorts(qpwgraph_node *n)
{
	return std::count_if(n->ports().begin(), n->ports().end(), [](qpwgraph_port *p) { return p->isInput(); });
}

qsizetype qpwgraph_toposort::countOutputPorts(qpwgraph_node *n)
{
	return std::count_if(n->ports().begin(), n->ports().end(), [](qpwgraph_port *p) { return p->isOutput(); });
}

// Counts connections that do not have the same source and destination node
qsizetype qpwgraph_toposort::countExternalConnections(qpwgraph_node *n, bool input)
{
	qsizetype count = 0;

	foreach (qpwgraph_port *p, n->ports()) {
		if (p->isInput() == input) {
			count += std::count_if(p->connects().begin(), p->connects().end(), [](qpwgraph_connect *c) { return c->port1()->portNode() != c->port2()->portNode(); });
		}
	}

	return count;
}

qsizetype qpwgraph_toposort::countInputConnections(qpwgraph_node *n)
{
	return countExternalConnections(n, true);
}

bool qpwgraph_toposort::nodeIsTrueSource(qpwgraph_node *n)
{
	return countInputPorts(n) == 0;
}

bool qpwgraph_toposort::nodeIsEffectiveSource(qpwgraph_node *n)
{
	return countInputConnections(n) == 0;
}

bool qpwgraph_toposort::nodeIsTrueSink(qpwgraph_node *n)
{
	return countOutputPorts(n) == 0;
}

// Returns all nodes directly connected to output ports of the given node
QSet<qpwgraph_node *> qpwgraph_toposort::childNodes(qpwgraph_node *n)
{
	auto children = QSet<qpwgraph_node *>();

	foreach (qpwgraph_port *p, n->ports()) {
		if (p->isInput()) {
			continue;
		}

		foreach (qpwgraph_connect *c, p->connects()) {
			qpwgraph_node *n1 = c->port1()->portNode();
			qpwgraph_node *n2 = c->port2()->portNode();

			if (n1 == n2) {
				continue;
			}

			if (n1 == n) {
				children << n2;
			} else {
				children << n1;
			}
		}
	}

	return children;
}

QSet<qpwgraph_port *> qpwgraph_toposort::connectedParentPorts(qpwgraph_node *n)
{
	QSet<qpwgraph_port *> portList;

	foreach (qpwgraph_port *p, n->ports()) {
		if (!p->isInput()) {
			continue;
		}

		foreach (qpwgraph_connect *c, p->connects()) {
			qpwgraph_node *n1 = c->port1()->portNode();
			qpwgraph_node *n2 = c->port2()->portNode();
			if (n1 == n2) {
				// self feedback
				continue;
			}

			if (nodeRanks[n1] > nodeRanks[n] || nodeRanks[n2] > nodeRanks[n]) {
				// left-pointing edge in a cycle
				continue;
			}

			if (c->port1()->portNode() == n) {
				portList << c->port2();
			} else {
				portList << c->port1();
			}
		}
	}

	return portList;
}

QSet<qpwgraph_port *> qpwgraph_toposort::connectedInputPorts(qpwgraph_node *n)
{
	QSet<qpwgraph_port *> portList;

	foreach (qpwgraph_port *p, n->ports()) {
		if (!p->isInput()) {
			continue;
		}

		foreach (qpwgraph_connect *c, p->connects()) {
			qpwgraph_node *n1 = c->port1()->portNode();
			qpwgraph_node *n2 = c->port2()->portNode();
			if (n1 == n2) {
				// self feedback
				continue;
			}

			if (nodeRanks[n1] > nodeRanks[n] || nodeRanks[n2] > nodeRanks[n]) {
				// left-pointing edge in a cycle
				continue;
			}

			portList << p;
		}
	}

	return portList;
}

bool qpwgraph_toposort::compareNodes(qpwgraph_node *n1, qpwgraph_node *n2)
{
	if (nodeRanks[n1] < nodeRanks[n2]) {
		return true;
	}
	if (nodeRanks[n2] < nodeRanks[n1]) {
		return false;
	}

	if (n1->ports().empty() && !n2->ports().empty()) {
		return true;
	}
	if (n2->ports().empty() && !n1->ports().empty()) {
		return false;
	}

	if (!(n1->ports().empty() || n2->ports().empty())) {
		if (n1->ports().first()->portType() < n2->ports().first()->portType()) {
			return true;
		}
		if (n2->ports().first()->portType() < n1->ports().first()->portType()) {
			return false;
		}
	}

	if (countInputPorts(n1) < countInputPorts(n2)) {
		return true;
	}
	if (countInputPorts(n2) < countInputPorts(n1)) {
		return false;
	}

	if (countOutputPorts(n1) < countOutputPorts(n2)) {
		return true;
	}
	if (countOutputPorts(n2) < countOutputPorts(n1)) {
		return false;
	}

	if (n1->nodeName() < n2->nodeName()) {
		return true;
	}
	if (n2->nodeName() < n1->nodeName()) {
		return false;
	}

	return false;
}

qreal qpwgraph_toposort::meanPortY(QSet<qpwgraph_port *> ports, QMap<qpwgraph_node *, QPointF> positions)
{
	if (ports.empty()) {
		return -std::numeric_limits<double>::infinity();
	}

	qreal y = 0;
	foreach (qpwgraph_port *p, ports) {
		y += p->pos().y() + positions[p->portNode()].y();
	}

	return y / ports.size();
}

qreal qpwgraph_toposort::meanParentPortY(QList<qpwgraph_node *> nodes, QMap<qpwgraph_node *, QPointF> positions)
{
	auto ports = QSet<qpwgraph_port *>();

	foreach (qpwgraph_node *n, nodes) {
		ports |= connectedParentPorts(n);
	}

	return meanPortY(ports, positions);
}

qreal qpwgraph_toposort::meanInputPortY(QList<qpwgraph_node *> nodes, QMap<qpwgraph_node *, QPointF> positions)
{
	auto ports = QSet<qpwgraph_port *>();

	foreach (qpwgraph_node *n, nodes) {
		ports |= connectedInputPorts(n);
	}

	return meanPortY(ports, positions);
}
