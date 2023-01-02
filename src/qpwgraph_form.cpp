// qpwgraph_form.cpp
//
/****************************************************************************
   Copyright (C) 2021-2023, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qpwgraph.h"
#include "qpwgraph_form.h"

#include "qpwgraph_config.h"

#include "qpwgraph_pipewire.h"
#include "qpwgraph_alsamidi.h"

#include "qpwgraph_connect.h"

#include "qpwgraph_patchbay.h"
#include "qpwgraph_systray.h"

#include <pipewire/pipewire.h>

#include <QTimer>
#include <QMenu>

#include <QFileInfo>
#include <QFileDialog>

#include <QMessageBox>

#include <QHBoxLayout>
#include <QToolButton>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>

#include <QColorDialog>

#include <QActionGroup>

#include <QResizeEvent>
#include <QCloseEvent>

#include <cmath>


//----------------------------------------------------------------------------
// qpwgraph_zoom_slider -- Custom slider widget.

#include <QMouseEvent>

class qpwgraph_zoom_slider : public QSlider
{
public:

	qpwgraph_zoom_slider() : QSlider(Qt::Horizontal)
	{
		QSlider::setMinimum(10);
		QSlider::setMaximum(190);
		QSlider::setTickInterval(90);
		QSlider::setTickPosition(QSlider::TicksBothSides);
	}

protected:

	void mousePressEvent(QMouseEvent *ev)
	{
		QSlider::mousePressEvent(ev);

		if (ev->button() == Qt::MiddleButton)
			QSlider::setValue(100);
	}
};


//----------------------------------------------------------------------------
// qpwgraph_form -- UI wrapper form.

// Constructor.
qpwgraph_form::qpwgraph_form (
	QWidget *parent, Qt::WindowFlags wflags )
	: QMainWindow(parent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(6, 1, 0)
	QMainWindow::setWindowIcon(QIcon(":/images/qpwgraph.png"));
#endif
	m_config = new qpwgraph_config("rncbc.org", "qpwgraph");

	m_ui.graphCanvas->setSettings(m_config->settings());

	m_pipewire = new qpwgraph_pipewire(m_ui.graphCanvas);
#ifdef CONFIG_ALSA_MIDI
	m_alsamidi = new qpwgraph_alsamidi(m_ui.graphCanvas);
#else
	m_alsamidi = nullptr;
#endif

	m_pipewire_changed = 0;
	m_alsamidi_changed = 0;

	m_ins = m_mids = m_outs = 0;

	m_repel_overlapping_nodes = 0;

	m_patchbay_names = new QComboBox(m_ui.patchbayToolbar);
	m_patchbay_names->setEditable(false);
	m_patchbay_names->setMinimumWidth(120);
	m_patchbay_names->setMaximumWidth(240);
	m_patchbay_names_tool = m_ui.patchbayToolbar->insertWidget(
		m_ui.patchbaySaveAction, m_patchbay_names);

	m_systray = nullptr;
	m_systray_closed = false;

	QUndoStack *commands = m_ui.graphCanvas->commands();

	QAction *undo_action = commands->createUndoAction(this, tr("&Undo"));
	undo_action->setIcon(QIcon(":/images/editUndo.png"));
	undo_action->setStatusTip(tr("Undo last edit action"));
	undo_action->setShortcuts(QKeySequence::Undo);

	QAction *redo_action = commands->createRedoAction(this, tr("&Redo"));
	redo_action->setIcon(QIcon(":/images/editRedo.png"));
	redo_action->setStatusTip(tr("Redo last edit action"));
	redo_action->setShortcuts(QKeySequence::Redo);

	QAction *before = m_ui.editSelectAllAction;
	m_ui.editMenu->insertAction(before, undo_action);
	m_ui.editMenu->insertAction(before, redo_action);
	m_ui.editMenu->insertSeparator(before);

	before = m_ui.viewCenterAction;
	m_ui.graphToolbar->insertAction(before, undo_action);
	m_ui.graphToolbar->insertAction(before, redo_action);
	m_ui.graphToolbar->insertSeparator(before);

	// Special zoom composite widget...
	QWidget *zoom_widget = new QWidget();
	zoom_widget->setMaximumWidth(240);
	zoom_widget->setToolTip(tr("Zoom"));

	QHBoxLayout *zoom_layout = new QHBoxLayout();
	zoom_layout->setContentsMargins(0, 0, 0, 0);
	zoom_layout->setSpacing(2);

	QToolButton *zoom_out = new QToolButton();
	zoom_out->setDefaultAction(m_ui.viewZoomOutAction);
	zoom_out->setFixedSize(22, 22);
	zoom_layout->addWidget(zoom_out);

	m_zoom_slider = new qpwgraph_zoom_slider();
	m_zoom_slider->setFixedHeight(22);
	zoom_layout->addWidget(m_zoom_slider);

	QToolButton *zoom_in = new QToolButton();
	zoom_in->setDefaultAction(m_ui.viewZoomInAction);
	zoom_in->setFixedSize(22, 22);
	zoom_layout->addWidget(zoom_in);

	m_zoom_spinbox = new QSpinBox();
	m_zoom_spinbox->setFixedHeight(22);
	m_zoom_spinbox->setAlignment(Qt::AlignCenter);
	m_zoom_spinbox->setMinimum(10);
	m_zoom_spinbox->setMaximum(200);
	m_zoom_spinbox->setSuffix(" %");
	zoom_layout->addWidget(m_zoom_spinbox);

	zoom_widget->setLayout(zoom_layout);
	m_ui.StatusBar->addPermanentWidget(zoom_widget);

	QObject::connect(m_patchbay_names,
		SIGNAL(activated(int)),
		SLOT(patchbayNameChanged(int)));

	QObject::connect(m_zoom_spinbox,
		SIGNAL(valueChanged(int)),
		SLOT(zoomValueChanged(int)));
	QObject::connect(m_zoom_slider,
		SIGNAL(valueChanged(int)),
		SLOT(zoomValueChanged(int)));

	if (m_pipewire) {
		QObject::connect(m_pipewire,
			SIGNAL(changed()),
			SLOT(pipewire_changed()));
	}
#ifdef CONFIG_ALSA_MIDI
	if (m_alsamidi) {
		QObject::connect(m_alsamidi,
			SIGNAL(changed()),
			SLOT(alsamidi_changed()));
	}
#endif
	QObject::connect(m_ui.graphCanvas,
		SIGNAL(added(qpwgraph_node *)),
		SLOT(added(qpwgraph_node *)));
	QObject::connect(m_ui.graphCanvas,
		SIGNAL(updated(qpwgraph_node *)),
		SLOT(updated(qpwgraph_node *)));
	QObject::connect(m_ui.graphCanvas,
		SIGNAL(removed(qpwgraph_node *)),
		SLOT(removed(qpwgraph_node *)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(connected(qpwgraph_port *, qpwgraph_port *)),
		SLOT(connected(qpwgraph_port *, qpwgraph_port *)));
	QObject::connect(m_ui.graphCanvas,
		SIGNAL(disconnected(qpwgraph_port *, qpwgraph_port *)),
		SLOT(disconnected(qpwgraph_port *, qpwgraph_port *)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(connected(qpwgraph_connect *)),
		SLOT(connected(qpwgraph_connect *)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(renamed(qpwgraph_item *, const QString&)),
		SLOT(renamed(qpwgraph_item *, const QString&)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(changed()),
		SLOT(stabilize()));

	// Some actions surely need those
	// shortcuts firmly attached...
	addAction(m_ui.viewMenubarAction);

	// HACK: Make old Ins/Del standard shortcuts
	// for connect/disconnect available again...
	QList<QKeySequence> shortcuts;
	shortcuts.append(m_ui.graphConnectAction->shortcut());
	shortcuts.append(QKeySequence("Ins"));
	m_ui.graphConnectAction->setShortcuts(shortcuts);
	shortcuts.clear();
	shortcuts.append(m_ui.graphDisconnectAction->shortcut());
	shortcuts.append(QKeySequence("Del"));
	m_ui.graphDisconnectAction->setShortcuts(shortcuts);

	QObject::connect(m_ui.graphConnectAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(connectItems()));
	QObject::connect(m_ui.graphDisconnectAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(disconnectItems()));

	QObject::connect(m_ui.patchbayMenu,
		 SIGNAL(aboutToShow()),
		 SLOT(updatePatchbayMenu()));

	QObject::connect(m_ui.patchbayNewAction,
		SIGNAL(triggered(bool)),
		SLOT(patchbayNew()));
	QObject::connect(m_ui.patchbayOpenAction,
		SIGNAL(triggered(bool)),
		SLOT(patchbayOpen()));
	QObject::connect(m_ui.patchbaySaveAction,
		SIGNAL(triggered(bool)),
		SLOT(patchbaySave()));
	QObject::connect(m_ui.patchbaySaveAsAction,
		SIGNAL(triggered(bool)),
		SLOT(patchbaySaveAs()));

	QObject::connect(m_ui.patchbayActivatedAction,
		SIGNAL(toggled(bool)),
		SLOT(patchbayActivated(bool)));
	QObject::connect(m_ui.patchbayExclusiveAction,
		SIGNAL(toggled(bool)),
		SLOT(patchbayExclusive(bool)));

	QObject::connect(m_ui.patchbayAutoPinAction,
		SIGNAL(toggled(bool)),
		SLOT(patchbayAutoPin(bool)));

	QObject::connect(m_ui.patchbayEditAction,
		SIGNAL(toggled(bool)),
		SLOT(patchbayEdit(bool)));
	QObject::connect(m_ui.patchbayPinAction,
		SIGNAL(triggered(bool)),
		SLOT(patchbayPin()));
	QObject::connect(m_ui.patchbayUnpinAction,
		SIGNAL(triggered(bool)),
		SLOT(patchbayUnpin()));

	QObject::connect(m_ui.graphQuitAction,
		SIGNAL(triggered(bool)),
		SLOT(closeQuit()));

	QObject::connect(m_ui.editSelectAllAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(selectAll()));
	QObject::connect(m_ui.editSelectNoneAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(selectNone()));
	QObject::connect(m_ui.editSelectInvertAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(selectInvert()));

	QObject::connect(m_ui.editRenameItemAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(renameItem()));

	QObject::connect(m_ui.viewMenubarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewMenubar(bool)));
	QObject::connect(m_ui.viewStatusbarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewStatusbar(bool)));
	QObject::connect(m_ui.viewGraphToolbarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewGraphToolbar(bool)));
	QObject::connect(m_ui.viewPatchbayToolbarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewPatchbayToolbar(bool)));

	QObject::connect(m_ui.viewTextBesideIconsAction,
		SIGNAL(triggered(bool)),
		SLOT(viewTextBesideIcons(bool)));

	QObject::connect(m_ui.viewCenterAction,
		SIGNAL(triggered(bool)),
		SLOT(viewCenter()));
	QObject::connect(m_ui.viewRefreshAction,
		SIGNAL(triggered(bool)),
		SLOT(viewRefresh()));

	QObject::connect(m_ui.viewZoomInAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(zoomIn()));
	QObject::connect(m_ui.viewZoomOutAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(zoomOut()));
	QObject::connect(m_ui.viewZoomFitAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(zoomFit()));
	QObject::connect(m_ui.viewZoomResetAction,
		SIGNAL(triggered(bool)),
		m_ui.graphCanvas, SLOT(zoomReset()));

	QObject::connect(m_ui.viewZoomRangeAction,
		SIGNAL(triggered(bool)),
		SLOT(viewZoomRange(bool)));

	QObject::connect(m_ui.viewRepelOverlappingNodesAction,
		SIGNAL(triggered(bool)),
		SLOT(viewRepelOverlappingNodes(bool)));
	QObject::connect(m_ui.viewConnectThroughNodesAction,
		SIGNAL(triggered(bool)),
		SLOT(viewConnectThroughNodes(bool)));

	m_ui.viewColorsPipewireAudioAction->setData(qpwgraph_pipewire::audioPortType());
	m_ui.viewColorsPipewireMidiAction->setData(qpwgraph_pipewire::midiPortType());
	m_ui.viewColorsPipewireVideoAction->setData(qpwgraph_pipewire::videoPortType());
	m_ui.viewColorsPipewireOtherAction->setData(qpwgraph_pipewire::otherPortType());
#ifdef CONFIG_ALSA_MIDI
	m_ui.viewColorsAlsaMidiAction->setData(qpwgraph_alsamidi::midiPortType());
	m_ui.viewColorsMenu->insertAction(
		m_ui.viewColorsResetAction, m_ui.viewColorsAlsaMidiAction);
	m_ui.viewColorsMenu->insertSeparator(
		m_ui.viewColorsResetAction);
#endif

	QObject::connect(m_ui.viewColorsPipewireAudioAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsAction()));
	QObject::connect(m_ui.viewColorsPipewireMidiAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsAction()));
	QObject::connect(m_ui.viewColorsPipewireVideoAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsAction()));
	QObject::connect(m_ui.viewColorsPipewireOtherAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsAction()));
#ifdef CONFIG_ALSA_MIDI
	QObject::connect(m_ui.viewColorsAlsaMidiAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsAction()));
#endif
	QObject::connect(m_ui.viewColorsResetAction,
		SIGNAL(triggered(bool)),
		SLOT(viewColorsReset()));

	m_sort_type = new QActionGroup(this);
	m_sort_type->setExclusive(true);
	m_sort_type->addAction(m_ui.viewSortPortNameAction);
	m_sort_type->addAction(m_ui.viewSortPortTitleAction);
	m_sort_type->addAction(m_ui.viewSortPortIndexAction);

	m_ui.viewSortPortNameAction->setData(qpwgraph_port::PortName);
	m_ui.viewSortPortTitleAction->setData(qpwgraph_port::PortTitle);
	m_ui.viewSortPortIndexAction->setData(qpwgraph_port::PortIndex);

	QObject::connect(m_ui.viewSortPortNameAction,
		SIGNAL(triggered(bool)),
		SLOT(viewSortTypeAction()));
	QObject::connect(m_ui.viewSortPortTitleAction,
		SIGNAL(triggered(bool)),
		SLOT(viewSortTypeAction()));
	QObject::connect(m_ui.viewSortPortIndexAction,
		SIGNAL(triggered(bool)),
		SLOT(viewSortTypeAction()));

	m_sort_order = new QActionGroup(this);
	m_sort_order->setExclusive(true);
	m_sort_order->addAction(m_ui.viewSortAscendingAction);
	m_sort_order->addAction(m_ui.viewSortDescendingAction);

	m_ui.viewSortAscendingAction->setData(qpwgraph_port::Ascending);
	m_ui.viewSortDescendingAction->setData(qpwgraph_port::Descending);

	QObject::connect(m_ui.viewSortAscendingAction,
		SIGNAL(triggered(bool)),
		SLOT(viewSortOrderAction()));
	QObject::connect(m_ui.viewSortDescendingAction,
		SIGNAL(triggered(bool)),
		SLOT(viewSortOrderAction()));

#ifdef CONFIG_SYSTEM_TRAY
	m_ui.helpMenu->insertAction(
		m_ui.helpAboutAction, m_ui.helpSystemTrayAction);
	m_ui.helpMenu->insertSeparator(
		m_ui.helpAboutAction);
	QObject::connect(m_ui.helpSystemTrayAction,
		SIGNAL(triggered(bool)),
		SLOT(helpSystemTray(bool)));
#endif

	QObject::connect(m_ui.helpAboutAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAbout()));
	QObject::connect(m_ui.helpAboutQtAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAboutQt()));

	QObject::connect(m_ui.graphToolbar,
		SIGNAL(orientationChanged(Qt::Orientation)),
		SLOT(orientationChanged(Qt::Orientation)));
	QObject::connect(m_ui.patchbayToolbar,
		SIGNAL(orientationChanged(Qt::Orientation)),
		SLOT(orientationChanged(Qt::Orientation)));

	restoreState();

	updatePatchbayMenu();
	updateViewColors();

	// Restore last open patchbay file...
	m_patchbay_untitled = 0;

	const QString path(m_patchbay_path);
	if (!path.isEmpty() && patchbayOpenFile(path)) {
		--m_patchbay_untitled;
	} else {
		qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
		if (patchbay)
			patchbay->snap(); // Simulate patchbayNew()!
	}

	updatePatchbayNames();

	// Make it ready :-)
	m_ui.StatusBar->showMessage(tr("Ready"), 3000);

	// Trigger refresh cycle...
	pipewire_changed();
	alsamidi_changed();

	QTimer::singleShot(300, this, SLOT(refresh()));
}


// Destructor.
qpwgraph_form::~qpwgraph_form (void)
{
#ifdef CONFIG_SYSTEM_TRAY
	if (m_systray)
		delete m_systray;
#endif

//	delete m_patchbay_names;

	delete m_sort_order;
	delete m_sort_type;

	if (m_pipewire)
		delete m_pipewire;
#ifdef CONFIG_ALSA_MIDI
	if (m_alsamidi)
		delete m_alsamidi;
#endif

	delete m_config;
}


// Take care of command line options and arguments...
void qpwgraph_form::apply_args ( qpwgraph_application *app )
{
	if (app->isPatchbayActivated())
		m_ui.patchbayActivatedAction->setChecked(true);
	if (app->isPatchbayExclusive())
		m_ui.patchbayExclusiveAction->setChecked(true);
	if (!app->patchbayPath().isEmpty())
		patchbayOpenFile(app->patchbayPath());

	updatePatchbayNames();

	if (app->isStartMinimized())
	#ifdef CONFIG_SYSTEM_TRAY
		hide();
	#else
		showMinimized();
	#endif
}


// Patchbay menu slots.
void qpwgraph_form::patchbayNew (void)
{
	if (!patchbayQueryClose())
		return;

	qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
	if (patchbay) {
		patchbay->clear();
		patchbay->snap();
	}

	m_patchbay_path.clear();
	++m_patchbay_untitled;

	m_ui.graphCanvas->patchbayEdit();

	updatePatchbayNames();
}


void qpwgraph_form::patchbayOpen (void)
{
	if (!patchbayQueryClose())
		return;

	const QString& path
		= QFileDialog::getOpenFileName(this,
			tr("Open Patchbay File"),
			patchbayFileDir(),
			patchbayFileFilter());

	if (path.isEmpty())
		return;

	patchbayOpenFile(path);

	updatePatchbayNames();
}


void qpwgraph_form::patchbayOpenRecent (void)
{
	// Retrive filename index from action data...
	QAction *action = qobject_cast<QAction *> (sender());
	if (action) {
		const QString& path = action->data().toString();
		// Check if we can safely close the current file...
		if (!path.isEmpty() && patchbayQueryClose())
			patchbayOpenFile(path);
	}

	updatePatchbayNames();
}


void qpwgraph_form::patchbaySave (void)
{
	if (m_patchbay_path.isEmpty()) {
		patchbaySaveAs();
		return;
	}

	patchbaySaveFile(m_patchbay_path);

	updatePatchbayNames();
}


void qpwgraph_form::patchbaySaveAs (void)
{
	const QString& path
		= QFileDialog::getSaveFileName(this,
			tr("Save Patchbay File"),
			patchbayFileDir(),
			patchbayFileFilter());

	if (path.isEmpty())
		return;

	if (QFileInfo(path).suffix().isEmpty())
		patchbaySaveFile(path + '.' + patchbayFileExt());
	else
		patchbaySaveFile(path);

	updatePatchbayNames();
}


void qpwgraph_form::patchbayActivated ( bool on )
{
	qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
	if (patchbay) {
		patchbay->setActivated(on);
		if (on)
			patchbay->scan();
	}

	stabilize();
}


void qpwgraph_form::patchbayExclusive ( bool on )
{
	qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
	if (patchbay) {
		patchbay->setExclusive(on);
		if (on)
			patchbay->scan();
	}

	stabilize();
}


void qpwgraph_form::patchbayEdit ( bool on )
{
	m_ui.graphCanvas->setPatchbayEdit(on);

	stabilize();
}


void qpwgraph_form::patchbayPin (void)
{
	m_ui.graphCanvas->patchbayPin();

	stabilize();
}


void qpwgraph_form::patchbayUnpin (void)
{
	m_ui.graphCanvas->patchbayUnpin();

	stabilize();
}


void qpwgraph_form::patchbayAutoPin ( bool on )
{
	m_ui.graphCanvas->setPatchbayAutoPin(on);

	stabilize();
}


// Main menu slots.
void qpwgraph_form::viewMenubar ( bool on )
{
	m_ui.MenuBar->setVisible(on);
}


void qpwgraph_form::viewGraphToolbar ( bool on )
{
	m_ui.graphToolbar->setVisible(on);
}


void qpwgraph_form::viewPatchbayToolbar ( bool on )
{
	m_ui.patchbayToolbar->setVisible(on);
}


void qpwgraph_form::viewStatusbar ( bool on )
{
	m_ui.StatusBar->setVisible(on);
}


void qpwgraph_form::viewTextBesideIcons ( bool on )
{
	if (on) {
		m_ui.graphToolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		m_ui.patchbayToolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	} else {
		m_ui.graphToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
		m_ui.patchbayToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	}
}


void qpwgraph_form::viewCenter (void)
{
	const QRectF& scene_rect
		= m_ui.graphCanvas->scene()->itemsBoundingRect();
	m_ui.graphCanvas->centerOn(scene_rect.center());

	stabilize();
}


void qpwgraph_form::viewRefresh (void)
{
	pipewire_changed();
	alsamidi_changed();

	if (m_ui.graphCanvas->isRepelOverlappingNodes())
		++m_repel_overlapping_nodes; // fake nodes added!

	refresh();
}


void qpwgraph_form::viewZoomRange ( bool on )
{
	m_ui.graphCanvas->setZoomRange(on);
}


void qpwgraph_form::viewColorsAction (void)
{
	QAction *action = qobject_cast<QAction *> (sender());
	if (action == nullptr)
		return;

	const uint port_type = action->data().toUInt();
	if (0 >= port_type)
		return;

	const QColor& color = QColorDialog::getColor(
		m_ui.graphCanvas->portTypeColor(port_type), this,
		tr("Colors - %1").arg(action->text().remove('&')));
	if (color.isValid()) {
		m_ui.graphCanvas->setPortTypeColor(port_type, color);
		m_ui.graphCanvas->updatePortTypeColors(port_type);
		updateViewColorsAction(action);
	}
}


void qpwgraph_form::viewColorsReset (void)
{
	m_ui.graphCanvas->clearPortTypeColors();
	if (m_pipewire)
		m_pipewire->resetPortTypeColors();
#ifdef CONFIG_ALSA_MIDI
	if (m_alsamidi)
		m_alsamidi->resetPortTypeColors();
#endif
	m_ui.graphCanvas->updatePortTypeColors();

	updateViewColors();
}


void qpwgraph_form::viewSortTypeAction (void)
{
	QAction *action = qobject_cast<QAction *> (sender());
	if (action == nullptr)
		return;

	const qpwgraph_port::SortType sort_type
		= qpwgraph_port::SortType(action->data().toInt());
	qpwgraph_port::setSortType(sort_type);

	m_ui.graphCanvas->updateNodes();
}


void qpwgraph_form::viewSortOrderAction (void)
{
	QAction *action = qobject_cast<QAction *> (sender());
	if (action == nullptr)
		return;

	const qpwgraph_port::SortOrder sort_order
		= qpwgraph_port::SortOrder(action->data().toInt());
	qpwgraph_port::setSortOrder(sort_order);

	m_ui.graphCanvas->updateNodes();
}


void qpwgraph_form::viewRepelOverlappingNodes ( bool on )
{
	m_ui.graphCanvas->setRepelOverlappingNodes(on);
	if (on) ++m_repel_overlapping_nodes;
}


void qpwgraph_form::viewConnectThroughNodes ( bool on )
{
	qpwgraph_connect::setConnectThroughNodes(on);
	m_ui.graphCanvas->updateConnects();
}


void qpwgraph_form::helpSystemTray ( bool on )
{
#ifdef CONFIG_SYSTEM_TRAY
	if (on && m_systray == nullptr) {
		m_systray = new qpwgraph_systray(this);
		m_systray->show();
	}
	else
	if (!on && m_systray) {
		m_systray->hide();
		delete m_systray;
		m_systray = nullptr;
	}
#else
	(void) on;
#endif

	m_systray_closed = false;
}


void qpwgraph_form::helpAbout (void)
{
	static const QString title = PROJECT_NAME;
	static const QString version = PROJECT_VERSION;
	static const QString subtitle = PROJECT_DESCRIPTION;
	static const QString website = PROJECT_HOMEPAGE_URL;
	static const QString copyright
		= "Copyright (C) 2021-2023, rncbc aka Rui Nuno Capela. All rights reserved.";

	QString text = "<p>\n";
	text += "<b>" + title + " - " + subtitle + "</b><br />\n";
	text += "<br />\n";
	text += tr("Version") + ": <b>" + version + "</b><br />\n";
	text += "<br />\n";
	text += tr("Using: Qt %1").arg(qVersion());
#if defined(QT_STATIC)
	text += "-static";
#endif
	text += ", ";
	text +=	tr("libpipewire %1 (headers: %2)")
		.arg(pw_get_library_version())
		.arg(pw_get_headers_version());
	text += "<br />\n";
	text += "<br />\n";
	text += tr("Website") + ": <a href=\"" + website + + "\">" + website + "</a><br />\n";
	text += "<br />\n";
	text += "<small>";
	text += copyright + "<br />\n";
	text += "<br />\n";
	text += tr("This program is free software; you can redistribute it and/or modify it") + "<br />\n";
	text += tr("under the terms of the GNU General Public License version 2 or later.");
	text += "</small>";
	text += "</p>\n";

	QMessageBox::about(this, tr("About") + ' ' + title, text);
}


void qpwgraph_form::helpAboutQt (void)
{
	QMessageBox::aboutQt(this);
}


void qpwgraph_form::zoomValueChanged ( int zoom_value )
{
	m_ui.graphCanvas->setZoom(0.01 * qreal(zoom_value));
}


void qpwgraph_form::patchbayNameChanged ( int index )
{
	if (index > 0) {
		const QString& path
			= m_patchbay_names->itemData(index).toString();
		if (!path.isEmpty() && patchbayQueryClose())
			patchbayOpenFile(path);
	}

	updatePatchbayNames();
}


// Node life-cycle slots.
void qpwgraph_form::added ( qpwgraph_node *node )
{
	const qpwgraph_canvas *canvas
		= m_ui.graphCanvas;
	const QRectF& rect
		= canvas->mapToScene(canvas->viewport()->rect()).boundingRect();
	const QPointF& pos = rect.center();
	const qreal w = 0.33 * qMax(rect.width(),  800.0);
	const qreal h = 0.33 * qMax(rect.height(), 600.0);

	qreal x = pos.x();
	qreal y = pos.y();

	switch (node->nodeMode()) {
	case qpwgraph_item::Input:
		++m_ins &= 0x0f;
		x += w;
		y += 0.33 * h * (m_ins  & 1 ? +m_ins : -m_ins);
		break;
	case qpwgraph_item::Output:
		++m_outs &= 0x0f;
		x -= w;
		y += 0.33 * h * (m_outs & 1 ? +m_outs : -m_outs);
		break;
	default: {
		int dx = 0;
		int dy = 0;
		for (int i = 0; i < m_mids; ++i) {
			if ((qAbs(dx) > qAbs(dy)) || (dx == dy && dx < 0))
				dy += (dx < 0 ? +1 : -1);
			else
				dx += (dy < 0 ? -1 : +1);
		}
		x += 0.33 * w * qreal(dx);
		y += 0.33 * h * qreal(dy);
		++m_mids &= 0x1f;
		break;
	}}

	x = 4.0 * ::round(0.25 * (x - qreal(::rand() & 0x1f)));
	y = 4.0 * ::round(0.25 * (y - qreal(::rand() & 0x1f)));

	node->setPos(x, y);

	updated(node);
}


void qpwgraph_form::updated ( qpwgraph_node */*node*/ )
{
	if (m_ui.graphCanvas->isRepelOverlappingNodes())
		++m_repel_overlapping_nodes;
}


