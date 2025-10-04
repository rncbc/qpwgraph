// qpwgraph_toposort.h -- Topological sort and related code for graph nodes
/*
Copyright (c) 2025, Mike Bourgeous
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
