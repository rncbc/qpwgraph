// qpwgraph_canvas.h
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

#ifndef __qpwgraph_canvas_h
#define __qpwgraph_canvas_h

#include <QGraphicsView>

#include "qpwgraph_command.h"

#include <QHash>


// Forward decls.
class QGraphicsScene;

class QRubberBand;
class QUndoStack;
class QSettings;

class QGraphicsProxyWidget;
class QLineEdit;

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

class QGestureEvent;
class QPinchGesture;

class qpwgraph_patchbay;


// Define if cleanup of legacy node names is needed (v0.5.0)...
#undef CONFIG_CLEANUP_NODE_NAMES


//----------------------------------------------------------------------------
// qpwgraph_canvas -- Canvas graphics scene/view.

class qpwgraph_canvas : public QGraphicsView
{
	Q_OBJECT

public:

	// Constructor.
	qpwgraph_canvas(QWidget *parent = nullptr);

	// Destructor.
	~qpwgraph_canvas();

	// Accessors.
	QGraphicsScene *scene() const;
	QUndoStack *commands() const;

	void setSettings(QSettings *settings);
	QSettings *settings() const;

	qpwgraph_patchbay *patchbay() const;

	// Patchbay auto-pin accessors.
	void setPatchbayAutoPin(bool on);
	bool isPatchbayAutoPin() const;

	// Patchbay auto-disconnect accessors.
	void setPatchbayAutoDisconnect(bool on);
	bool isPatchbayAutoDisconnect() const;

	// Patchbay edit-mode accessors.
	void setPatchbayEdit(bool on);
	bool isPatchbayEdit() const;

	void patchbayEdit();

	bool canPatchbayPin() const;
	bool canPatchbayUnpin() const;

	void patchbayPin();
	void patchbayUnpin();

	bool isPatchbayEmpty() const;

	// Canvas methods.
	void addItem(qpwgraph_item *item);
	void removeItem(qpwgraph_item *item);

	// Current item accessor.
	qpwgraph_item *currentItem() const;

	// Connection predicates.
	bool canConnect() const;
	bool canDisconnect() const;

	// Edit predicates.
	bool canRenameItem() const;
	bool canSearchItem() const;

	// Zooming methods.
	void setZoom(qreal zoom);
	qreal zoom() const;

	void setZoomRange(bool zoomrange);
	bool isZoomRange() const;

	// Clean-up all un-marked nodes...
	void resetNodes(uint node_type);
	void clearNodes(uint node_type);

	// Special node finders.
	qpwgraph_node *findNode(
		uint id, qpwgraph_item::Mode mode, uint type = 0) const;

	QList<qpwgraph_node *> findNodes(
		const qpwgraph_node::NodeNameKey& name_key) const;
	QList<qpwgraph_node *> findNodes(
		const QString& name, qpwgraph_item::Mode mode, uint type = 0) const;

	void releaseNode(qpwgraph_node *node);

	// Whether it's in the middle of something...
	bool isBusy() const;

	// Port (dis)connections dispatcher.
	void emitConnectPorts(
		qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect);

	// Port (dis)connections notifiers.
	void emitConnected(qpwgraph_port *port1, qpwgraph_port *port2);
	void emitDisconnected(qpwgraph_port *port1, qpwgraph_port *port2);

	// Rename notifier.
	void emitRenamed(qpwgraph_item *item, const QString& name);

	// Other generic notifier.
	void emitChanged();

	// Graph canvas state methods.
	bool restoreState();
	bool saveState() const;

	// Repel overlapping nodes...
	void setRepelOverlappingNodes(bool on);
	bool isRepelOverlappingNodes() const;
	void repelOverlappingNodes(qpwgraph_node *node,
		qpwgraph_move_command *move_command = nullptr,
		const QPointF& delta = QPointF());
	void repelOverlappingNodesAll(
		qpwgraph_move_command *move_command = nullptr);

	// Graph colors management.
	void setPortTypeColor(uint port_type, const QColor& color);
	const QColor& portTypeColor(uint port_type);
	void updatePortTypeColors(uint port_type = 0);
	void clearPortTypeColors();

	// Clear all selection.
	void clearSelection();

	// Clear all state.
	void clear();

	// Snap into position helper.
	QPointF snapPos(const QPointF& pos) const;

#ifdef CONFIG_CLEANUP_NODE_NAMES
	static bool cleanupNodeName(QString& name);
#endif

	// Search placeholder text accessors.
	void setSearchPlaceholderText(const QString& text);
	QString searchPlaceholderText() const;

	// Filter/hide list management accessors.
	void setFilterNodesEnabled(bool enabled);
	bool isFilterNodesEnabled() const;