void qpwgraph_form::removed ( qpwgraph_node */*node*/ )
{
#if 0// FIXME: DANGEROUS! Node might have been deleted by now...
	if (node) {
		switch (node->nodeMode()) {
		case qpwgraph_item::Input:
			--m_ins;
			break;
		case qpwgraph_item::Output:
			--m_outs;
			break;
		default:
			--m_mids;
			break;
		}
	}
#endif
}


// Port (dis)connection slots.
void qpwgraph_form::connected ( qpwgraph_port *port1, qpwgraph_port *port2 )
{
	if (qpwgraph_pipewire::isPortType(port1->portType())) {
		if (m_pipewire)
			m_pipewire->connectPorts(port1, port2, true);
		pipewire_changed();
	}
#ifdef CONFIG_ALSA_MIDI
	else
	if (qpwgraph_alsamidi::isPortType(port1->portType())) {
		if (m_alsamidi)
			m_alsamidi->connectPorts(port1, port2, true);
		alsamidi_changed();
	}
#endif

	stabilize();
}


void qpwgraph_form::disconnected ( qpwgraph_port *port1, qpwgraph_port *port2 )
{
	if (qpwgraph_pipewire::isPortType(port1->portType())) {
		if (m_pipewire)
			m_pipewire->connectPorts(port1, port2, false);
		pipewire_changed();
	}
#ifdef CONFIG_ALSA_MIDI
	else
	if (qpwgraph_alsamidi::isPortType(port1->portType())) {
		if (m_alsamidi)
			m_alsamidi->connectPorts(port1, port2, false);
		alsamidi_changed();
	}
#endif

	stabilize();
}


