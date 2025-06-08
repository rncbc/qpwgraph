// qpwgraph_config.cpp
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

#include "config.h"

#include "qpwgraph_config.h"

#include <QSettings>

#include <QMainWindow>
#include <QFileInfo>

#include <QComboBox>


// Local constants.
static const char *GeometryGroup    = "/GraphGeometry";
static const char *LayoutGroup      = "/GraphLayout";

static const char *ViewGroup        = "/GraphView";
static const char *ViewMenubarKey   = "/Menubar";
static const char *ViewToolbarKey   = "/Toolbar";
static const char *ViewStatusbarKey = "/Statusbar";
static const char *ViewThumbviewKey = "/Thumbview";
static const char *ViewTextBesideIconsKey = "/TextBesideIcons";
static const char *ViewZoomRangeKey = "/ZoomRange";
static const char *ViewSortTypeKey  = "/SortType";
static const char *ViewSortOrderKey = "/SortOrder";
static const char *ViewRepelOverlappingNodesKey = "/RepelOverlappingNodes";
static const char *ViewConnectThroughNodesKey = "/ConnectThroughNodes";

static const char *PatchbayGroup    = "/Patchbay";
static const char *PatchbayDirKey   = "/Dir";
static const char *PatchbayPathKey  = "/Path";
static const char *PatchbayActivatedKey = "/Activated";
static const char *PatchbayExclusiveKey = "/Exclusive";
static const char *PatchbayAutoPinKey = "/AutoPin";
static const char *PatchbayAutoDisconnectKey = "/AutoDisconnect";
static const char *PatchbayRecentFilesKey = "/RecentFiles";
static const char *PatchbayToolbarKey = "/Toolbar";
static const char *PatchbayQueryQuitKey = "/QueryQuit";

#ifdef CONFIG_SYSTEM_TRAY
static const char *SystemTrayGroup  = "/SystemTray";
static const char *SystemTrayEnabledKey = "/Enabled";
static const char *SystemTrayQueryCloseKey = "/QueryClose";
static const char *SystemTrayStartMinimizedKey = "/StartMinimized";
#endif

#ifdef CONFIG_ALSA_MIDI
static const char *AlsaMidiGroup  = "/AlsaMidi";
static const char *AlsaMidiEnabledKey = "/Enabled";
#endif

static const char *SessionGroup = "/Session";
static const char *SessionStartMinimizedKey = "/StartMinimized";

static const char *FilterNodesGroup = "/FilterNodes";
static const char *MergerNodesGroup = "/MergerNodes";
static const char *NodesEnabledKey = "/Enabled";
static const char *NodesListKey = "/List";

static const char *HistoryGroup = "/History";


// Legacy main-form class renaming support (> v0.7.7)
#define LEGACY_MAIN_FORM 1
#ifdef  LEGACY_MAIN_FORM
static const char *LegacyName = "form";
static const char *ModernName = "main";
#endif


//----------------------------------------------------------------------------
// qpwgraph_config --  Canvas state memento.

// Constructors.
qpwgraph_config::qpwgraph_config ( QSettings *settings, bool owner )
	: m_settings(settings), m_owner(owner),
		m_menubar(false), m_toolbar(false), m_statusbar(false),
		m_thumbview(0), m_texticons(false), m_zoomrange(false),
		m_sorttype(0), m_sortorder(0),
		m_repelnodes(false),
		m_cthrunodes(false),
		m_patchbay_toolbar(false),
		m_patchbay_activated(false),
		m_patchbay_exclusive(false),
		m_patchbay_autopin(true),
		m_patchbay_autodisconnect(false),
		m_patchbay_queryquit(false),
		m_systray_queryclose(false),
		m_systray_enabled(true),
		m_alsaseq_enabled(true),
		m_start_minimized(false),
		m_filter_enabled(false),
		m_filter_dirty(false),
		m_merger_enabled(false),
		m_merger_dirty(false)
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


void qpwgraph_config::setThumbview ( int thumbview )
{
	m_thumbview = thumbview;
}

