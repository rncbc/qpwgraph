// qpwgraph_config.h
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

#ifndef __qpwgraph_config_h
#define __qpwgraph_config_h

#include "qpwgraph_node.h"

#include <QString>


// Forwards decls.
class QSettings;
class QMainWindow;


//----------------------------------------------------------------------------
// qpwgraph_config --  Canvas state memento.

class qpwgraph_config
{
public:

	// Constructor.
	qpwgraph_config(QSettings *settings, bool owner = false);
	qpwgraph_config(const QString& org_name, const QString& app_name);

	// Destructor.
	~qpwgraph_config();

	// Accessors.
	void setSettings(QSettings *settings, bool owner = false);
	QSettings *settings() const;

	void setMenubar(bool menubar);
	bool isMenubar() const;

	void setToolbar(bool toolbar);
	bool isToolbar() const;

	void setStatusbar(bool statusbar);
	bool isStatusbar() const;

	void setTextBesideIcons(bool texticons);
	bool isTextBesideIcons() const;

	void setZoomRange(bool zoomrange);
	bool isZoomRange() const;

	void setSortType(int sorttype);
	int sortType() const;

	void setSortOrder(int sortorder);
	int sortOrder() const;

	void setRepelOverlappingNodes(bool repelnodes);
	bool isRepelOverlappingNodes() const;

	void setConnectThroughNodes(bool cthrunodes);
	bool isConnectThroughNodes() const;

	void setPatchbayToolbar(bool toolbar);
	bool isPatchbayToolbar() const;

	void setPatchbayDir(const QString& dir);
	const QString& patchbayDir() const;

	void setPatchbayPath(const QString& path);
	const QString& patchbayPath() const;

	void setPatchbayActivated(bool activated);
	bool isPatchbayActivated() const;

	void setPatchbayExclusive(bool exclusive);
	bool isPatchbayExclusive() const;

	void setPatchbayAutoPin(bool autopin);
	bool isPatchbayAutoPin() const;

	void patchbayRecentFiles(const QString& path);
	const QStringList& patchbayRecentFiles() const;

	void setSystemTrayEnabled(bool enabled);
	bool isSystemTrayEnabled() const;

	// Graph main-widget state methods.
	bool restoreState(QMainWindow *widget);
	bool saveState(QMainWindow *widget) const;

private:

	// Instance variables.
	QSettings  *m_settings;
	bool        m_owner;

	bool        m_menubar;
	bool        m_toolbar;
	bool        m_statusbar;
	bool        m_texticons;
	bool        m_zoomrange;
	int         m_sorttype;
	int         m_sortorder;

	bool        m_repelnodes;
	bool        m_cthrunodes;

	bool        m_patchbay_toolbar;
	QString     m_patchbay_dir;
	QString     m_patchbay_path;
	bool        m_patchbay_activated;
	bool        m_patchbay_exclusive;
	bool        m_patchbay_autopin;
	QStringList m_patchbay_recentfiles;

	bool        m_systray_enabled;
};


#endif	// __qpwgraph_config_h

// end of qpwgraph_config.h
