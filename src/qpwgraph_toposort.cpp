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
#include <iostream>

qpwgraph_toposort::qpwgraph_toposort(QList<qpwgraph_node *> nodes) :
	inputNodes(nodes), unvisitedNodes(), visitedNodes()
{
}

// Rearrange nodes by type and connection using a topological ordering.
// Nodes with no incoming connections go first, nodes with no outgoing
// connections go last.  Nodes are arranged in columns based on their distance
// in the graph from a source node.
//
// Returns a Map from node pointer to position, without modifying the nodes.
//
// TODO: Allow passing a list of nodes here instead of the constructor?
QMap<qpwgraph_node *, QPointF> qpwgraph_toposort::arrange()
{
	// Sort nodes topologically, using heuristics to break cycles.
	sort();

	// Precompute and store information used during arrangement.
	QMap<qpwgraph_node *, QPointF> newPositions;
	QMap<int, QList<qpwgraph_node *>> rankNodes;
	QMap<int, float> rankMaxWidth;
	qreal xmin = std::numeric_limits<double>::infinity();
	qreal ymin = std::numeric_limits<double>::infinity();
	int maxRank = 0;
	foreach (qpwgraph_node *n, inputNodes) {
		std::cout << "TOPO: " << debugNode(n) << std::endl;

		int rank = n->depth();
		if (!rankNodes.contains(rank)) {
			rankNodes[rank] = QList<qpwgraph_node *>();
		}
		rankNodes[rank] << n;

		if (!rankMaxWidth.contains(rank)) {
			rankMaxWidth[rank] = 0;
		}
		rankMaxWidth[rank] = qMax(rankMaxWidth[rank], n->boundingRect().width());

		maxRank = qMax(maxRank, rank);

		xmin = qMin(xmin, n->pos().x());
		ymin = qMin(ymin, n->pos().y());
	}

	for (int i = 0; i <= maxRank; i++) {
		if (rankMaxWidth.contains(i)) {
			std::cout << "TOPO: rank " << i << ": maxWidth=" << rankMaxWidth[i] << " count=" << rankNodes[i].size() << std::endl;
		}
	}

	// Place nodes based on topological sort
	// TODO: extract spacing values to constants or parameters
	// TODO: never go below 0,0 for xmin/ymin?
	double x = xmin, y = ymin;
	int current_rank = inputNodes.first()->depth();
	const double xpad = 120;
	const double ypad = 40;
	foreach (qpwgraph_node *node, inputNodes) {
		if (node->depth() > current_rank) {
			// End of a rank; reset Y and move to the next column
			std::cout << "TOPO: next rank: from " << current_rank << " to " << node->depth() << std::endl;
			y = ymin;

			// XXX restore if FIXME from qpwgraph_toposort.cpp is fixed (node->depth() - current_rank)
			// XXX int xpad_count = (node->depth() - current_rank)
			int xpad_count = 1; // XXX

			x += rankMaxWidth[current_rank] + xpad_count * xpad;
			current_rank = node->depth();
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
		std::sort(rankNodes[i].begin(), rankNodes[i].end(), [newPositions](qpwgraph_node *n1, qpwgraph_node *n2) { return meanParentPortY({n1}, newPositions) < meanParentPortY({n2}, newPositions); });

		qreal minY = newPositions[rankNodes[i].first()].y();
		foreach (qpwgraph_node *n, rankNodes[i]) {
			minY = qMin(minY, newPositions[n].y());
		}

		qreal y = minY;
		foreach (qpwgraph_node *n, rankNodes[i]) {
			std::cout << "TOPO: COLUMN SORT: " << debugNode(n) << " parent Y is " << meanParentPortY({n}, newPositions) << std::endl;
			std::cout << "TOPO: COLUMN SORT: Setting " << debugNode(n) << " Y from " << newPositions[n].y() << " to " << y << std::endl;
			newPositions[n] = QPointF(newPositions[n].x(), y);
			y += n->boundingRect().height() + ypad;
		}

		// Center each column's average input port Y value on the source ports' average Y value
		qreal parentY = meanParentPortY(rankNodes[i], newPositions);
		qreal inputY = meanInputPortY(rankNodes[i], newPositions);
		qreal delta = inputY - parentY;
		if (std::isfinite(delta)) {
			foreach (qpwgraph_node *n, rankNodes[i]) {
				float newY = newPositions[n].y() - delta;
				std::cout << "TOPO: COLUMN SHIFT: Setting " << debugNode(n) << " Y from " << newPositions[n].y() << " to " << newY << std::endl;
				newPositions[n] = QPointF(newPositions[n].x(), newY);
			}
		}
	}

	return newPositions;
}

qpwgraph_node *qpwgraph_toposort::first()
{
	return inputNodes.first();
}

qpwgraph_node *qpwgraph_toposort::last()
{
	return inputNodes.last();
}

// Assigns ranks to and sorts the nodes given to the constructor.  Sort occurs
// in-place in the inputNodes member variable.
void qpwgraph_toposort::sort()
{
	std::cout << "TOPO: beginning sorting" << std::endl;

	foreach (qpwgraph_node *n, inputNodes) {
		// TODO: move rank storage into this class as a map?
		if (nodeIsTrueSource(n)) {
			n->setDepth(0);
		} else if (nodeIsTrueSink(n)) {
			n->setDepth(2);
		} else {
			n->setDepth(1);
		}
	}

	// Sort before for best cycle-breaking heuristics
	std::sort(inputNodes.begin(), inputNodes.end(), compareNodes);
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
		std::cout << "TOPO: CYCLE: breaking cycle with " << debugNode(n) << std::endl;
		visitNode(QSet<qpwgraph_node *>(), n);

		if (initialCount == unvisitedNodes.size()) {
			std::cerr << "TOPO: \e[1;31mBUG\e[0m: unvisited count did not change during cycle breaking" << std::endl;
			break;
		}
	}

	// Place sink nodes at final rank
	int sinkDepth = 2;
	foreach (qpwgraph_node *n, inputNodes) {
		if (nodeIsTrueSink(n)) {
			sinkDepth = qMax(sinkDepth, n->depth());
		} else {
			sinkDepth = qMax(sinkDepth, n->depth() + 1);
		}
	}
	foreach (qpwgraph_node *n, inputNodes) {
		if (nodeIsTrueSink(n)) {
			std::cout << "TOPO: Setting sink " << debugNode(n) << " depth to " << sinkDepth << std::endl;
			n->setDepth(qMax(n->depth(), sinkDepth));
		}
	}

	// Sort again with rank
	std::sort(inputNodes.begin(), inputNodes.end(), compareNodes);
}