void qpwgraph_form::connected ( qpwgraph_connect *connect )
{
	qpwgraph_port *port1 = connect->port1();
	if (port1 == nullptr)
		return;

	if (qpwgraph_pipewire::isPortType(port1->portType())) {
		if (m_pipewire)
			m_pipewire->addItem(connect, false);
	}
#ifdef CONFIG_ALSA_MIDI
	else
	if (qpwgraph_alsamidi::isPortType(port1->portType())) {
		if (m_alsamidi)
			m_alsamidi->addItem(connect, false);
	}
#endif
}


// Item renaming slot.
void qpwgraph_form::renamed ( qpwgraph_item *item, const QString& name )
{
	qpwgraph_sect *sect = item_sect(item);
	if (sect)
		sect->renameItem(item, name);
}


// Graph section slots.
void qpwgraph_form::pipewire_changed (void)
{
	++m_pipewire_changed;
}


void qpwgraph_form::alsamidi_changed (void)
{
	++m_alsamidi_changed;
}


// Pseudo-asyncronous timed refreshner.
void qpwgraph_form::refresh (void)
{
	if (m_ui.graphCanvas->isBusy()) {
		QTimer::singleShot(1200, this, SLOT(refresh()));
		return;
	}

	int nchanged = 0;

	if (m_pipewire_changed > 0) {
		m_pipewire_changed = 0;
		if (m_pipewire)
			m_pipewire->updateItems();
		++nchanged;
	}
#ifdef CONFIG_ALSA_MIDI
	if (m_alsamidi_changed > 0) {
		m_alsamidi_changed = 0;
		if (m_alsamidi)
			m_alsamidi->updateItems();
		++nchanged;
	}
#endif

	if (nchanged > 0) {
		qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
		if (patchbay)
			patchbay->scan();
		stabilize();
	}
	else
	if (m_repel_overlapping_nodes > 0) {
		m_repel_overlapping_nodes = 0;
		m_ui.graphCanvas->repelOverlappingNodesAll();
		stabilize();
	}

	QTimer::singleShot(300, this, SLOT(refresh()));
}


