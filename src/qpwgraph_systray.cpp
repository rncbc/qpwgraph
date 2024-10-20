// qpwgraph_systray.cpp
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

#include "qpwgraph_systray.h"


#ifdef CONFIG_SYSTEM_TRAY

#include "qpwgraph_main.h"

#include <QWidget>
#include <QAction>

#include <QApplication>


//----------------------------------------------------------------------------
// qpwgraph_systray -- Custom system tray icon.

// Constructor.
qpwgraph_systray::qpwgraph_systray ( qpwgraph_main *main )
	: QSystemTrayIcon(main), m_main(main)
{
	// Set things as inherited...
#if QT_VERSION < QT_VERSION_CHECK(6, 1, 0)
	QSystemTrayIcon::setIcon(QIcon(":/images/qpwgraph.png"));
#else
	QSystemTrayIcon::setIcon(m_main->windowIcon().pixmap(32, 32));
#endif
	QSystemTrayIcon::setToolTip(m_main->windowTitle());

	m_show = m_menu.addAction(tr("Show/Hide"), this, SLOT(showHide()));
	m_quit = m_menu.addAction(tr("Quit"), m_main, SLOT(closeQuit()));

	QSystemTrayIcon::setContextMenu(&m_menu);

	QObject::connect(this,
		SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		SLOT(activated(QSystemTrayIcon::ActivationReason)));

	QSystemTrayIcon::show();
}


// Update context menu.
void qpwgraph_systray::updateContextMenu (void)
{
	if (m_main->isVisible() && !m_main->isMinimized())
		m_show->setText(tr("Hide"));
	else
		m_show->setText(tr("Show"));
}


// Handle systeam tray activity.
void qpwgraph_systray::activated ( QSystemTrayIcon::ActivationReason reason )
{
	switch (reason) {
	case QSystemTrayIcon::Trigger:
		showHide();
		// Fall trhu...
	case QSystemTrayIcon::MiddleClick:
	case QSystemTrayIcon::DoubleClick:
	case QSystemTrayIcon::Unknown:
	default:
		break;
	}
}


// Handle menu actions.
void qpwgraph_systray::showHide (void)
{
	if (m_main->isVisible() && !m_main->isMinimized()) {
		// Hide away from sight, totally...
		m_main->hide();
	} else {
		// Show normally.
		m_main->showNormal();
		m_main->raise();
		m_main->activateWindow();
	}
}


#endif	// CONFIG_SYSTEM_TRAY

// end of qpwgraph_systray.cpp
