// qpwgraph_form.cpp
//
/****************************************************************************
   Copyright (C) 2021-2022, rncbc aka Rui Nuno Capela. All rights reserved.

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
	m_ui.ToolBar->insertAction(before, undo_action);
	m_ui.ToolBar->insertAction(before, redo_action);
	m_ui.ToolBar->insertSeparator(before);

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
		SIGNAL(removed(qpwgraph_node *)),
		SLOT(removed(qpwgraph_node *)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(connected(qpwgraph_port *, qpwgraph_port *)),
		SLOT(connected(qpwgraph_port *, qpwgraph_port *)));
	QObject::connect(m_ui.graphCanvas,
		SIGNAL(disconnected(qpwgraph_port *, qpwgraph_port *)),
		SLOT(disconnected(qpwgraph_port *, qpwgraph_port *)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(renamed(qpwgraph_item *, const QString&)),
		SLOT(renamed(qpwgraph_item *, const QString&)));

	QObject::connect(m_ui.graphCanvas,
		SIGNAL(changed()),
		SLOT(stabilize()));

	// Some actions surely need those
	// shortcuts firmly attached...
	addAction(m_ui.viewMenubarAction);

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

	QObject::connect(m_ui.patchbayScanAction,
		SIGNAL(triggered(bool)),
		SLOT(patchbayScan()));

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
	QObject::connect(m_ui.viewToolbarAction,
		SIGNAL(triggered(bool)),
		SLOT(viewToolbar(bool)));

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

	m_ui.viewColorsPipewireAudioAction->setData(qpwgraph_pipewire::audioPortType());
	m_ui.viewColorsPipewireMidiAction->setData(qpwgraph_pipewire::midiPortType());
	m_ui.viewColorsPipewireVideoAction->setData(qpwgraph_pipewire::videoPortType());
	m_ui.viewColorsPipewireOtherAction->setData(qpwgraph_pipewire::otherPortType());
#ifdef CONFIG_ALSA_MIDI
	m_ui.viewColorsAlsaMidiAction->setData(qpwgraph_alsamidi::midiPortType());
#else
	m_ui.viewColorsMenu->removeAction(m_ui.viewColorsAlsaMidiAction);
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

	QObject::connect(m_ui.helpAboutAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAbout()));
	QObject::connect(m_ui.helpAboutQtAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAboutQt()));

	QObject::connect(m_ui.ToolBar,
		SIGNAL(orientationChanged(Qt::Orientation)),
		SLOT(orientationChanged(Qt::Orientation)));

#ifdef CONFIG_SYSTEM_TRAY
	m_systray = new qpwgraph_systray(this);
	m_systray->show();
#else
	m_systray = nullptr;
#endif

	restoreState();

	updatePatchbayMenu();
	updateViewColors();

	m_patchbay_dirty = 0;
	m_patchbay_untitled = 1;

	stabilize();

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
	delete m_systray;
#endif

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
		m_ui.patchbayActivatedAction->setChecked(true)
		if (app->isPatchbayExclusive())
			m_ui.patchbayExclusiveAction->setChecked(true)
		if (!app->patchbayPath().isEmpty())
			patchbayOpenFile(app->patchbayPath());
	}

	stabilize();
}


// Patchbay menu slots.
void qpwgraph_form::patchbayNew (void)
{
	if (!patchbayQueryClose())
		return;

	qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
	if (patchbay)
		patchbay->clear();

	m_patchbay_path.clear();
	m_patchbay_dirty = 0;
	++m_patchbay_untitled;

	stabilize();
}


void qpwgraph_form::patchbayOpen (void)
{
	if (!patchbayQueryClose())
		return;

	const QString& path
		= QFileDialog::getOpenFileName(this,
			tr("Open Patchbay File"), m_patchbay_path, patchbayFileFilter());
	if (path.isEmpty())
		return;

	patchbayOpenFile(path);
	stabilize();
}


void qpwgraph_form::updatePatchbayMenu (void)
{
	// Rebuild the recent files menu...
	m_ui.patchbayOpenRecentMenu->clear();
	QStringListIterator iter(m_config->patchbayRecentFiles());
	for (int i = 0; iter.hasNext(); ++i) {
		const QString& path = iter.next();
		if (QFileInfo(path).exists()) {
			QAction *action = m_ui.patchbayOpenRecentMenu->addAction(
				QString("&%1 %2").arg(i + 1).arg(path),
				this, SLOT(patchbayOpenRecent()));
			action->setData(i);
		}
	}

	// Settle as enabled?
	m_ui.patchbayOpenRecentMenu->setEnabled(
		!m_ui.patchbayOpenRecentMenu->isEmpty());
}


void qpwgraph_form::patchbayOpenRecent (void)
{
	// Retrive filename index from action data...
	QAction *action = qobject_cast<QAction *> (sender());
	if (action) {
		const int index = action->data().toInt();
		if (index >= 0 && index < m_config->patchbayRecentFiles().count()) {
			const QString path
				= m_config->patchbayRecentFiles().at(index);
			// Check if we can safely close the current file...
			if (!path.isEmpty() && patchbayQueryClose())
				patchbayOpenFile(path);
		}
	}

	stabilize();
}


void qpwgraph_form::patchbaySave (void)
{
	if (m_patchbay_path.isEmpty()) {
		patchbaySaveAs();
		return;
	}

	patchbaySaveFile(m_patchbay_path);

	stabilize();
}


void qpwgraph_form::patchbaySaveAs (void)
{
	const QString& path
		= QFileDialog::getSaveFileName(this,
			tr("Save Patchbay File"), m_patchbay_path, patchbayFileFilter());
	if (path.isEmpty())
		return;

	if (QFileInfo(path).suffix().isEmpty())
		patchbaySaveFile(path + '.' + patchbayFileExt());
	else
		patchbaySaveFile(path);

	stabilize();
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


void qpwgraph_form::patchbayScan (void)
{
	qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
	if (patchbay)
		patchbay->scan();

	stabilize();
}


// Main menu slots.
void qpwgraph_form::viewMenubar ( bool on )
{
	m_ui.MenuBar->setVisible(on);
}


void qpwgraph_form::viewToolbar ( bool on )
{
	m_ui.ToolBar->setVisible(on);
}


void qpwgraph_form::viewStatusbar ( bool on )
{
	m_ui.StatusBar->setVisible(on);
}


void qpwgraph_form::viewTextBesideIcons ( bool on )
{
	if (on) {
		m_ui.ToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	} else {
		m_ui.ToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
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


void qpwgraph_form::helpAbout (void)
{
	static const QString title = PROJECT_NAME;
	static const QString version = PROJECT_VERSION;
	static const QString subtitle = PROJECT_DESCRIPTION;
	static const QString website = PROJECT_HOMEPAGE_URL;
	static const QString copyright
		= "Copyright (C) 2021-2022, rncbc aka Rui Nuno Capela. All rights reserved.";

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

	stabilize();
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

	patchbayActivatedDirty();
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

	patchbayActivatedDirty();
	stabilize();
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
		patchbayScan();
	//	stabilize();
	}

	QTimer::singleShot(300, this, SLOT(refresh()));
}


// Graph selection change slot.
void qpwgraph_form::stabilize (void)
{
	// Update window title.
	QString title = patchbayCurrentName();
	if (m_patchbay_dirty > 0)
		title += ' ' + tr("[modified]");
	setWindowTitle(title);

	const qpwgraph_canvas *canvas = m_ui.graphCanvas;

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

	const qpwgraph_patchbay *patchbay = canvas->patchbay();
	if (patchbay) {
		const bool is_activated = patchbay->isActivated();
		m_ui.patchbayExclusiveAction->setEnabled(is_activated);
		m_ui.patchbayScanAction->setEnabled(is_activated);
	}

	m_ui.patchbaySaveAction->setEnabled(m_patchbay_dirty > 0);
}


// Tool-bar orientation change slot.
void qpwgraph_form::orientationChanged ( Qt::Orientation orientation )
{
	if (m_config->isTextBesideIcons() && orientation == Qt::Horizontal) {
		m_ui.ToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	} else {
		m_ui.ToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
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
		++m_patchbay_dirty;
	}

	if (!patchbay->load(path)) {
		QMessageBox::critical(this, tr("Error"),
			tr("Could not open patchbay file:\n\n%1\n\nSorry.").arg(path),
			QMessageBox::Cancel);
		return false;
	}

	m_config->patchbayRecentFiles(path);
	m_patchbay_path = path;
	m_patchbay_dirty = 0;

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
	m_patchbay_path = path;
	m_patchbay_dirty = 0;

	return true;
}


// Get the current display file-name.
QString qpwgraph_form::patchbayCurrentName (void) const
{
	if (m_patchbay_path.isEmpty())
		return tr("Untitled%1").arg(m_patchbay_untitled);
	else
		return QFileInfo(m_patchbay_path).completeBaseName();
}


// Get default patchbay file extension/filter.
QString qpwgraph_form::patchbayFileExt (void) const
{
	return QString(PROJECT_NAME).toLower();
}


QString qpwgraph_form::patchbayFileFilter (void) const
{
	return tr("Patchbay files (*.%1)").arg(patchbayFileExt()) + ";;"
		 + tr("All files (*.*)");
}


// Make current patchbay dirty if activated.
void qpwgraph_form::patchbayActivatedDirty (void)
{
	qpwgraph_patchbay *patchbay = m_ui.graphCanvas->patchbay();
	if (patchbay && patchbay->isActivated())
		++m_patchbay_dirty;
}


// Whether we can close current patchbay.
bool qpwgraph_form::patchbayQueryClose (void)
{
	bool ret = (m_patchbay_dirty > 0);
	if (!ret)
		return true;

	switch (QMessageBox::warning(this,
		tr("Warning"),
		tr("The current file has been changed:\n\n%1\n\n"
		"Do you want to save the changes?").arg(m_patchbay_path),
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

	return ret;
}


// Context-menu event handler.
void qpwgraph_form::contextMenuEvent ( QContextMenuEvent *event )
{
//	m_ui.graphCanvas->clear();

	stabilize();

	QMenu menu(this);
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
	m_systray->updateContextMenu();
#endif
}


void qpwgraph_form::hideEvent ( QHideEvent *event )
{
	QMainWindow::hideEvent(event);
#ifdef CONFIG_SYSTEM_TRAY
	m_systray->updateContextMenu();
#endif
	saveState();
}


void qpwgraph_form::closeEvent ( QCloseEvent *event )
{
	if (!patchbayQueryClose()) {
		event->ignore();
		return;
	}

	hide();
#ifdef CONFIG_SYSTEM_TRAY
	m_systray->updateContextMenu();
	event->ignore();
#else
	QMainWindow::closeEvent(event);
#endif
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
		m_ui.patchbayActivatedAction->setChecked(is_activated);
		m_ui.patchbayExclusiveAction->setChecked(is_exclusive);
		patchbay->setActivated(is_activated);
		patchbay->setExclusive(is_exclusive);
	}

	m_ui.viewMenubarAction->setChecked(m_config->isMenubar());
	m_ui.viewToolbarAction->setChecked(m_config->isToolbar());
	m_ui.viewStatusbarAction->setChecked(m_config->isStatusbar());

	m_ui.viewTextBesideIconsAction->setChecked(m_config->isTextBesideIcons());
	m_ui.viewZoomRangeAction->setChecked(m_config->isZoomRange());

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
	viewToolbar(m_config->isToolbar());
	viewStatusbar(m_config->isStatusbar());

	viewTextBesideIcons(m_config->isTextBesideIcons());
	viewZoomRange(m_config->isZoomRange());

	m_ui.graphCanvas->restoreState();
}


// Forcibly save whole form state.
void qpwgraph_form::saveState (void)
{
	m_ui.graphCanvas->saveState();

	m_config->setTextBesideIcons(m_ui.viewTextBesideIconsAction->isChecked());
	m_config->setZoomRange(m_ui.viewZoomRangeAction->isChecked());
	m_config->setSortType(int(qpwgraph_port::sortType()));
	m_config->setSortOrder(int(qpwgraph_port::sortOrder()));

	m_config->setStatusbar(m_ui.StatusBar->isVisible());
	m_config->setToolbar(m_ui.ToolBar->isVisible());
	m_config->setMenubar(m_ui.MenuBar->isVisible());

	m_config->setPatchbayExclusive(m_ui.patchbayExclusiveAction->isChecked());
	m_config->setPatchbayActivated(m_ui.patchbayActivatedAction->isChecked());

	m_config->saveState(this);
}


// Forcibly quit application.
void qpwgraph_form::closeQuit (void)
{
	saveState();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QApplication::exit(0);
#else
	QApplication::quit();
#endif
}


// end of qpwgraph_form.cpp