// Graph selection change slot.
void qpwgraph_form::stabilize (void)
{
	const qpwgraph_canvas *canvas
		= m_ui.graphCanvas;
	const qpwgraph_patchbay *patchbay
		= canvas->patchbay();
	const bool is_activated
		= (patchbay && patchbay->isActivated());
	const bool is_dirty
		= (patchbay && patchbay->isDirty());

	// Update window title.
	QString title = patchbayFileName();
	if (is_dirty)
		title += ' ' + tr("[modified]");
	setWindowTitle(title);
#ifdef CONFIG_SYSTEM_TRAY
	if (m_systray) m_systray->setToolTip(title);
#endif

	m_ui.patchbayExclusiveAction->setEnabled(is_activated);
	m_ui.patchbaySaveAction->setEnabled(is_dirty);

	m_ui.patchbayPinAction->setEnabled(canvas->canPatchbayPin());
	m_ui.patchbayUnpinAction->setEnabled(canvas->canPatchbayUnpin());

	m_ui.graphConnectAction->setEnabled(canvas->canConnect());
	m_ui.graphDisconnectAction->setEnabled(canvas->canDisconnect());

	m_ui.editSelectNoneAction->setEnabled(
		!canvas->scene()->selectedItems().isEmpty());
	m_ui.editRenameItemAction->setEnabled(
		canvas->canRenameItem());

#if 0
	const QRectF& outter_rect
		= canvas->scene()->sceneRect().adjusted(-2.0, -2.0, +2.0, +2.0);
	const QRectF& inner_rect
		= canvas->mapToScene(canvas->viewport()->rect()).boundingRect();
	const bool is_contained
		= outter_rect.contains(inner_rect) ||
			canvas->horizontalScrollBar()->isVisible() ||
			canvas->verticalScrollBar()->isVisible();
#else
	const bool is_contained = true;
#endif
	const qreal zoom = canvas->zoom();
	m_ui.viewCenterAction->setEnabled(is_contained);
	m_ui.viewZoomInAction->setEnabled(zoom < 1.9);
	m_ui.viewZoomOutAction->setEnabled(zoom > 0.1);
	m_ui.viewZoomFitAction->setEnabled(is_contained);
	m_ui.viewZoomResetAction->setEnabled(zoom != 1.0);

	const int zoom_value = int(100.0f * zoom);
	const bool is_spinbox_blocked = m_zoom_spinbox->blockSignals(true);
	const bool is_slider_blocked = m_zoom_slider->blockSignals(true);
	m_zoom_spinbox->setValue(zoom_value);
	m_zoom_slider->setValue(zoom_value);
	m_zoom_spinbox->blockSignals(is_spinbox_blocked);
	m_zoom_slider->blockSignals(is_slider_blocked);
}