void qpwgraph_toposort::visitNode(QSet<qpwgraph_node *> path, qpwgraph_node *n)
{
	std::cout << "TOPO: visiting " << debugNode(n) << " from path " << debugPath(path) << std::endl;

	unvisitedNodes.removeOne(n);
	visitedNodes << n;

	auto newPath = QSet<qpwgraph_node *>(path);
	newPath += n;

	foreach (qpwgraph_node *next, childNodes(n)) {
		if (newPath.contains(next)) {
			std::cout << "TOPO: CYCLE: already visited " << debugNode(n) << std::endl;
			continue;
		}

		int newDepth = qMax(n->depth() + 1, next->depth());
		std::cout << "TOPO: setting " << debugNode(next) << " depth from " << next->depth() << " to " << newDepth << std::endl;
		next->setDepth(newDepth);

		// FIXME: fixes some issues with cyclic graphs but prevents
		// updating depth in parallel graphs
		//
		// in1 -> a
		// in1 -> b
		// in1 -> c
		// in1 -> d
		// in1 -> e
		// in2 -> e
		// a -> out
		// b -> out
		// c -> out
		// d -> out
		// e -> out
		// a -> b
		// b -> c
		// d -> e
		// e -> c
		// XXX if (!visitedNodes.contains(next)) {
			visitNode(newPath, next);
		// XXX }
	}
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

qsizetype qpwgraph_toposort::countOutputConnections(qpwgraph_node *n)
{
	return countExternalConnections(n, false);
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

			if (n1->depth() > n->depth() || n2->depth() > n->depth()) {
				// left-pointing edge in a cycle
				std::cout << "TOPO: ignoring parent by depth: " << debugConnection(c) << std::endl;
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

			if (n1->depth() > n->depth() || n2->depth() > n->depth()) {
				// left-pointing edge in a cycle
				std::cout << "TOPO: ignoring input by depth: " << debugConnection(c) << std::endl;
				continue;
			}

			portList << p;
		}
	}

	return portList;
}

