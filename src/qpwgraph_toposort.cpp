// qpwgraph_toposort.cpp
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

#include "qpwgraph_node.h"
#include "qpwgraph_port.h"
#include "qpwgraph_connect.h"

#include <QMap>


//----------------------------------------------------------------------------
// qpwgraph_toposort -- Topological sort and related code for graph nodes impl.
//

qpwgraph_toposort::qpwgraph_toposort ( const QList<qpwgraph_node *>& nodes )
	: m_inputNodes(nodes)
{
}


// Rearrange nodes by type and connection using a topological ordering.
// Nodes with no incoming connections go first, nodes with no outgoing
// connections go last.  Nodes are arranged in columns based on their distance
// in the graph from a source node.
//
// Returns a Map from node pointer to position, without modifying the nodes.
const QHash<qpwgraph_node *, QPointF>& qpwgraph_toposort::arrange ( const QRectF& viewportRect )
{
	// Sort nodes topologically, using heuristics to break cycles.
	rankAndSort();

	// Precompute and store information used during arrangement.
	QMap<int, QList<qpwgraph_node *>> rankNodes;
	QMap<int, qreal> rankMaxWidth;
	qreal xmin = std::numeric_limits<double>::infinity();
	qreal ymin = std::numeric_limits<double>::infinity();
	int maxRank = 0;
	foreach (qpwgraph_node *n, m_inputNodes) {
		int rank = m_nodeRanks[n];
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

	// Calculate total space required for all columns plus margins, for
	// expanding to fill viewport width.
	qreal totalColumnWidths = 40.0;
	foreach (qreal w, rankMaxWidth) {
		totalColumnWidths += w;
	}

	// Place nodes based on topological sort
	const qreal xpad = qMax(40.0, (viewportRect.width() - totalColumnWidths) / qMax(2, rankMaxWidth.size() - 1));
	const qreal ypad = 20.0;
	qreal x = xmin, y = ymin;
	int current_rank = m_nodeRanks[m_inputNodes.first()];
	foreach (qpwgraph_node *node, m_inputNodes) {
		if (m_nodeRanks[node] != current_rank) {
			// End of a rank; reset Y and move to the next column
			y = ymin;

			x += rankMaxWidth[current_rank] + xpad;
			current_rank = m_nodeRanks[node];
		}

		const qreal w = node->boundingRect().width();
		if (current_rank == 0) {
			// Right-align sources
			m_newPositions[node] = QPointF(x + (rankMaxWidth[current_rank] - w), y);
		} else if (current_rank == maxRank) {
			// Left-align sinks
			m_newPositions[node] = QPointF(x, y);
		} else {
			// Center-align everything else
			m_newPositions[node] = QPointF(x + (rankMaxWidth[current_rank] - w) / 2, y);
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

		qreal minY = m_newPositions[rankNodes[i].first()].y();
		foreach (qpwgraph_node *n, rankNodes[i]) {
			minY = qMin(minY, m_newPositions[n].y());
		}

		qreal y = minY;
		foreach (qpwgraph_node *n, rankNodes[i]) {
			m_newPositions[n] = QPointF(m_newPositions[n].x(), y);
			y += n->boundingRect().height() + ypad;
		}

		// Center each column's average input port Y value on the source ports' average Y value
		qreal parentY = meanParentPortY(rankNodes[i], m_newPositions);
		qreal inputY = meanInputPortY(rankNodes[i], m_newPositions);
		qreal delta = inputY - parentY;
		if (std::isfinite(delta)) {
			foreach (qpwgraph_node *n, rankNodes[i]) {
				QPointF &pos = m_newPositions[n];
				m_newPositions[n] = QPointF(pos.x(), pos.y() - delta);
			}
		}
	}

	return m_newPositions;
}


// Assigns ranks to and sorts the nodes given to the constructor.  Sort occurs
// in-place in the inputNodes member variable.
void qpwgraph_toposort::rankAndSort (void)
{
	foreach (qpwgraph_node *n, m_inputNodes) {
		if (nodeIsTrueSource(n)) {
			m_nodeRanks[n] = 0;
		} else if (nodeIsTrueSink(n)) {
			m_nodeRanks[n] = 2;
		} else {
			m_nodeRanks[n] = 1;
		}
	}

	// Sort before for best cycle-breaking heuristics
	sortNodesByRank(m_inputNodes);
	m_unvisitedNodes.clear();
	m_unvisitedNodes << m_inputNodes;

	// Visit sources
	foreach (qpwgraph_node *n, m_inputNodes) {
		if (nodeIsEffectiveSource(n)) {
			visitNode(QSet<qpwgraph_node *>(), n);
		}
	}

	// Break cycles
	while (!m_unvisitedNodes.empty()) {
		qsizetype initialCount = m_unvisitedNodes.size();

		qpwgraph_node *n = m_unvisitedNodes.first();
		visitNode(QSet<qpwgraph_node *>(), n);

		if (initialCount == m_unvisitedNodes.size()) {
			// This is a bug but we don't want an infinite loop
			break;
		}
	}

	// Place sink nodes at final rank
	int sinkDepth = 2;
	foreach (qpwgraph_node *n, m_inputNodes) {
		if (nodeIsTrueSink(n)) {
			sinkDepth = qMax(sinkDepth, m_nodeRanks[n]);
		} else {
			sinkDepth = qMax(sinkDepth, m_nodeRanks[n] + 1);
		}
	}
	foreach (qpwgraph_node *n, m_inputNodes) {
		if (nodeIsTrueSink(n)) {
			m_nodeRanks[n] = qMax(m_nodeRanks[n], sinkDepth);
		}
	}

	// Sort again with computed ranks
	sortNodesByRank(m_inputNodes);
}


// Depth-first iteration of all children of the given node to assign ranks.
void qpwgraph_toposort::visitNode (
	const QSet<qpwgraph_node *>& path, qpwgraph_node *n )
{
	m_unvisitedNodes.removeOne(n);

	auto newPath = QSet<qpwgraph_node *>(path);
	newPath += n;

	foreach (qpwgraph_node *next, childNodes(n)) {
		if (newPath.contains(next)) {
			// Cycle detected; skip this child node
			continue;
		}

		int newDepth = qMax(m_nodeRanks[n] + 1, m_nodeRanks[next]);
		m_nodeRanks[next] = newDepth;

		visitNode(newPath, next);
	}
}


void qpwgraph_toposort::sortNodesByRank ( QList<qpwgraph_node *>& nodes )
{
	std::sort(nodes.begin(), nodes.end(),
		[this](qpwgraph_node *n1, qpwgraph_node *n2)
			{ return compareNodes(n1, n2); });
}


void qpwgraph_toposort::sortNodesByConnectionY ( QList<qpwgraph_node *>& nodes )
{
	std::sort(nodes.begin(), nodes.end(),
		[this](qpwgraph_node *n1, qpwgraph_node *n2)
			{ return compareNodesByConnectionLocation(n1, n2); });
}


qsizetype qpwgraph_toposort::countInputPorts ( qpwgraph_node *n )
{
	return std::count_if(n->ports().begin(), n->ports().end(),
		[](qpwgraph_port *p) { return p->isInput(); });
}


qsizetype qpwgraph_toposort::countOutputPorts ( qpwgraph_node *n )
{
	return std::count_if(n->ports().begin(), n->ports().end(),
		[](qpwgraph_port *p) { return p->isOutput(); });
}


// Counts connections that do not have the same source and destination node
qsizetype qpwgraph_toposort::countExternalConnections (
	qpwgraph_node *n, bool is_input )
{
	qsizetype count = 0;

	foreach (qpwgraph_port *p, n->ports()) {
		if (p->isInput() && is_input) {
			count += std::count_if(p->connects().begin(), p->connects().end(),
				[](qpwgraph_connect *c)
				{ return c->port1()->portNode() != c->port2()->portNode(); });
		}
	}

	return count;
}


qsizetype qpwgraph_toposort::countInputConnections ( qpwgraph_node *n )
{
	return countExternalConnections(n, true);
}


bool qpwgraph_toposort::nodeIsTrueSource ( qpwgraph_node *n )
{
	return countInputPorts(n) == 0;
}


bool qpwgraph_toposort::nodeIsEffectiveSource ( qpwgraph_node *n )
{
	return countInputConnections(n) == 0;
}


bool qpwgraph_toposort::nodeIsTrueSink ( qpwgraph_node *n )
{
	return countOutputPorts(n) == 0;
}


// Returns all nodes directly connected to output ports of the given node
QSet<qpwgraph_node *> qpwgraph_toposort::childNodes ( qpwgraph_node *n )
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


QSet<qpwgraph_port *> qpwgraph_toposort::connectedParentPorts ( qpwgraph_node *n )
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

			if (m_nodeRanks[n1] > m_nodeRanks[n] || m_nodeRanks[n2] > m_nodeRanks[n]) {
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


QSet<qpwgraph_port *> qpwgraph_toposort::connectedInputPorts ( qpwgraph_node *n )
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

			if (m_nodeRanks[n1] > m_nodeRanks[n] || m_nodeRanks[n2] > m_nodeRanks[n]) {
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
	if (m_nodeRanks[n1] < m_nodeRanks[n2]) {
		return true;
	}
	if (m_nodeRanks[n2] < m_nodeRanks[n1]) {
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


bool qpwgraph_toposort::compareNodesByConnectionLocation (
	qpwgraph_node *n1, qpwgraph_node *n2 )
{
	// Compare port types first so that e.g. MIDI-only nodes still appear above audio nodes
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

	// Compare source port locations
	return meanParentPortY({n1}, m_newPositions) < meanParentPortY({n2}, m_newPositions);
}


qreal qpwgraph_toposort::meanPortY (
	const QSet<qpwgraph_port *>& ports,
	const QHash<qpwgraph_node *, QPointF>& positions )
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


qreal qpwgraph_toposort::meanParentPortY (
	const QList<qpwgraph_node *>& nodes,
	const QHash<qpwgraph_node *, QPointF>& positions )
{
	auto ports = QSet<qpwgraph_port *>();

	foreach (qpwgraph_node *n, nodes) {
		ports |= connectedParentPorts(n);
	}

	return meanPortY(ports, positions);
}


qreal qpwgraph_toposort::meanInputPortY (
	const QList<qpwgraph_node *>& nodes,
	const QHash<qpwgraph_node *, QPointF>& positions )
{
	auto ports = QSet<qpwgraph_port *>();

	foreach (qpwgraph_node *n, nodes) {
		ports |= connectedInputPorts(n);
	}

	return meanPortY(ports, positions);
}


// end of qpwgraph_toposort.cpp