// Tool-bar orientation change slot.
void qpwgraph_form::orientationChanged ( Qt::Orientation orientation )
{
	QToolBar *toolbar = qobject_cast<QToolBar *> (sender());
	if (toolbar == nullptr)
		return;

	if (toolbar == m_ui.patchbayToolbar && m_patchbay_names_tool)
		m_patchbay_names_tool->setVisible(orientation == Qt::Horizontal);

	if (m_config->isTextBesideIcons() && orientation == Qt::Horizontal) {
		toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	} else {
		toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	}
}


// Open/save patchbay file.
bool qpwgraph_form::patchbayOpenFile ( const QString& path, bool clear )
{
	qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
	if (patchbay == nullptr)
		return false;

	if (clear) {
		patchbay->clear();
		m_patchbay_path.clear();
	}

	if (!patchbay->load(path)) {
		QMessageBox::critical(this, tr("Error"),
			tr("Could not open patchbay file:\n\n%1\n\nSorry.").arg(path),
			QMessageBox::Cancel);
		return false;
	}

	m_config->patchbayRecentFiles(path);

	m_patchbay_dir = QFileInfo(path).absolutePath();
	m_patchbay_path = path;

	m_ui.graphCanvas->patchbayEdit();

	patchbay->scan();
	return true;
}


