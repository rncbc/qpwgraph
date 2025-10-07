// qpwgraph_node.h
//
/****************************************************************************
   Copyright (C) 2021-2025, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qpwgraph_node_h
#define __qpwgraph_node_h

#include "qpwgraph_port.h"

#include <QHash>
#include <QIcon>


// Forward decls.
class QStyleOptionGraphicsItem;


//----------------------------------------------------------------------------
// qpwgraph_node -- Node graphics item.

class qpwgraph_node : public qpwgraph_item
{
public:

	// Constructor.
	qpwgraph_node(uint id, const QString& name, Mode mode, uint type = 0);

	// Destructor..
	~qpwgraph_node();

	// Graphics item type.
	enum { Type = QGraphicsItem::UserType + 1 };

	int type() const { return Type; }

	// Accessors.
	uint nodeId() const;

	void setNodeName(const QString& name);
	QString nodeName() const;

	void setNodeMode(Mode mode);
	Mode nodeMode() const;

	void setNodeType(uint type);
	uint nodeType() const;

	void setNodeIcon(const QIcon& icon);
	const QIcon& nodeIcon() const;

	void setNodeNum(uint num);
	uint nodeNum() const;

	void setNodeLabel(const QString& label);
	const QString& nodeLabel() const;
	QString nodeNameLabel() const;

	void setNodeTitle(const QString& title);
	const QString& nodeTitle() const;

	void setNodePrefix(const QString& prefix);
	const QString& nodePrefix() const;

	void setNodeNameEx(bool name_ex);
	bool isNodeNameEx() const;
	QString nodeNameEx() const;

	// Port-list methods.
	qpwgraph_port *addPort(uint id, const QString& name, Mode mode, int type = 0);

	qpwgraph_port *addInputPort(uint id, const QString& name, int type = 0);
	qpwgraph_port *addOutputPort(uint id, const QString& name, int type = 0);

	void removePort(qpwgraph_port *port);
	void removePorts();

	// Port finder (by id/name, mode and type)
	qpwgraph_port *findPort(uint id, Mode mode, uint type = 0);
	qpwgraph_port *findPort(const QString& name, Mode mode, uint type = 0);

	// Port-list accessor.
	const QList<qpwgraph_port *>& ports() const;

	// Reset port markings, destroy if unmarked.
	void resetPorts();

	// Path/shape updater.
	void updatePath();

	// Node hash key (by id).
	class NodeIdKey : public IdKey
	{
	public:
		// Constructors.
		NodeIdKey(uint id, Mode mode, uint type = 0)
			: IdKey(id, mode, type) {}
		NodeIdKey(qpwgraph_node *node)
			: IdKey(node->nodeId(), node->nodeMode(), node->nodeType()) {}
	};

	typedef QMultiHash<NodeIdKey, qpwgraph_node *> NodeIds;

	// Node hash key (by name).
	class NodeNameKey : public NameKey
	{
	public:
		// Constructors.
		NodeNameKey (const QString& name, Mode mode, uint type = 0)
			: NameKey(name, mode, type) {}
		NodeNameKey(qpwgraph_node *node)
			: NameKey(node->nodeNameEx(), node->nodeMode(), node->nodeType()) {}
	};

	typedef QMultiHash<NodeNameKey, qpwgraph_node *> NodeNames;

	// Rectangular editor extents.
	QRectF editorRect() const;

protected:

	void paint(QPainter *painter,
		const QStyleOptionGraphicsItem *option, QWidget *widget);

	QVariant itemChange(GraphicsItemChange change, const QVariant& value);

private:

	// Instance variables.
	uint    m_id;
	QString m_name;
	Mode    m_mode;
	uint    m_type;

	QIcon   m_icon;
	uint    m_num;
	QString m_label;
	QString m_title;

	QString m_prefix;

	bool m_name_ex;

	QGraphicsPixmapItem *m_pixmap;
	QGraphicsTextItem   *m_text;

	qpwgraph_port::PortIds   m_port_ids;
	qpwgraph_port::PortNames m_port_names;
	QList<qpwgraph_port *>   m_ports;
};


#endif	// __qpwgraph_node_h

// end of qpwgraph_node.h
