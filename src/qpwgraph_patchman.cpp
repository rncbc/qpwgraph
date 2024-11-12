// qpwgraph_patchman.cpp
//
/****************************************************************************
   Copyright (C) 2021-2024, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "config.h"

#include "qpwgraph_patchman.h"

#include "qpwgraph_canvas.h"
#include "qpwgraph_pipewire.h"
#include "qpwgraph_alsamidi.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QSplitter>
#include <QTreeWidget>
#include <QHeaderView>
#include <QScrollBar>
#include <QPushButton>

#include <QDialogButtonBox>

#include <QMessageBox>

#include <QPainter>
#include <QPainterPath>


//----------------------------------------------------------------------------
// qpwgraph_patchman::TreeWidget -- side-view tree widget decl.

class qpwgraph_patchman::TreeWidget : public QTreeWidget
{
public:

	// Side mode type.
	enum Mode { Outputs, Inputs };

	// Constructor.
	TreeWidget(MainWidget *parent, Mode mode);
	// Destructor.
	~TreeWidget();

	// Side mode accessors.
	bool isOutputs() const;
	bool isInputs() const;

	// Node/port finders.
	QTreeWidgetItem *findNodeItem(
		const QString& node_name, int node_type) const;
	QTreeWidgetItem *findPortItem(QTreeWidgetItem *node_item,
		const QString& port_name, int port_type) const;

protected:

	// Initial size hints.
	QSize sizeHint() const;

private:

	// Instance members.
	MainWidget *m_main;

	Mode m_mode;
};


//----------------------------------------------------------------------------
// qpwgraph_patchman::LineWidget -- middle-view line widget decl.

class qpwgraph_patchman::LineWidget : public QWidget
{
public:

	// Constructor.
	LineWidget(MainWidget *parent);
	// Destructor.
	~LineWidget();

	// Connect line managers.
	void addLine(QTreeWidgetItem *port1_item, QTreeWidgetItem *port2_item);
	void removeLine(QTreeWidgetItem *port1_item, QTreeWidgetItem *port2_item);

	// Connect line finder.
	bool findLine(QTreeWidgetItem *port1_item, QTreeWidgetItem *port2_item) const;

	// Connect-lines cleaner.
	void clear();

	// Connect-line empty status.
	bool isEmpty() const;

protected:

	// Legal client/port item position helper.
	int itemY(QTreeWidgetItem *item) const;

	// Draw one connection line.
	void drawLine(QPainter *painter,
		int x1, int y1, int x2, int y2,
		int h1, int h2,	const QPen& pen) const;

	// Draw connection lines.
	void paintEvent(QPaintEvent *);

	// Initial size hints.
	QSize sizeHint() const;

private:

	// Instance members.
	MainWidget *m_main;

	typedef QMultiHash<QTreeWidgetItem *, QTreeWidgetItem *> Lines;

	Lines m_lines;

	// Connector line color map/persistence.
	static QHash<QString, int> g_colors;
};


//----------------------------------------------------------------------------
// qpwgraph_patchman::MainWidget -- main-view composite widget decl.

class qpwgraph_patchman::MainWidget : public QSplitter
{
public:

	// Constructor.
	MainWidget(qpwgraph_patchman *parent);
	// Destructor.
	~MainWidget();

	// Child widget accessors.
	TreeWidget *outputs() const  { return m_outputs; }
	LineWidget *connects() const { return m_connects; }
	TreeWidget *inputs() const   { return m_inputs; }

	// Patchbay items accessors.
	void setPatchbayItems(const qpwgraph_patchbay::Items& items);
	const qpwgraph_patchbay::Items& patchbayItems() const;

	// Patchbay view refresh.
	void refresh();

	// Patchbay management actions.
	bool canRemove() const;
	void remove();

	bool canRemoveAll() const;
	void removeAll();

	bool canCleanup() const;
	void cleanup();

	// Stabilize item highlights.
	void stabilize();

protected:

	// Patchbay item finder/removal.
	bool findConnect(
		QTreeWidgetItem *port1_item, QTreeWidgetItem *port2_item) const;
	bool removeConnect(
		QTreeWidgetItem *port1_item, QTreeWidgetItem *port2_item);

	void clearPatchbayItems();

	// Initial size hints.
	QSize sizeHint() const;

private:

	// Instance memebers
	qpwgraph_patchman *m_patchman;

	TreeWidget *m_outputs;
	LineWidget *m_connects;
	TreeWidget *m_inputs;

	qpwgraph_patchbay::Items m_items;
};


//----------------------------------------------------------------------------
// qpwgraph_patchman::ItemDelegate -- side-view item delegate decl.
#include <QItemDelegate>

class qpwgraph_patchman::ItemDelegate : public QItemDelegate
{
public:

	// Constructor.
	ItemDelegate(TreeWidget *parent)
		: QItemDelegate(parent), m_tree(parent) {}

protected:

	// Overridden paint method.
	void paint(QPainter *painter,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const
	{
		QStyleOptionViewItem opt = option;
		QTreeWidgetItem *item = m_tree->itemFromIndex(index);
		if (item && item->data(0, Qt::UserRole).toBool()) {
			// opt.palette;
			const QColor& color
				= opt.palette.base().color().value() < 0x7f
				? Qt::cyan : Qt::blue;
			if (opt.state & QStyle::State_Selected)
				opt.palette.setColor(QPalette::HighlightedText, color);
			else
				opt.palette.setColor(QPalette::Text, color);
		}
		QItemDelegate::paint(painter, opt, index);
	}

private:

	TreeWidget *m_tree;
};


//----------------------------------------------------------------------------
// qpwgraph_patchman::TreeWidget -- side-view tree widget impl.

// Constructor.
qpwgraph_patchman::TreeWidget::TreeWidget ( MainWidget *parent, Mode mode )
	: QTreeWidget(parent), m_main(parent), m_mode(mode)
{
	QHeaderView *header = QTreeWidget::header();
	header->setDefaultAlignment(Qt::AlignLeft);
	header->setSectionsMovable(false);
	header->setSectionsClickable(true);
	header->setSortIndicatorShown(true);
	header->setStretchLastSection(true);

	QTreeWidget::setRootIsDecorated(true);
	QTreeWidget::setUniformRowHeights(true);
	QTreeWidget::setAutoScroll(true);
	QTreeWidget::setSelectionMode(
		QAbstractItemView::ExtendedSelection);
	QTreeWidget::setSizePolicy(
		QSizePolicy(
			QSizePolicy::Expanding,
			QSizePolicy::Expanding));
	QTreeWidget::setSortingEnabled(true);
	QTreeWidget::setMinimumWidth(120);
	QTreeWidget::setColumnCount(1);

	QTreeWidget::setItemDelegate(new ItemDelegate(this));

	QString text;
	if (isOutputs())
		text = tr("Nodes / Output Ports");
	else
		text = tr("Nodes / Input Ports");
	QTreeWidget::headerItem()->setText(0, text);
	QTreeWidget::sortItems(0, Qt::AscendingOrder);
	QTreeWidget::setToolTip(text);
}


// Destructor.
qpwgraph_patchman::TreeWidget::~TreeWidget (void)
{
}


// Side mode accessors.
bool qpwgraph_patchman::TreeWidget::isOutputs (void) const
{
	return (m_mode == Outputs);
}

bool qpwgraph_patchman::TreeWidget::isInputs (void) const
{
	return (m_mode == Inputs);
}


// Node/port item finders.
QTreeWidgetItem *qpwgraph_patchman::TreeWidget::findNodeItem (
	const QString& node_name, int node_type ) const
{
	const int nitems
		= QTreeWidget::topLevelItemCount();
	for (int i = 0; i < nitems; ++i) {
		QTreeWidgetItem *node_item = QTreeWidget::topLevelItem(i);
		if (node_item->text(0) == node_name &&
			node_item->type()  == node_type) {
			return node_item;
		}
	}

	return nullptr;
}


QTreeWidgetItem *qpwgraph_patchman::TreeWidget::findPortItem (
	QTreeWidgetItem *node_item, const QString& port_name, int port_type ) const
{
	const int nitems
		= node_item->childCount();
	for (int i = 0; i < nitems; ++i) {
		QTreeWidgetItem *port_item = node_item->child(i);
		if (port_item->text(0) == port_name &&
			port_item->type()  == port_type) {
			return port_item;
		}
	}

	return nullptr;
}


// Initial size hints.
QSize qpwgraph_patchman::TreeWidget::sizeHint (void) const
{
	return QSize(290, 240);
}


//----------------------------------------------------------------------------
// qpwgraph_patchman::LineWidget -- middle-view line widget impl.

// Connector line color map/persistence.
QHash<QString, int> qpwgraph_patchman::LineWidget::g_colors;

// Constructor.
qpwgraph_patchman::LineWidget::LineWidget ( MainWidget *parent )
	: QWidget(parent), m_main(parent)
{
	QWidget::setSizePolicy(
		QSizePolicy(
			QSizePolicy::Expanding,
			QSizePolicy::Expanding));

	QWidget::setMinimumWidth(20);
}


// Destructor.
qpwgraph_patchman::LineWidget::~LineWidget (void)
{
}


// Connect-line managers.
void qpwgraph_patchman::LineWidget::addLine (
	QTreeWidgetItem *port1_item, QTreeWidgetItem *port2_item )
{
	m_lines.insert(port1_item, port2_item);
}


void qpwgraph_patchman::LineWidget::removeLine (
	QTreeWidgetItem *port1_item, QTreeWidgetItem *port2_item )
{
	m_lines.remove(port1_item, port2_item);
}


// Connect line finder.
bool qpwgraph_patchman::LineWidget::findLine (
	QTreeWidgetItem *port1_item, QTreeWidgetItem *port2_item ) const
{
	const QList<QTreeWidgetItem *>& port2_items
		=  m_lines.values(port1_item);
	return port2_items.contains(port2_item);
}


// Connect-lines cleaner.
void qpwgraph_patchman::LineWidget::clear (void)
{
	m_lines.clear();

	QWidget::update();
}


// Connect-line empty status.
bool qpwgraph_patchman::LineWidget::isEmpty (void) const
{
	return m_lines.isEmpty();
}


// Legal client/port item position helper.
int qpwgraph_patchman::LineWidget::itemY ( QTreeWidgetItem *item ) const
{
	QRect rect;
	QTreeWidget *tree_widget = item->treeWidget();
	QTreeWidgetItem *parent_item = item->parent();
	if (parent_item && !parent_item->isExpanded())
		rect = tree_widget->visualItemRect(parent_item);
	else
		rect = tree_widget->visualItemRect(item);
	return rect.top() + rect.height() / 2;
}


// Draw one connection line.
void qpwgraph_patchman::LineWidget::drawLine ( QPainter *painter,
	int x1, int y1, int x2, int y2, int h1, int h2, const QPen& pen ) const
{
	// Set apropriate pen...
	painter->setPen(pen);

	// Account for list view headers.
	y1 += h1;
	y2 += h2;

	// Invisible output ports don't get a connecting dot.
	if (y1 > h1)
		painter->drawLine(x1, y1, x1 + 4, y1);

	// Setup control points
	QPolygon spline(4);
	const int cp = int(float(x2 - x1 - 8) * 0.4f);
	spline.putPoints(0, 4,
		x1 + 4, y1, x1 + 4 + cp, y1,
		x2 - 4 - cp, y2, x2 - 4, y2);
	// The connection line, it self.
	QPainterPath path;
	path.moveTo(spline.at(0));
	path.cubicTo(spline.at(1), spline.at(2), spline.at(3));
	painter->strokePath(path, pen);

	// Invisible input ports don't get a connecting dot.
	if (y2 > h2)
		painter->drawLine(x2 - 4, y2, x2, y2);
}


// Draw connection lines.
void qpwgraph_patchman::LineWidget::paintEvent ( QPaintEvent * )
{
	TreeWidget *outputs = m_main->outputs();
	TreeWidget *inputs = m_main->inputs();

	const int yc = QWidget::pos().y();
	const int yo = outputs->pos().y();
	const int yi = inputs->pos().y();

	QPainter painter(this);
	int x1, y1, h1;
	int x2, y2, h2;
	int rgb[3] = { 0x33, 0x66, 0x99 };

	// Draw all lines anti-aliased...
	painter.setRenderHint(QPainter::Antialiasing);

	// Inline adaptive to darker background themes...
	if (QWidget::palette().window().color().value() < 0x7f)
		for (int i = 0; i < 3; ++i) rgb[i] += 0x33;

	// Almost constants.
	x1 = 0;
	x2 = QWidget::width();
	h1 = (outputs->header())->sizeHint().height();
	h2 = (inputs->header())->sizeHint().height();

	Lines::ConstIterator iter = m_lines.constBegin();
	const Lines::ConstIterator& iter_end = m_lines.constEnd();
	for ( ; iter != iter_end; ++iter) {
		QTreeWidgetItem *port1_item = iter.key();
		QTreeWidgetItem *node1_item = port1_item->parent();
		if (node1_item == nullptr)
			continue;
		// Set new connector color.
		const QString& node1_name = node1_item->text(0);
		int k = g_colors.value(node1_name, -1);
		if (k < 0) {
			k = g_colors.size() + 1;
			g_colors.insert(node1_name, k);
		}
		QPen pen(QColor(rgb[k % 3], rgb[(k / 3) % 3], rgb[(k / 9) % 3]));
		// Get starting connector line coordinates.
		y1 = itemY(port1_item) + (yo - yc);
		const QList<QTreeWidgetItem *>& port2_items
			= m_lines.values(port1_item);
		for (QTreeWidgetItem *port2_item : port2_items) {
			QTreeWidgetItem *node2_item = port2_item->parent();
			if (node2_item == nullptr)
				continue;
			// Obviously, should be a connection
			// from pOPort to pIPort items:
			y2 = itemY(port2_item) + (yi - yc);
			drawLine(&painter, x1, y1, x2, y2, h1, h2, pen);
		}
	}
}


// Initial size hints.
QSize qpwgraph_patchman::LineWidget::sizeHint (void) const
{
	return QSize(60, 240);
}


//----------------------------------------------------------------------------
// qpwgraph_patchman::MainWidget -- main-view composite widget impl.

// Constructor.
qpwgraph_patchman::MainWidget::MainWidget ( qpwgraph_patchman *parent )
	: QSplitter(parent), m_patchman(parent)
{
	m_outputs = new TreeWidget(this, TreeWidget::Outputs);
	m_connects = new LineWidget(this);
	m_inputs = new TreeWidget(this, TreeWidget::Inputs);

	QSplitter::setHandleWidth(2);

	QObject::connect(m_outputs,
		SIGNAL(itemExpanded(QTreeWidgetItem *)),
		m_connects, SLOT(update()));
	QObject::connect(m_outputs,
		SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		m_connects, SLOT(update()));
	QObject::connect(m_outputs->verticalScrollBar(),
		SIGNAL(valueChanged(int)),
		m_connects, SLOT(update()));
	QObject::connect(m_outputs->header(),
		SIGNAL(sectionClicked(int)),
		m_connects, SLOT(update()));

	QObject::connect(m_inputs,
		SIGNAL(itemExpanded(QTreeWidgetItem *)),
		m_connects, SLOT(update()));
	QObject::connect(m_inputs,
		SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		m_connects, SLOT(update()));
	QObject::connect(m_inputs->verticalScrollBar(),
		SIGNAL(valueChanged(int)),
		m_connects, SLOT(update()));
	QObject::connect(m_inputs->header(),
		SIGNAL(sectionClicked(int)),
		m_connects, SLOT(update()));
}


// Destructor.
qpwgraph_patchman::MainWidget::~MainWidget (void)
{
	m_items.clearItems();
}


// Patchbay items accessors.
void qpwgraph_patchman::MainWidget::setPatchbayItems (
	const qpwgraph_patchbay::Items& items )
{
	m_items.copyItems(items);

	refresh();
}


const qpwgraph_patchbay::Items& qpwgraph_patchman::MainWidget::patchbayItems (void) const
{
	return m_items;
}


// Patchbay view refresh.
void qpwgraph_patchman::MainWidget::refresh (void)
{
	qpwgraph_patchbay *patchbay = m_patchman->patchbay();
	if (patchbay == nullptr)
		return;

	m_outputs->clear();
	m_connects->clear();
	m_inputs->clear();

	qpwgraph_patchbay::Items::ConstIterator iter = m_items.constBegin();
	const qpwgraph_patchbay::Items::ConstIterator& iter_end = m_items.constEnd();
	for ( ; iter != iter_end; ++iter) {
		qpwgraph_patchbay::Item *item = iter.value();
		const int node_type = item->node_type;
		QIcon node_icon;
		if (node_type == qpwgraph_pipewire::nodeType())
			node_icon = QIcon(":images/itemPipewire.png");
	#ifdef CONFIG_ALSA_MIDI
		else
		if (node_type == qpwgraph_alsamidi::nodeType())
			node_icon = QIcon(":images/itemAlsamidi.png");
	#endif
		const int port_type = item->port_type;
		QIcon port_icon;
		const QColor& color
			= (patchbay->canvas())->portTypeColor(port_type);
		if (color.isValid()) {
			QPixmap pm(8, 8);
			QPainter(&pm).fillRect(0, 0, pm.width(), pm.height(), color);
			port_icon = QIcon(pm);
		}
		// Output node/port...
		const QString& node1_name = item->node1;
		QTreeWidgetItem *node1_item = m_outputs->findNodeItem(node1_name, node_type);
		if (node1_item == nullptr) {
			node1_item = new QTreeWidgetItem(m_outputs, node_type);
			node1_item->setIcon(0, node_icon);
			node1_item->setText(0, node1_name);
			node1_item->setData(0, Qt::UserRole, false);
		}
		const QString& port1_name = item->port1;
		QTreeWidgetItem *port1_item = m_outputs->findPortItem(node1_item, port1_name, port_type);
		if (port1_item == nullptr) {
			port1_item = new QTreeWidgetItem(node1_item, port_type);
			port1_item->setIcon(0, port_icon);
			port1_item->setText(0, port1_name);
			port1_item->setData(0, Qt::UserRole, false);
		}
		// Input node/port...
		const QString& node2_name = item->node2;
		QTreeWidgetItem *node2_item = m_inputs->findNodeItem(node2_name, node_type);
		if (node2_item == nullptr) {
			node2_item = new QTreeWidgetItem(m_inputs, node_type);
			node2_item->setIcon(0, node_icon);
			node2_item->setText(0, node2_name);
			node2_item->setData(0, Qt::UserRole, false);
		}
		const QString& port2_name = item->port2;
		QTreeWidgetItem *port2_item = m_inputs->findPortItem(node2_item, port2_name, port_type);
		if (port2_item == nullptr) {
			port2_item = new QTreeWidgetItem(node2_item, port_type);
			port2_item->setIcon(0, port_icon);
			port2_item->setText(0, port2_name);
			port2_item->setData(0, Qt::UserRole, false);
		}
		// Connect line...
		m_connects->addLine(port1_item, port2_item);
	}

	m_inputs->expandAll();
	m_outputs->expandAll();
	m_connects->update();
}


// Patchbay management actions.
bool qpwgraph_patchman::MainWidget::canRemove (void) const
{
	const QList<QTreeWidgetItem *>& items1
		= m_outputs->selectedItems();
	const QList<QTreeWidgetItem *>& items2
		= m_inputs->selectedItems();

	if (items1.isEmpty() || items2.isEmpty())
		return false;

	QListIterator<QTreeWidgetItem *> iter1(items1);
	QListIterator<QTreeWidgetItem *> iter2(items2);

	const int nitems
		= qMax(items1.count(),items2.count());

	for (int i = 0; i < nitems; ++i) {
		if (!iter1.hasNext())
			iter1.toFront();
		if (!iter2.hasNext())
			iter2.toFront();
		QTreeWidgetItem *item1 = iter1.next();
		QTreeWidgetItem *item2 = iter2.next();
		if (item2->parent() == nullptr) {
			if (item1->parent() == nullptr) {
				// Each-to-each connections...
				const int nchilds
					= qMin(item1->childCount(), item2->childCount());
				for (int j = 0; j < nchilds; ++j) {
					QTreeWidgetItem *port1_item = item1->child(j);
					QTreeWidgetItem *port2_item = item2->child(j);
					if (findConnect(port1_item, port2_item))
						return true;
				}
			} else {
				// Many(all)-to-one/many connection...
				const int nchilds = item2->childCount();
				for (int j = 0; j < nchilds; ++j) {
					QTreeWidgetItem *port1_item = item1;
					QTreeWidgetItem *port2_item = item2->child(j);
					if (findConnect(port1_item, port2_item))
						return true;
				}
			}
		} else {
			if (item1->parent() == nullptr) {
				// Many(all)-to-one/many connection...
				const int nchilds = item1->childCount();
				for (int j = 0; j < nchilds; ++j) {
					QTreeWidgetItem *port1_item = item1->child(j);
					QTreeWidgetItem *port2_item = item2;
					if (findConnect(port1_item, port2_item))
						return true;
				}
			} else {
				// One-to-many(all) connection...
				QTreeWidgetItem *port1_item = item1;
				QTreeWidgetItem *port2_item = item2;
				if (findConnect(port1_item, port2_item))
					return true;
			}
		}
	}

	return false;
}


void qpwgraph_patchman::MainWidget::remove (void)
{
	int nremoved = 0;

	const QList<QTreeWidgetItem *>& items1
		= m_outputs->selectedItems();
	const QList<QTreeWidgetItem *>& items2
		= m_inputs->selectedItems();

	if (items1.isEmpty() || items2.isEmpty())
		return;

	QListIterator<QTreeWidgetItem *> iter1(items1);
	QListIterator<QTreeWidgetItem *> iter2(items2);

	const int nitems
		= qMax(items1.count(),items2.count());

	for (int i = 0; i < nitems; ++i) {
		if (!iter1.hasNext())
			iter1.toFront();
		if (!iter2.hasNext())
			iter2.toFront();
		QTreeWidgetItem *item1 = iter1.next();
		QTreeWidgetItem *item2 = iter2.next();
		if (item2->parent() == nullptr) {
			if (item1->parent() == nullptr) {
				// Each-to-each connections...
				const int nchilds
					= qMin(item1->childCount(), item2->childCount());
				for (int j = 0; j < nchilds; ++j) {
					QTreeWidgetItem *port1_item = item1->child(j);
					QTreeWidgetItem *port2_item = item2->child(j);
					if (removeConnect(port1_item, port2_item))
						++nremoved;
				}
			} else {
				// Many(all)-to-one/many connection...
				const int nchilds = item2->childCount();
				for (int j = 0; j < nchilds; ++j) {
					QTreeWidgetItem *port1_item = item1;
					QTreeWidgetItem *port2_item = item2->child(j);
					if (removeConnect(port1_item, port2_item))
						++nremoved;
				}
			}
		} else {
			if (item1->parent() == nullptr) {
				// Many(all)-to-one/many connection...
				const int nchilds = item1->childCount();
				for (int j = 0; j < nchilds; ++j) {
					QTreeWidgetItem *port1_item = item1->child(j);
					QTreeWidgetItem *port2_item = item2;
					if (removeConnect(port1_item, port2_item))
						++nremoved;
				}
			} else {
				// One-to-many(all) connection...
				QTreeWidgetItem *port1_item = item1;
				QTreeWidgetItem *port2_item = item2;
					if (removeConnect(port1_item, port2_item))
						++nremoved;
			}
		}
	}

	// Refresh if anything has been removed...
	if (nremoved > 0)
		refresh();
}


bool qpwgraph_patchman::MainWidget::canRemoveAll (void) const
{
	return !m_connects->isEmpty();
}


void qpwgraph_patchman::MainWidget::removeAll (void)
{
	m_items.clearItems();

	refresh();
}


bool qpwgraph_patchman::MainWidget::canCleanup (void) const
{
	qpwgraph_patchbay *patchbay = m_patchman->patchbay();
	if (patchbay == nullptr)
		return false;

	qpwgraph_canvas *canvas = patchbay->canvas();
	if (canvas == nullptr)
		return false;

	qpwgraph_patchbay::Items::ConstIterator iter = m_items.constBegin();
	const qpwgraph_patchbay::Items::ConstIterator& iter_end = m_items.constEnd();
	for ( ; iter != iter_end; ++iter) {
		qpwgraph_patchbay::Item *item = iter.value();
		QList<qpwgraph_node *> nodes1
			= canvas->findNodes(
				item->node1,
				qpwgraph_item::Output,
				item->node_type);
		if (nodes1.isEmpty())
			nodes1 = canvas->findNodes(
				item->node1,
				qpwgraph_item::Duplex,
				item->node_type);
		if (nodes1.isEmpty())
			return true;
		foreach (qpwgraph_node *node1, nodes1) {
			qpwgraph_port *port1
				= node1->findPort(
					item->port1,
					qpwgraph_item::Output,
					item->port_type);
			if (port1 == nullptr)
				return true;
			QList<qpwgraph_node *> nodes2
				= canvas->findNodes(
					item->node2,
					qpwgraph_item::Input,
					item->node_type);
			if (nodes2.isEmpty())
				nodes2 = canvas->findNodes(
					item->node2,
					qpwgraph_item::Duplex,
					item->node_type);
			if (nodes2.isEmpty())
				return true;
			foreach (qpwgraph_node *node2, nodes2) {
				qpwgraph_port *port2
					= node2->findPort(
						item->port2,
						qpwgraph_item::Input,
						item->port_type);
				if (port2 == nullptr)
					return true;
				qpwgraph_connect *connect12 = port1->findConnect(port2);
				if (connect12 == nullptr)
					return true;
				if (!patchbay->findConnect(connect12))
					return true;
				qpwgraph_connect *connect21 = port2->findConnect(port1);
				if (connect21 == nullptr)
					return true;
				if (!patchbay->findConnect(connect21))
					return true;
			}
		}
	}

	return false;
}


void qpwgraph_patchman::MainWidget::cleanup (void)
{
	qpwgraph_patchbay *patchbay = m_patchman->patchbay();
	if (patchbay == nullptr)
		return;

	qpwgraph_canvas *canvas = patchbay->canvas();
	if (canvas == nullptr)
		return;

	QList<qpwgraph_patchbay::Item *> items;

	qpwgraph_patchbay::Items::ConstIterator iter = m_items.constBegin();
	const qpwgraph_patchbay::Items::ConstIterator& iter_end = m_items.constEnd();
	for ( ; iter != iter_end; ++iter) {
		qpwgraph_patchbay::Item *item = iter.value();
		QList<qpwgraph_node *> nodes1
			= canvas->findNodes(
				item->node1,
				qpwgraph_item::Output,
				item->node_type);
		if (nodes1.isEmpty())
			nodes1 = canvas->findNodes(
				item->node1,
				qpwgraph_item::Duplex,
				item->node_type);
		if (nodes1.isEmpty()) {
			items.append(item);
			continue;
		}
		foreach (qpwgraph_node *node1, nodes1) {
			qpwgraph_port *port1
				= node1->findPort(
					item->port1,
					qpwgraph_item::Output,
					item->port_type);
			if (port1 == nullptr) {
				items.append(item);
				continue;
			}
			QList<qpwgraph_node *> nodes2
				= canvas->findNodes(
					item->node2,
					qpwgraph_item::Input,
					item->node_type);
			if (nodes2.isEmpty())
				nodes2 = canvas->findNodes(
					item->node2,
					qpwgraph_item::Duplex,
					item->node_type);
			if (nodes2.isEmpty()) {
				items.append(item);
				continue;
			}
			foreach (qpwgraph_node *node2, nodes2) {
				qpwgraph_port *port2
					= node2->findPort(
						item->port2,
						qpwgraph_item::Input,
						item->port_type);
				if (port2 == nullptr) {
					items.append(item);
					continue;
				}
				qpwgraph_connect *connect12 = port1->findConnect(port2);
				if (connect12 == nullptr) {
					items.append(item);
					continue;
				}
				if (!patchbay->findConnect(connect12)) {
					items.append(item);
					continue;
				}
				qpwgraph_connect *connect21 = port2->findConnect(port1);
				if (connect21 == nullptr) {
					items.append(item);
					continue;
				}
				if (!patchbay->findConnect(connect21)) {
					items.append(item);
					continue;
				}
			}
		}
	}

	if (!items.isEmpty()) {
		QListIterator<qpwgraph_patchbay::Item *> iter2(items);
		while (iter2.hasNext())
			m_items.removeItem(*iter2.next());
		refresh();
	}
}


// Patchbay item finder/removal.
bool qpwgraph_patchman::MainWidget::findConnect (
	QTreeWidgetItem *port1_item, QTreeWidgetItem *port2_item ) const
{
	return m_connects->findLine(port1_item, port2_item);
}


bool qpwgraph_patchman::MainWidget::removeConnect (
	QTreeWidgetItem *port1_item, QTreeWidgetItem *port2_item )
{
	if (!findConnect(port1_item, port2_item))
		return false;

	QTreeWidgetItem *node1_item = port1_item->parent();
	QTreeWidgetItem *node2_item = port2_item->parent();

	if (node1_item == nullptr || node2_item == nullptr)
		return false;

	m_connects->removeLine(port1_item, port2_item);

	return m_items.removeItem(
		qpwgraph_patchbay::Item(
			node1_item->type(),
			port1_item->type(),
			node1_item->text(0),
			port1_item->text(0),
			node2_item->text(0),
			port2_item->text(0)));
}


// Stabilize item highlights.
void qpwgraph_patchman::MainWidget::stabilize (void)
{
	QHash<QTreeWidgetItem *, int> items;

	qpwgraph_patchbay::Items::ConstIterator iter = m_items.constBegin();
	const qpwgraph_patchbay::Items::ConstIterator& iter_end = m_items.constEnd();
	for ( ; iter != iter_end; ++iter) {
		qpwgraph_patchbay::Item *item = iter.value();
		QTreeWidgetItem *node1_item
			= m_outputs->findNodeItem(item->node1, item->node_type);
		if (node1_item == nullptr)
			continue;
		QTreeWidgetItem *node2_item
			= m_inputs->findNodeItem(item->node2, item->node_type);
		if (node2_item == nullptr)
			continue;
		QTreeWidgetItem *port1_item
			= m_outputs->findPortItem(node1_item, item->port1, item->port_type);
		if (port1_item == nullptr)
			continue;
		QTreeWidgetItem *port2_item
			= m_inputs->findPortItem(node2_item, item->port2, item->port_type);
		if (port2_item == nullptr)
			continue;
		const bool hilite1
			= (node2_item->isSelected() || port2_item->isSelected());
		int n = items.value(node1_item, 0);
		if (hilite1)
			++n;
		items.insert(node1_item, n);
		n = items.value(port1_item, 0);
		if (hilite1)
			++n;
		items.insert(port1_item, n);
		const bool hilite2
			= (node1_item->isSelected() || port1_item->isSelected());
		n = items.value(node2_item, 0);
		if (hilite2)
			++n;
		items.insert(node2_item, n);
		n = items.value(port2_item, 0);
		if (hilite2)
			++n;
		items.insert(port2_item, n);
	}

	QHash<QTreeWidgetItem *, int>::ConstIterator items_iter = items.constBegin();
	const QHash<QTreeWidgetItem *, int>::ConstIterator& items_end = items.constEnd();
	for ( ; items_iter != items_end; ++items_iter) {
		QTreeWidgetItem *item = items_iter.key();
		const bool hilite = (items_iter.value() > 0);
		item->setData(0, Qt::UserRole, bool(
			(item->parent() || !item->isExpanded()) && hilite));
	}
}


// Initial size hints.
QSize qpwgraph_patchman::MainWidget::sizeHint (void) const
{
	return QSize(640, 240);
}



//----------------------------------------------------------------------------
// qpwgraph_patchman -- main dialog impl.

// Constructor.
qpwgraph_patchman::qpwgraph_patchman ( QWidget *parent )
	: QDialog(parent), m_patchbay(nullptr), m_dirty(0)
{
	QDialog::setWindowTitle(tr("Manage Patchbay"));

	m_remove_button = new QPushButton("&Remove");
	m_remove_all_button = new QPushButton("Remove &All");
	m_cleanup_button = new QPushButton("Clean&up");
	m_reset_button = new QPushButton("Re&set");

	m_button_box = new QDialogButtonBox();
	m_button_box->setStandardButtons(
		QDialogButtonBox::Ok |
		QDialogButtonBox::Cancel);

	m_main = new MainWidget(this);

	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->setContentsMargins(4, 8, 4, 4);
	hbox->setSpacing(8);
	hbox->addWidget(m_remove_button);
	hbox->addWidget(m_remove_all_button);
	hbox->addStretch();
	hbox->addWidget(m_cleanup_button);
	hbox->addWidget(m_reset_button);
	hbox->addStretch();
	hbox->addWidget(m_button_box);

	QVBoxLayout *vbox = new QVBoxLayout();
	vbox->setContentsMargins(4, 8, 4, 4);
	vbox->setSpacing(4);
	vbox->addWidget(m_main);
	vbox->addLayout(hbox);

	QDialog::setLayout(vbox);

	QObject::connect(m_main->outputs(),
		SIGNAL(itemSelectionChanged()),
		SLOT(stabilize()));
	QObject::connect(m_main->outputs(),
		SIGNAL(itemExpanded(QTreeWidgetItem *)),
		SLOT(stabilize()));
	QObject::connect(m_main->outputs(),
		SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		SLOT(stabilize()));

	QObject::connect(m_main->inputs(),
		SIGNAL(itemSelectionChanged()),
		SLOT(stabilize()));
	QObject::connect(m_main->inputs(),
		SIGNAL(itemExpanded(QTreeWidgetItem *)),
		SLOT(stabilize()));
	QObject::connect(m_main->inputs(),
		SIGNAL(itemCollapsed(QTreeWidgetItem *)),
		SLOT(stabilize()));

	QObject::connect(m_remove_button,
		SIGNAL(clicked()),
		SLOT(removeClicked()));
	QObject::connect(m_remove_all_button,
		SIGNAL(clicked()),
		SLOT(removeAllClicked()));
	QObject::connect(m_cleanup_button,
		SIGNAL(clicked()),
		SLOT(cleanupClicked()));
	QObject::connect(m_reset_button,
		SIGNAL(clicked()),
		SLOT(resetClicked()));

	QObject::connect(m_button_box,
		SIGNAL(accepted()),
		SLOT(accept()));
	QObject::connect(m_button_box,
		SIGNAL(rejected()),
		SLOT(reject()));

	// Ready?
	stabilize();
}


// Patchbay accessors.
void qpwgraph_patchman::setPatchbay ( qpwgraph_patchbay *patchbay )
{
	m_patchbay = patchbay;

	resetClicked();
}


qpwgraph_patchbay *qpwgraph_patchman::patchbay (void) const
{
	return m_patchbay;
}


// Patchbay view refresh.
void qpwgraph_patchman::refresh (void)
{
	m_main->refresh();

	stabilize();
}


// Destructor.
qpwgraph_patchman::~qpwgraph_patchman (void)
{
}


void qpwgraph_patchman::removeClicked (void)
{
	m_main->remove();

	++m_dirty;

	stabilize();
}


void qpwgraph_patchman::removeAllClicked (void)
{
	m_main->removeAll();

	++m_dirty;

	stabilize();
}


void qpwgraph_patchman::cleanupClicked (void)
{
	m_main->cleanup();

	++m_dirty;

	stabilize();
}


void qpwgraph_patchman::resetClicked (void)
{
	if (m_patchbay)
		m_main->setPatchbayItems(m_patchbay->items());

	m_dirty = 0;

	stabilize();
}


void qpwgraph_patchman::accept (void)
{
	if (m_patchbay)
		m_patchbay->setItems(m_main->patchbayItems());

	QDialog::accept();
}


void qpwgraph_patchman::reject (void)
{
	bool ret = true;

	// Check if there's any pending changes...
	if (m_dirty > 0) {
		switch (QMessageBox::warning(this,
			tr("Warning"),
			tr("The current patchbay have been changed.") + "\n\n" +
			tr("Do you want to apply the changes?"),
			QMessageBox::Apply | QMessageBox::Discard | QMessageBox::Cancel)) {
		case QMessageBox::Apply:
			accept();
			return;
		case QMessageBox::Discard:
			break;
		default: // Cancel.
			ret = false;
		}
	}

	if (ret)
		QDialog::reject();
}


// Stabilize current form state.
void qpwgraph_patchman::stabilize (void)
{
	m_main->stabilize();

	m_remove_button->setEnabled(m_main->canRemove());
	m_remove_all_button->setEnabled(m_main->canRemoveAll());
	m_cleanup_button->setEnabled(m_main->canCleanup());
	m_reset_button->setEnabled(m_dirty > 0);

	m_button_box->button(QDialogButtonBox::Ok)->setEnabled(m_dirty > 0);
}


// end of qpwgraph_patchman.cpp