bool qpwgraph_form::patchbaySaveFile ( const QString& path )
{
	qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
	if (patchbay == nullptr)
		return false;

	if (!patchbay->save(path)) {
		QMessageBox::critical(this, tr("Error"),
			tr("Could not save patchbay file:\n\n%1\n\nSorry.").arg(path),
			QMessageBox::Cancel);
		return false;
	}

	m_config->patchbayRecentFiles(path);

	m_patchbay_dir = QFileInfo(path).absolutePath();
	m_patchbay_path = path;

	return true;
}


// Get the current display file-name.
QString qpwgraph_form::patchbayFileName (void) const
{
	if (m_patchbay_path.isEmpty())
		return tr("Untitled%1").arg(m_patchbay_untitled + 1);
	else
		return QFileInfo(m_patchbay_path).completeBaseName();
}


// Get default patchbay file directory/extension/filter.
QString qpwgraph_form::patchbayFileDir (void) const
{
	if (m_patchbay_path.isEmpty())
		return m_patchbay_dir;
	else
		return m_patchbay_path;
}


QString qpwgraph_form::patchbayFileExt (void) const
{
	return QString(PROJECT_NAME).toLower();
}


QString qpwgraph_form::patchbayFileFilter (void) const
{
	return tr("Patchbay files (*.%1)").arg(patchbayFileExt()) + ";;"
		 + tr("All files (*.*)");
}



