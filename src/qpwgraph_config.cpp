// qpwgraph_config.cpp
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

#include "qpwgraph_config.h"

#include <QSettings>

#include <QMainWindow>


// Local constants.
static const char *GeometryGroup    = "/GraphGeometry";
static const char *LayoutGroup      = "/GraphLayout";

static const char *ViewGroup        = "/GraphView";
static const char *ViewMenubarKey   = "/Menubar";
static const char *ViewToolbarKey   = "/Toolbar";
static const char *ViewStatusbarKey = "/Statusbar";
static const char *ViewTextBesideIconsKey = "/TextBesideIcons";
static const char *ViewZoomRangeKey = "/ZoomRange";
static const char *ViewSortTypeKey  = "/SortType";
static const char *ViewSortOrderKey = "/SortOrder";

static const char *PatchbayGroup    = "/Patchbay";
static const char *RecentFilesKey	= "/RecentFiles";
static const char *ActivatedKey	    = "/Activated";
static const char *ExclusiveKey	    = "/Exclusive";


//----------------------------------------------------------------------------
// qpwgraph_config --  Canvas state memento.

// Constructors.
qpwgraph_config::qpwgraph_config ( QSettings *settings, bool owner )
	: m_settings(settings), m_owner(owner),
		m_menubar(false), m_toolbar(false), m_statusbar(false),
		m_texticons(false), m_zoomrange(false),
		m_sorttype(0), m_sortorder(0)
{
}


qpwgraph_config::qpwgraph_config ( const QString& org_name, const QString& app_name )
	: qpwgraph_config(new QSettings(org_name, app_name), true)
{
}


// Destructor.
qpwgraph_config::~qpwgraph_config (void)
{
	setSettings(nullptr);
}


// Accessors.
void qpwgraph_config::setSettings ( QSettings *settings, bool owner )
{
	if (m_settings && m_owner)
		delete m_settings;

	m_settings = settings;
	m_owner = owner;
}


QSettings *qpwgraph_config::settings (void) const
{
	return m_settings;
}


void qpwgraph_config::setMenubar ( bool menubar )
{
	m_menubar = menubar;
}

bool qpwgraph_config::isMenubar (void) const
{
	return m_menubar;
}


void qpwgraph_config::setToolbar ( bool toolbar )
{
	m_toolbar = toolbar;
}

bool qpwgraph_config::isToolbar (void) const
{
	return m_toolbar;
}


void qpwgraph_config::setStatusbar ( bool statusbar )
{
	m_statusbar = statusbar;
}

bool qpwgraph_config::isStatusbar (void) const
{
	return m_statusbar;
}


void qpwgraph_config::setTextBesideIcons ( bool texticons )
{
	m_texticons = texticons;
}

bool qpwgraph_config::isTextBesideIcons (void) const
{
	return m_texticons;
}


void qpwgraph_config::setZoomRange ( bool zoomrange )
{
	m_zoomrange = zoomrange;
}

bool qpwgraph_config::isZoomRange (void) const
{
	return m_zoomrange;
}


void qpwgraph_config::setSortType ( int sorttype )
{
	m_sorttype = sorttype;
}

int qpwgraph_config::sortType (void) const
{
	return m_sorttype;
}


void qpwgraph_config::setSortOrder ( int sortorder )
{
	m_sortorder = sortorder;
}

int qpwgraph_config::sortOrder (void) const
{
	return m_sortorder;
}


void qpwgraph_config::patchbayRecentFiles ( const QString& path )
{
	// Remove from list if already there (avoid duplicates)
	if (m_patchbay_recentfiles.contains(path))
		m_patchbay_recentfiles.removeAll(path);

	// Put it to front...
	m_patchbay_recentfiles.push_front(path);

	// Time to keep the list under limits.
	int nfiles = m_patchbay_recentfiles.count();
	while (nfiles > 8) {
		m_patchbay_recentfiles.pop_back();
		--nfiles;
	}
}

