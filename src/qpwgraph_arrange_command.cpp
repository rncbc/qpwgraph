#include "qpwgraph_arrange_command.h"

qpwgraph_arrange_command::qpwgraph_arrange_command(qpwgraph_canvas *canvas, QList<qpwgraph_node *> nodes, QMap<qpwgraph_node *, QPointF> newPositions) :
	qpwgraph_move_command(canvas, QList<qpwgraph_node *>(), QPointF(), QPointF())
{
	setText(QObject::tr("Arrange %1 Nodes").arg(nodes.size()));

	foreach (qpwgraph_node *n, nodes) {
		if (newPositions.contains(n)) {
			addItem(n, n->pos(), newPositions[n]);
		}
	}
}