// Whether we can close/quit current patchbay.
bool qpwgraph_form::patchbayQueryClose (void)
{
	bool ret = true;
	const qpwgraph_patchbay *patchbay
		= m_ui.graphCanvas->patchbay();
	if (patchbay && patchbay->isDirty()) {
		showNormal();
		switch (QMessageBox::warning(this, tr("Warning"),
			tr("The current patchbay has been changed:\n\n\"%1\"\n\n"
			"Do you want to save the changes?").arg(patchbayFileName()),
			QMessageBox::Save |
			QMessageBox::Discard |
			QMessageBox::Cancel)) {
		case QMessageBox::Save:
			patchbaySave();
		// Fall thru....
		case QMessageBox::Discard:
			break;
		default: // Cancel.
			ret = false;
			break;
		}
	}

	return ret;
}


bool qpwgraph_form::patchbayQueryQuit (void)
{
	if (!patchbayQueryClose())
		return false;

	bool ret = true;
	const qpwgraph_patchbay *patchbay
		= m_ui.graphCanvas->patchbay();
	if (patchbay && patchbay->isActivated()) {
		showNormal();
		ret = (QMessageBox::warning(this, tr("Warning"),
			tr("A patchbay is currently activated:\n\n\"%1\"\n\n"
			"Are you sure you want to quit?").arg(patchbayFileName()),
			QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok);
	}

	return ret;
}


// Context-menu event handler.
void qpwgraph_form::contextMenuEvent ( QContextMenuEvent *event )
{
	m_ui.graphCanvas->clear();

	stabilize();

	QMenu menu(this);
	if (m_ui.graphCanvas->isPatchbayEdit()) {
		menu.addAction(m_ui.patchbayPinAction);
		menu.addAction(m_ui.patchbayUnpinAction);
		menu.addSeparator();
	}
	menu.addAction(m_ui.graphConnectAction);
	menu.addAction(m_ui.graphDisconnectAction);
	menu.addSeparator();
	menu.addActions(m_ui.editMenu->actions());
	menu.addSeparator();
	menu.addMenu(m_ui.viewZoomMenu);

	menu.exec(event->globalPos());

	stabilize();
}


// Widget resize event handler.
void qpwgraph_form::resizeEvent ( QResizeEvent *event )
{
	QMainWindow::resizeEvent(event);

	stabilize();
}


// Widget event handlers.
void qpwgraph_form::showEvent ( QShowEvent *event )
{
	QMainWindow::showEvent(event);
#ifdef CONFIG_SYSTEM_TRAY
	if (m_systray)
		m_systray->updateContextMenu();
#endif
}


void qpwgraph_form::hideEvent ( QHideEvent *event )
{
	QMainWindow::hideEvent(event);
#ifdef CONFIG_SYSTEM_TRAY
	if (m_systray)
		m_systray->updateContextMenu();
#endif
	saveState();
}


void qpwgraph_form::closeEvent ( QCloseEvent *event )
{
#ifdef CONFIG_SYSTEM_TRAY
	if (m_systray) {
		if (!m_systray_closed) {
			const QString& title
				= tr("Information");
			const QString& text
				= tr("The program will keep running in the system tray.\n\n"
					"To terminate the program, please choose \"Quit\"\n"
					"in the main menu or the context menu of the system tray icon.");
			if (QSystemTrayIcon::supportsMessages())
				m_systray->showMessage(title, text, QSystemTrayIcon::Information);
			else
				QMessageBox::information(this, title, text);
		}
		m_systray_closed = true;
		hide();
		event->ignore();
	}
	else
#endif
	if (patchbayQueryQuit()) {
		hide();
		QMainWindow::closeEvent(event);
	} else {
		event->ignore();
	}
}


// Special port-type color methods.
void qpwgraph_form::updateViewColorsAction ( QAction *action )
{
	const uint port_type = action->data().toUInt();
	if (0 >= port_type)
		return;

	const QColor& color = m_ui.graphCanvas->portTypeColor(port_type);
	if (!color.isValid())
		return;

	QPixmap pm(22, 22);
	QPainter(&pm).fillRect(0, 0, pm.width(), pm.height(), color);
	action->setIcon(QIcon(pm));
}


void qpwgraph_form::updateViewColors (void)
{
	updateViewColorsAction(m_ui.viewColorsPipewireAudioAction);
	updateViewColorsAction(m_ui.viewColorsPipewireMidiAction);
	updateViewColorsAction(m_ui.viewColorsPipewireVideoAction);
	updateViewColorsAction(m_ui.viewColorsPipewireOtherAction);
#ifdef CONFIG_ALSA_MIDI
	updateViewColorsAction(m_ui.viewColorsAlsaMidiAction);
#endif
}


// Update patchbay recent files menu.
void qpwgraph_form::updatePatchbayMenu (void)
{
	// Rebuild the recent files menu...
	const QIcon icon(":/images/itemPatchbay.png");
	m_ui.patchbayOpenRecentMenu->clear();
	QStringListIterator iter(m_config->patchbayRecentFiles());
	for (int i = 0; iter.hasNext(); ++i) {
		const QFileInfo info(iter.next());
		if (info.exists()) {
			QAction *action = m_ui.patchbayOpenRecentMenu->addAction(icon,
				QString("&%1 %2").arg(i + 1).arg(info.completeBaseName()),
				this, SLOT(patchbayOpenRecent()));
			action->setData(info.absoluteFilePath());
		}
	}

	// Settle as enabled?
	m_ui.patchbayOpenRecentMenu->setEnabled(
		!m_ui.patchbayOpenRecentMenu->isEmpty());
}


// Update patchbay names combo-box (toolbar).
void qpwgraph_form::updatePatchbayNames (void)
{
	const bool is_blocked
		= m_patchbay_names->blockSignals(true);

	const QIcon icon(":/images/itemPatchbay.png");
	m_patchbay_names->clear();
	m_patchbay_names->addItem(icon,
		patchbayFileName(), m_patchbay_path);
	const QStringList& paths = m_config->patchbayRecentFiles();
	foreach (const QString& path, paths) {
		if (path == m_patchbay_path)
			continue;
		m_patchbay_names->addItem(icon,
			QFileInfo(path).completeBaseName(), path);
	}

	m_patchbay_names->setCurrentIndex(0);
	m_patchbay_names->blockSignals(is_blocked);

	stabilize();
}


// Item sect predicate.
qpwgraph_sect *qpwgraph_form::item_sect ( qpwgraph_item *item ) const
{
	if (item->type() == qpwgraph_node::Type) {
		qpwgraph_node *node = static_cast<qpwgraph_node *> (item);
		if (node && qpwgraph_pipewire::isNodeType(node->nodeType()))
			return m_pipewire;
	#ifdef CONFIG_ALSA_MIDI
		else
		if (node && qpwgraph_alsamidi::isNodeType(node->nodeType()))
			return m_alsamidi;
	#endif
	}
	else
	if (item->type() == qpwgraph_port::Type) {
		qpwgraph_port *port = static_cast<qpwgraph_port *> (item);
		if (port && qpwgraph_pipewire::isPortType(port->portType()))
			return m_pipewire;
	#ifdef CONFIG_ALSA_MIDI
		else
		if (port && qpwgraph_alsamidi::isPortType(port->portType()))
			return m_alsamidi;
	#endif
	}

	return nullptr; // No deal!
}


// Restore whole form state...
void qpwgraph_form::restoreState (void)
{
	m_config->restoreState(this);

	qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
	if (patchbay) {
		const bool is_activated = m_config->isPatchbayActivated();
		const bool is_exclusive = m_config->isPatchbayExclusive();
		const bool is_autopin = m_config->isPatchbayAutoPin();
		m_ui.patchbayActivatedAction->setChecked(is_activated);
		m_ui.patchbayExclusiveAction->setChecked(is_exclusive);
		m_ui.patchbayAutoPinAction->setChecked(is_autopin);
		patchbay->setActivated(is_activated);
		patchbay->setExclusive(is_exclusive);
		m_ui.graphCanvas->setPatchbayAutoPin(is_autopin);
	}

	m_ui.viewMenubarAction->setChecked(m_config->isMenubar());
	m_ui.viewGraphToolbarAction->setChecked(m_config->isToolbar());
	m_ui.viewPatchbayToolbarAction->setChecked(m_config->isPatchbayToolbar());
	m_ui.viewStatusbarAction->setChecked(m_config->isStatusbar());

	m_ui.viewTextBesideIconsAction->setChecked(m_config->isTextBesideIcons());
	m_ui.viewZoomRangeAction->setChecked(m_config->isZoomRange());
	m_ui.viewRepelOverlappingNodesAction->setChecked(m_config->isRepelOverlappingNodes());
	m_ui.viewConnectThroughNodesAction->setChecked(m_config->isConnectThroughNodes());

	const qpwgraph_port::SortType sort_type
		= qpwgraph_port::SortType(m_config->sortType());
	qpwgraph_port::setSortType(sort_type);
	switch (sort_type) {
	case qpwgraph_port::PortIndex:
		m_ui.viewSortPortIndexAction->setChecked(true);
		break;
	case qpwgraph_port::PortTitle:
		m_ui.viewSortPortTitleAction->setChecked(true);
		break;
	case qpwgraph_port::PortName:
	default:
		m_ui.viewSortPortNameAction->setChecked(true);
		break;
	}

	const qpwgraph_port::SortOrder sort_order
		= qpwgraph_port::SortOrder(m_config->sortOrder());
	qpwgraph_port::setSortOrder(sort_order);
	switch (sort_order) {
	case qpwgraph_port::Descending:
		m_ui.viewSortDescendingAction->setChecked(true);
		break;
	case qpwgraph_port::Ascending:
	default:
		m_ui.viewSortAscendingAction->setChecked(true);
		break;
	}

	viewMenubar(m_config->isMenubar());
	viewGraphToolbar(m_config->isToolbar());
	viewPatchbayToolbar(m_config->isPatchbayToolbar());
	viewStatusbar(m_config->isStatusbar());

	viewTextBesideIcons(m_config->isTextBesideIcons());
	viewZoomRange(m_config->isZoomRange());
	viewRepelOverlappingNodes(m_config->isRepelOverlappingNodes());
	viewConnectThroughNodes(m_config->isConnectThroughNodes());

	m_ui.graphCanvas->restoreState();

	// Restore last open patchbay directory and file-path...
	m_patchbay_dir = m_config->patchbayDir();
	m_patchbay_path = m_config->patchbayPath();

#ifdef CONFIG_SYSTEM_TRAY
	const bool is_systray_enabled
		= m_config->isSystemTrayEnabled();
	m_ui.helpSystemTrayAction->setChecked(is_systray_enabled);
	helpSystemTray(is_systray_enabled);
#endif
}


// Forcibly save whole form state.
void qpwgraph_form::saveState (void)
{
	m_ui.graphCanvas->saveState();

	m_config->setTextBesideIcons(m_ui.viewTextBesideIconsAction->isChecked());
	m_config->setZoomRange(m_ui.viewZoomRangeAction->isChecked());
	m_config->setSortType(int(qpwgraph_port::sortType()));
	m_config->setSortOrder(int(qpwgraph_port::sortOrder()));
	m_config->setRepelOverlappingNodes(m_ui.viewRepelOverlappingNodesAction->isChecked());
	m_config->setConnectThroughNodes(m_ui.viewConnectThroughNodesAction->isChecked());

	m_config->setStatusbar(m_ui.StatusBar->isVisible());
	m_config->setToolbar(m_ui.graphToolbar->isVisible());
	m_config->setPatchbayToolbar(m_ui.patchbayToolbar->isVisible());
	m_config->setMenubar(m_ui.MenuBar->isVisible());

	m_config->setPatchbayAutoPin(m_ui.patchbayAutoPinAction->isChecked());
	m_config->setPatchbayExclusive(m_ui.patchbayExclusiveAction->isChecked());
	m_config->setPatchbayActivated(m_ui.patchbayActivatedAction->isChecked());
	m_config->setPatchbayPath(m_patchbay_path);
	m_config->setPatchbayDir(m_patchbay_dir);

#ifdef CONFIG_SYSTEM_TRAY
	m_config->setSystemTrayEnabled(m_ui.helpSystemTrayAction->isChecked());
#endif

	m_config->saveState(this);
}


// Forcibly quit application.
void qpwgraph_form::closeQuit (void)
{
	if (!patchbayQueryQuit())
		return;

	if (isVisible())
		saveState();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QApplication::exit(0);
#else
	QApplication::quit();
#endif
}


// end of qpwgraph_form.cpp

