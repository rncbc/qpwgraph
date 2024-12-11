// qpwgraph_systray.h
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

#ifndef __qpwgraph_systray_h
#define __qpwgraph_systray_h

#include "config.h"

#ifdef CONFIG_SYSTEM_TRAY

#include <QSystemTrayIcon>
#include <QMenu>


// Forward decls.
class qpwgraph_main;
class QAction;


//----------------------------------------------------------------------------
// qpwgraph_systray -- Custom system tray icon.

class qpwgraph_systray : public QSystemTrayIcon
{
	Q_OBJECT

public:

	// Constructor.
	qpwgraph_systray(qpwgraph_main *main);

	// Destructor.
	~qpwgraph_systray();

	// Update context menu.
	void updateContextMenu();

protected slots:

	// Handle systeam tray activity.
	void activated(QSystemTrayIcon::ActivationReason reason);

	// Handle menu actions.
	void showHide();

	// Handle presets menu actions.
	void patchbayPresetSelected();

	// Rebuild the patchbay presets menu.
	void updatePatchbayPresets();

signals:
	// Notify about preset changes.
	void patchbayPresetChanged(int index);

protected:

	// Destroy the patchbay presets menu.
	void clearPatchbayPresets();

private:

	qpwgraph_main *m_main;

	QAction *m_show;
	QAction *m_quit;

	QMenu    m_menu;

	QMenu   *m_presets;
};


#endif	// CONFIG_SYSTEM_TRAY

#endif  // __qpwgraph_systray_h

// end of qpwgraph_systray.h