int qpwgraph_config::thumbview (void) const
{
	return m_thumbview;
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


void qpwgraph_config::setRepelOverlappingNodes ( bool repelnodes )
{
	m_repelnodes = repelnodes;
}


bool qpwgraph_config::isRepelOverlappingNodes (void) const
{
	return m_repelnodes;
}


void qpwgraph_config::setConnectThroughNodes ( bool cthrunodes )
{
	m_cthrunodes = cthrunodes;
}


bool qpwgraph_config::isConnectThroughNodes (void) const
{
	return m_cthrunodes;
}


void qpwgraph_config::setPatchbayToolbar ( bool toolbar )
{
	m_patchbay_toolbar = toolbar;
}

bool qpwgraph_config::isPatchbayToolbar (void) const
{
	return m_patchbay_toolbar;
}


void qpwgraph_config::setPatchbayDir ( const QString& dir )
{
	m_patchbay_dir = dir;
}

const QString& qpwgraph_config::patchbayDir (void) const
{
	return m_patchbay_dir;
}


void qpwgraph_config::setPatchbayPath ( const QString& path )
{
	m_patchbay_path = path;
}

const QString& qpwgraph_config::patchbayPath (void) const
{
	return m_patchbay_path;
}


void qpwgraph_config::setPatchbayActivated ( bool activated )
{
	m_patchbay_activated = activated;
}

bool qpwgraph_config::isPatchbayActivated (void) const
{
	return m_patchbay_activated;
}


void qpwgraph_config::setPatchbayExclusive ( bool exclusive )
{
	m_patchbay_exclusive = exclusive;
}

bool qpwgraph_config::isPatchbayExclusive (void) const
{
	return m_patchbay_exclusive;
}


void qpwgraph_config::setPatchbayAutoPin ( bool autopin )
{
	m_patchbay_autopin = autopin;
}

bool qpwgraph_config::isPatchbayAutoPin (void) const
{
	return m_patchbay_autopin;
}


void qpwgraph_config::setPatchbayAutoDisconnect ( bool autodisconnect )
{
	m_patchbay_autodisconnect = autodisconnect;
}

bool qpwgraph_config::isPatchbayAutoDisconnect (void) const
{
	return m_patchbay_autodisconnect;
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



void qpwgraph_config::setPatchbayQueryQuit ( bool query_quit )
{
	m_patchbay_queryquit = query_quit;
}

bool qpwgraph_config::isPatchbayQueryQuit (void) const
{
	return m_patchbay_queryquit;
}


void qpwgraph_config::setSystemTrayQueryClose ( bool query_close )
{
	m_systray_queryclose = query_close;
}

bool qpwgraph_config::isSystemTrayQueryClose (void) const
{
	return m_systray_queryclose;
}


void qpwgraph_config::setSystemTrayEnabled ( bool enabled )
{
	m_systray_enabled = enabled;
}

bool qpwgraph_config::isSystemTrayEnabled (void) const
{
	return m_systray_enabled;
}


void qpwgraph_config::setAlsaMidiEnabled ( bool enabled )
{
	m_alsaseq_enabled = enabled;
}

bool qpwgraph_config::isAlsaMidiEnabled (void) const
{
	return m_alsaseq_enabled;
}


void qpwgraph_config::setStartMinimized ( bool start_minimized )
{
	m_start_minimized = start_minimized;
}

bool qpwgraph_config::isStartMinimized (void) const
{
	return m_start_minimized;
}


void qpwgraph_config::setFilterNodesEnabled ( bool enabled )
{
	m_filter_enabled = enabled;
}

bool qpwgraph_config::isFilterNodesEnabled (void) const
{
	return m_filter_enabled;
}


void qpwgraph_config::setFilterNodesList ( const QStringList& nodes )
{
	m_filter_nodes = nodes;
}

const QStringList& qpwgraph_config::filterNodesList (void) const
{
	return m_filter_nodes;
}


void qpwgraph_config::setFilterNodesDirty ( bool dirty )
{
	m_filter_dirty = dirty;
}

bool qpwgraph_config::isFilterNodesDirty (void) const
{
	return m_filter_dirty;
}


void qpwgraph_config::setMergerNodesEnabled ( bool enabled )
{
	m_merger_enabled = enabled;
}

bool qpwgraph_config::isMergerNodesEnabled (void) const
{
	return m_merger_enabled;
}


void qpwgraph_config::setMergerNodesList ( const QStringList& nodes )
{
	m_merger_nodes = nodes;
}

const QStringList& qpwgraph_config::mergerNodesList (void) const
{
	return m_merger_nodes;
}


void qpwgraph_config::setMergerNodesDirty ( bool dirty )
{
	m_merger_dirty = dirty;
}

bool qpwgraph_config::isMergerNodesDirty (void) const
{
	return m_merger_dirty;
}


void qpwgraph_config::setSessionStartMinimized ( bool start_minimized )
{
	m_settings->beginGroup(SessionGroup);
	m_settings->setValue(SessionStartMinimizedKey, start_minimized);
	m_settings->endGroup();

	m_settings->sync();
}

bool qpwgraph_config::isSessionStartMinimized (void) const
{
	m_settings->beginGroup(SessionGroup);
	const bool start_minimized
		= m_settings->value(SessionStartMinimizedKey, false).toBool();
	m_settings->endGroup();

	return start_minimized;
}


// Graph main-widget state methods.
bool qpwgraph_config::restoreState ( QMainWindow *widget )
{
	if (m_settings == nullptr || widget == nullptr)
		return false;

	m_settings->beginGroup(MergerNodesGroup);
	m_merger_enabled = m_settings->value(NodesEnabledKey, false).toBool();
	m_merger_nodes = m_settings->value(NodesListKey).toStringList();
	m_settings->endGroup();

	m_settings->beginGroup(FilterNodesGroup);
	m_filter_enabled = m_settings->value(NodesEnabledKey, false).toBool();
	m_filter_nodes = m_settings->value(NodesListKey).toStringList();
	m_settings->endGroup();

#ifdef CONFIG_SYSTEM_TRAY
	m_settings->beginGroup(SystemTrayGroup);
	m_systray_enabled = m_settings->value(SystemTrayEnabledKey, true).toBool();
	m_systray_queryclose = m_settings->value(SystemTrayQueryCloseKey, true).toBool();
	m_start_minimized = m_settings->value(SystemTrayStartMinimizedKey, false).toBool();
	m_settings->endGroup();
#endif

#ifdef CONFIG_ALSA_MIDI
	m_settings->beginGroup(AlsaMidiGroup);
	m_alsaseq_enabled = m_settings->value(AlsaMidiEnabledKey, true).toBool();
	m_settings->endGroup();
#endif

	m_settings->beginGroup(PatchbayGroup);
	m_patchbay_toolbar = m_settings->value(PatchbayToolbarKey, true).toBool();
	m_patchbay_dir = m_settings->value(PatchbayDirKey).toString();
	m_patchbay_path = m_settings->value(PatchbayPathKey).toString();
	m_patchbay_activated = m_settings->value(PatchbayActivatedKey, false).toBool();
	m_patchbay_exclusive = m_settings->value(PatchbayExclusiveKey, false).toBool();
	m_patchbay_autopin = m_settings->value(PatchbayAutoPinKey, true).toBool();
	m_patchbay_autodisconnect = m_settings->value(PatchbayAutoDisconnectKey, false).toBool();
	m_patchbay_recentfiles = m_settings->value(PatchbayRecentFilesKey).toStringList();
	m_patchbay_queryquit = m_settings->value(PatchbayQueryQuitKey, true).toBool();
	m_settings->endGroup();

	QMutableStringListIterator iter(m_patchbay_recentfiles);
	while (iter.hasNext()) {
		if (!QFileInfo(iter.next()).exists())
			iter.remove();
	}

	m_settings->beginGroup(ViewGroup);
	m_menubar = m_settings->value(ViewMenubarKey, true).toBool();
	m_toolbar = m_settings->value(ViewToolbarKey, true).toBool();
	m_statusbar = m_settings->value(ViewStatusbarKey, true).toBool();
	m_thumbview = m_settings->value(ViewThumbviewKey, 0).toInt();
	m_texticons = m_settings->value(ViewTextBesideIconsKey, true).toBool();
	m_zoomrange = m_settings->value(ViewZoomRangeKey, false).toBool();
	m_sorttype  = m_settings->value(ViewSortTypeKey, 0).toInt();
	m_sortorder = m_settings->value(ViewSortOrderKey, 0).toInt();
	m_repelnodes = m_settings->value(ViewRepelOverlappingNodesKey, false).toBool();
	m_cthrunodes = m_settings->value(ViewConnectThroughNodesKey, false).toBool();
	m_settings->endGroup();

	m_settings->beginGroup(GeometryGroup);
#ifdef LEGACY_MAIN_FORM
	QString sGeometryKey = '/' + widget->objectName();
	QByteArray geometry_state = m_settings->value(sGeometryKey).toByteArray();
	if (geometry_state.isEmpty() || geometry_state.isNull()) {
		sGeometryKey.replace(ModernName, LegacyName);
		geometry_state = m_settings->value(sGeometryKey).toByteArray();
		if (!geometry_state.isEmpty() && !geometry_state.isNull())
			m_settings->remove(sGeometryKey);
	}
#else
	const QByteArray& geometry_state
		= m_settings->value('/' + widget->objectName()).toByteArray();
#endif
	m_settings->endGroup();

	if (geometry_state.isEmpty() || geometry_state.isNull())
		return false;

	widget->restoreGeometry(geometry_state);

	m_settings->beginGroup(LayoutGroup);
#ifdef LEGACY_MAIN_FORM
	QString sLayoutKey = '/' + widget->objectName();
	QByteArray layout_state = m_settings->value(sLayoutKey).toByteArray();
	if (layout_state.isEmpty() || layout_state.isNull()) {
		sLayoutKey.replace(ModernName, LegacyName);
		layout_state = m_settings->value(sLayoutKey).toByteArray();
		if (!layout_state.isEmpty() && !layout_state.isNull())
			m_settings->remove(sLayoutKey);
	}
#else
	const QByteArray& layout_state
		= m_settings->value('/' + widget->objectName()).toByteArray();
#endif
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

	m_settings->beginGroup(MergerNodesGroup);
	m_settings->setValue(NodesEnabledKey, m_merger_enabled);
	m_settings->setValue(NodesListKey, m_merger_nodes);
	m_settings->endGroup();

	m_settings->beginGroup(FilterNodesGroup);
	m_settings->setValue(NodesEnabledKey, m_filter_enabled);
	m_settings->setValue(NodesListKey, m_filter_nodes);
	m_settings->endGroup();

#ifdef CONFIG_SYSTEM_TRAY
	m_settings->beginGroup(SystemTrayGroup);
	m_settings->setValue(SystemTrayEnabledKey, m_systray_enabled);
	m_settings->setValue(SystemTrayQueryCloseKey, m_systray_queryclose);
	m_settings->setValue(SystemTrayStartMinimizedKey, m_start_minimized);
	m_settings->endGroup();
#endif

#ifdef CONFIG_ALSA_MIDI
	m_settings->beginGroup(AlsaMidiGroup);
	m_settings->setValue(AlsaMidiEnabledKey, m_alsaseq_enabled);
	m_settings->endGroup();
#endif

	m_settings->beginGroup(PatchbayGroup);
	m_settings->setValue(PatchbayToolbarKey, m_patchbay_toolbar);
	m_settings->setValue(PatchbayDirKey, m_patchbay_dir);
	m_settings->setValue(PatchbayPathKey, m_patchbay_path);
	m_settings->setValue(PatchbayActivatedKey, m_patchbay_activated);
	m_settings->setValue(PatchbayExclusiveKey, m_patchbay_exclusive);
	m_settings->setValue(PatchbayAutoPinKey, m_patchbay_autopin);
	m_settings->setValue(PatchbayAutoDisconnectKey, m_patchbay_autodisconnect);
	m_settings->setValue(PatchbayRecentFilesKey, m_patchbay_recentfiles);
	m_settings->setValue(PatchbayQueryQuitKey, m_patchbay_queryquit);
	m_settings->endGroup();

	m_settings->beginGroup(ViewGroup);
	m_settings->setValue(ViewMenubarKey, m_menubar);
	m_settings->setValue(ViewToolbarKey, m_toolbar);
	m_settings->setValue(ViewStatusbarKey, m_statusbar);
	m_settings->setValue(ViewThumbviewKey, m_thumbview);
	m_settings->setValue(ViewTextBesideIconsKey, m_texticons);
	m_settings->setValue(ViewZoomRangeKey, m_zoomrange);
	m_settings->setValue(ViewSortTypeKey, m_sorttype);
	m_settings->setValue(ViewSortOrderKey, m_sortorder);
	m_settings->setValue(ViewRepelOverlappingNodesKey, m_repelnodes);
	m_settings->setValue(ViewConnectThroughNodesKey, m_cthrunodes);
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


// Combo box history persistence helpers.
//
void qpwgraph_config::loadComboBoxHistory ( QComboBox *cbox, int nlimit )
{
	m_settings->beginGroup(HistoryGroup);
	if (m_settings->childKeys().count() > 0) {
		const QStringList& items
			= m_settings->value('/' + cbox->objectName()).toStringList();
		if (!items.isEmpty()) {
			const bool block_signals
				= cbox->blockSignals(true);
			cbox->setUpdatesEnabled(false);
			cbox->setDuplicatesEnabled(false);
			cbox->clear();
			cbox->addItems(items);
			cbox->setUpdatesEnabled(true);
			cbox->blockSignals(block_signals);
		}
	}
	m_settings->endGroup();

}


void qpwgraph_config::saveComboBoxHistory ( QComboBox *cbox, int nlimit )
{
	int ncount = cbox->count();
	if (ncount > nlimit)
		ncount = nlimit;

	QStringList items;
	for (int i = 0; i < ncount; ++i)
		items.append(cbox->itemText(i));

	m_settings->beginGroup(HistoryGroup);
	m_settings->setValue('/' + cbox->objectName(), items);
	m_settings->endGroup();
}


// end of qpwgraph_config.cpp
