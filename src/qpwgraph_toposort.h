#ifndef __qpwgraph_toposort_h
#define __qpwgraph_toposort_h

#include "qpwgraph_node.h"

#include <string>

#include <QList>
#include <QSet>

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