	void setFilterNodesList(const QStringList& nodes);
	const QStringList& filterNodesList() const;

	bool isFilterNodes(const QString& node_name) const;

signals:

	// Node factory notifications.
	void added(qpwgraph_node *node);
	void updated(qpwgraph_node *node);
	void removed(qpwgraph_node *node);

	// Port (dis)connection notifications.
	void connected(qpwgraph_port *port1, qpwgraph_port *port2);
	void disconnected(qpwgraph_port *port1, qpwgraph_port *port2);

	void connected(qpwgraph_connect *connect);

	// Generic change notification.
	void changed();

	// Rename notification.
	void renamed(qpwgraph_item *item, const QString& name);

public slots:

	// Dis/connect selected items.
	void connectItems();
	void disconnectItems();

	// Select actions.
	void selectAll();
	void selectNone();
	void selectInvert();

	// Edit actions.
	void renameItem();
	void searchItem();

	// Discrete zooming actions.
	void zoomIn();
	void zoomOut();
	void zoomFit();
	void zoomReset();

	// Update all nodes.
	void updateNodes();

	// Update all connectors.
	void updateConnects();

protected slots:

	// Rename item slots.
	void renameTextChanged(const QString&);
	void renameEditingFinished();

	// Search item slots.
	void searchTextChanged(const QString&);
	void searchEditingFinished();

protected:

	// Item finder (internal).
	qpwgraph_item *itemAt(const QPointF& pos) const;

	// Port (dis)connection commands.
	void connectPorts(
		qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect);

	// Mouse event handlers.
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);

	void wheelEvent(QWheelEvent *event);

	// Keyboard event handler.
	void keyPressEvent(QKeyEvent *event);

	// Gesture event handlers.
	bool event(QEvent *event);
	bool gestureEvent(QGestureEvent *event);
	void pinchGesture(QPinchGesture *pinch);

	// Graph node/port key helpers.
	QString nodeKey(qpwgraph_node *node, int n = 0) const;
	QString portKey(qpwgraph_port *port) const;

	void addNodeKeys(qpwgraph_node *node);
	void removeNodeKeys(qpwgraph_node *node);

	// Zoom in rectangle range.
	void zoomFitRange(const QRectF& range_rect);

	// Graph node/port state methods.
	bool restoreNode(qpwgraph_node *node);
	bool saveNode(qpwgraph_node *node) const;

	bool restorePort(qpwgraph_port *port);
	bool savePort(qpwgraph_port *port) const;

	// Update editors position and size.
	void updateRenameEditor();
	void updateSearchEditor();

	// Bounding margins/limits...
	const QRectF& boundingRect(bool reset = false);
	void boundingPos(QPointF& pos);

	// Start search editor...
	void startSearchEditor(const QString& text = QString());

	void resizeEvent(QResizeEvent *event) override;

#ifdef CONFIG_CLEANUP_NODE_NAMES
	void cleanupNodeNames(const char *group);
#endif

private:

	// Mouse pointer dragging states.
	enum DragState { DragNone = 0, DragStart, DragMove, DragScroll };

	// Instance variables.
	QGraphicsScene   *m_scene;
	DragState         m_state;
	QPointF           m_pos;
	qpwgraph_item    *m_item;
	qpwgraph_connect *m_connect;
	QRubberBand      *m_rubberband;
	qreal             m_zoom;
	bool              m_zoomrange;
	bool              m_gesture;

	qpwgraph_node::NodeIds   m_node_ids;
	qpwgraph_node::NodeNames m_node_names;
	QList<qpwgraph_node *>   m_nodes;

	QUndoStack *m_commands;
	QSettings  *m_settings;

	qpwgraph_patchbay *m_patchbay;
	bool m_patchbay_edit;
	bool m_patchbay_autopin;
	bool m_patchbay_autodisconnect;

	QList<QGraphicsItem *> m_selected;
	int m_selected_nodes;

	bool m_repel_overlapping_nodes;

	// Graph port colors.
	QHash<uint, QColor> m_port_colors;

	// Item renaming stuff.
	qpwgraph_item *m_rename_item;
	QLineEdit     *m_rename_editor;
	int            m_renamed;

	// Original node position (for move command).
	QPointF m_pos1;

	// Allowed auto-scroll margins/limits (for move command).
	QRectF m_rect1;

	// Item search stuff.
	QLineEdit *m_search_editor;

	// Filter/hide list management.
	bool        m_filter_enabled;
	QStringList m_filter_nodes;
};


#endif	// __qpwgraph_canvas_h

// end of qpwgraph_canvas.h