bool qpwgraph_toposort::compareNodes(qpwgraph_node *n1, qpwgraph_node *n2)
{
	if (n1->depth() < n2->depth()) {
		std::cout << "n1 depth < n2 depth" << std::endl;
		return true;
	}
	if (n2->depth() < n1->depth()) {
		std::cout << "n2 depth < n1 depth" << std::endl;
		return false;
	}

	if (n1->ports().empty() && !n2->ports().empty()) {
		std::cout << "n1 no ports" << std::endl;
		return true;
	}
	if (n2->ports().empty() && !n1->ports().empty()) {
		std::cout << "n2 no ports" << std::endl;
		return false;
	}

	if (!(n1->ports().empty() || n2->ports().empty())) {
		if (n1->ports().first()->portType() < n2->ports().first()->portType()) {
			std::cout << "n1 port type" << std::endl;
			return true;
		}
		if (n2->ports().first()->portType() < n1->ports().first()->portType()) {
			std::cout << "n2 port type" << std::endl;
			return false;
		}
	}

	if (countInputPorts(n1) < countInputPorts(n2)) {
		std::cout << "n1 input port count" << std::endl;
		return true;
	}
	if (countInputPorts(n2) < countInputPorts(n1)) {
		std::cout << "n2 input port count" << std::endl;
		return false;
	}

	if (countOutputPorts(n1) < countOutputPorts(n2)) {
		std::cout << "n1 output port count" << std::endl;
		return true;
	}
	if (countOutputPorts(n2) < countOutputPorts(n1)) {
		std::cout << "n2 output port count" << std::endl;
		return false;
	}

	if (n1->nodeName() < n2->nodeName()) {
		std::cout << "n1 node name" << std::endl;
		return true;
	}
	if (n2->nodeName() < n1->nodeName()) {
		std::cout << "n2 node name" << std::endl;
		return false;
	}

	std::cout << "n1 n2 equal" << std::endl;
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

QString qpwgraph_toposort::modeName(qpwgraph_item::Mode mode)
{
	switch(mode) {
		case qpwgraph_item::Mode::None:
			return "None";

		case qpwgraph_item::Mode::Input:
			return "Input";

		case qpwgraph_item::Mode::Output:
			return "Output";

		case qpwgraph_item::Mode::Duplex:
			return "Duplex";
	}

	return "UNKNOWN/BUG";
}

std::string qpwgraph_toposort::debugNode(qpwgraph_node *n)
{
	int color = (n->nodeId() * 773) % 200 + 20;

	QString s = "\e[38;5;" + QString::number(color) + "m" +
			modeName(n->nodeMode()) + " node d" + QString::number(n->depth()) + " id" +
			QString::number(n->nodeId()) + " / " + n->nodeName() +
			"\e[37m";

	return s.toStdString();
}

std::string qpwgraph_toposort::debugConnection(qpwgraph_connect *c)
{
	qpwgraph_port *fromPort = c->port1();
	qpwgraph_node *fromNode = fromPort->portNode();

	qpwgraph_port *toPort = c->port2();
	qpwgraph_node *toNode = toPort->portNode();

	QString s = "connection " +
		QString::fromStdString(debugNode(fromNode)) + ":" + fromPort->portName().split(':').last() +
		" -> " +
		QString::fromStdString(debugNode(toNode)) + ":" + toPort->portName().split(':').last();

	return s.toStdString();
}

std::string qpwgraph_toposort::debugPath(QSet<qpwgraph_node *> path)
{
	QStringList pathList;

	foreach (qpwgraph_node *n, path) {
		pathList << QString::fromStdString(debugNode(n));
	}

	return pathList.join(" \e[0;1m->\e[0m ").toStdString();
}

std::string qpwgraph_toposort::debugPoint(QPointF p)
{
	return (QString("(") + QString::number(p.x()) + ", " + QString::number(p.y()) + ")").toStdString();
}

std::string qpwgraph_toposort::debugRect(QRectF p)
{
	return QString::asprintf("(%.01f..%.01f, %.01f..%.01f)", p.x(), p.x() + p.width(), p.y(), p.y() + p.height()).toStdString();
}