const QStringList& qpwgraph_config::patchbayRecentFiles (void) const
{
	return m_patchbay_recentfiles;
}


void qpwgraph_config::setPatchbayActivated ( bool activated )
{
	m_patchbay_activated = activated;
}

int qpwgraph_config::isPatchbayActivated (void) const
{
	return m_patchbay_activated;
}


void qpwgraph_config::setPatchbayExclusive ( bool exclusive )
{
	m_patchbay_exclusive = exclusive;
}

int qpwgraph_config::isPatchbayExclusive (void) const
{
	return m_patchbay_exclusive;
}


// Graph main-widget state methods.
bool qpwgraph_config::restoreState ( QMainWindow *widget )
{
	if (m_settings == nullptr || widget == nullptr)
		return false;

	m_settings->beginGroup(PatchbayGroup);
	m_patchbay_recentfiles = m_settings->value(RecentFilesKey).toStringList();
	m_patchbay_activated = m_settings->value(ActivatedKey).toBool();
	m_patchbay_exclusive = m_settings->value(ExclusiveKey).toBool();
	m_settings->endGroup();

	m_settings->beginGroup(ViewGroup);
	m_menubar = m_settings->value(ViewMenubarKey, true).toBool();
	m_toolbar = m_settings->value(ViewToolbarKey, true).toBool();
	m_statusbar = m_settings->value(ViewStatusbarKey, true).toBool();
	m_texticons = m_settings->value(ViewTextBesideIconsKey, true).toBool();
	m_zoomrange = m_settings->value(ViewZoomRangeKey, false).toBool();
	m_sorttype  = m_settings->value(ViewSortTypeKey, 0).toInt();
	m_sortorder = m_settings->value(ViewSortOrderKey, 0).toInt();
	m_settings->endGroup();

	m_settings->beginGroup(GeometryGroup);
	const QByteArray& geometry_state
		= m_settings->value('/' + widget->objectName()).toByteArray();
	m_settings->endGroup();

	if (geometry_state.isEmpty() || geometry_state.isNull())
		return false;

	widget->restoreGeometry(geometry_state);

	m_settings->beginGroup(LayoutGroup);
	const QByteArray& layout_state
		= m_settings->value('/' + widget->objectName()).toByteArray();
	m_settings->endGroup();

	if (layout_state.isEmpty() || layout_state.isNull())
		return false;

	widget->restoreState(layout_state);

	return true;
}


bool qpwgraph_config::saveState ( QMainWindow *widget ) const
{
	if (m_settings == nullptr || widget == nullptr)
		return false;

	m_settings->beginGroup(PatchbayGroup);
	m_settings->setValue(RecentFilesKey, m_patchbay_recentfiles);
	m_settings->setValue(ActivatedKey, m_patchbay_activated);
	m_settings->setValue(ExclusiveKey, m_patchbay_exclusive);
	m_settings->endGroup();

	m_settings->beginGroup(ViewGroup);
	m_settings->setValue(ViewMenubarKey, m_menubar);
	m_settings->setValue(ViewToolbarKey, m_toolbar);
	m_settings->setValue(ViewStatusbarKey, m_statusbar);
	m_settings->setValue(ViewTextBesideIconsKey, m_texticons);
	m_settings->setValue(ViewZoomRangeKey, m_zoomrange);
	m_settings->setValue(ViewSortTypeKey, m_sorttype);
	m_settings->setValue(ViewSortOrderKey, m_sortorder);
	m_settings->endGroup();

	m_settings->beginGroup(GeometryGroup);
	const QByteArray& geometry_state = widget->saveGeometry();
	m_settings->setValue('/' + widget->objectName(), geometry_state);
	m_settings->endGroup();

	m_settings->beginGroup(LayoutGroup);
	const QByteArray& layout_state = widget->saveState();
	m_settings->setValue('/' + widget->objectName(), layout_state);
	m_settings->endGroup();

	return true;
}


// end of qpwgraph_config.cpp
