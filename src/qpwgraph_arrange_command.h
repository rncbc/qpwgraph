#ifndef __qpwgraph_arrange_command_h
#define __qpwgraph_arrange_command_h

#include "qpwgraph_command.h"

class qpwgraph_arrange_command : public qpwgraph_move_command
{
public:

	qpwgraph_arrange_command(qpwgraph_canvas *canvas, QList<qpwgraph_node *> nodes, QMap<qpwgraph_node *, QPointF> newPositions);

private:

	qpwgraph_canvas *canvas;
	QList<qpwgraph_node *> nodes;
	QMap<qpwgraph_node *, QPointF> newPositions;
};

#endif // __qpwgraph_arrange_command_h
