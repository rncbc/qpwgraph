// qpwgraph_options.cpp
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

#include "qpwgraph_options.h"

#include "qpwgraph_main.h"
#include "qpwgraph_config.h"

#include <QMessageBox>
#include <QPushButton>

#ifdef CONFIG_SYSTEM_TRAY
#include <QSystemTrayIcon>
#endif


//----------------------------------------------------------------------------
// qpwgraph_options -- UI wrapper form.

// Constructor.
qpwgraph_options::qpwgraph_options ( qpwgraph_main *parent )
	: QDialog(parent)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// Initialize dirty control state.
	m_dirty = 0;

	// Setup current options...
	qpwgraph_config *config = parent->config();
	if (config) {
	#ifdef CONFIG_SYSTEM_TRAY
		m_ui.SystemTrayEnabledCheckBox->setChecked(
			config->isSystemTrayEnabled());
		m_ui.SystemTrayQueryCloseCheckBox->setChecked(
			config->isSystemTrayQueryClose());
		m_ui.SystemTrayStartMinimizedCheckBox->setChecked(
			config->isStartMinimized());
	#endif
		m_ui.PatchbayQueryQuitCheckBox->setChecked(
			config->isPatchbayQueryQuit());
	#ifdef CONFIG_ALSA_MIDI
		m_ui.AlsaMidiEnabledCheckBox->setChecked(
			config->isAlsaMidiEnabled());
	#endif
	}

#ifdef CONFIG_SYSTEM_TRAY
	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		m_ui.SystemTrayEnabledCheckBox->setEnabled(false);
		m_ui.SystemTrayQueryCloseCheckBox->setEnabled(false);
		m_ui.SystemTrayStartMinimizedCheckBox->setChecked(false);
	}
#else
	m_ui.SystemTrayEnabledCheckBox->hide();
	m_ui.SystemTrayQueryCloseCheckBox->hide();
	m_ui.SystemTrayStartMinimizedCheckBox->hide();
#endif

#ifndef CONFIG_ALSA_MIDI
	m_ui.AlsaMidiEnabledCheckBox->hide();
#endif

	// Try to restore old window positioning.
	adjustSize();

	// UI connections...
#ifdef CONFIG_SYSTEM_TRAY
	QObject::connect(m_ui.SystemTrayEnabledCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(changed()));
	QObject::connect(m_ui.SystemTrayQueryCloseCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(changed()));
	QObject::connect(m_ui.SystemTrayStartMinimizedCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(changed()));
#endif
	QObject::connect(m_ui.PatchbayQueryQuitCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(changed()));
#ifdef CONFIG_ALSA_MIDI
	QObject::connect(m_ui.AlsaMidiEnabledCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(changed()));
#endif
	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(accepted()),
		SLOT(accept()));
	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(rejected()),
		SLOT(reject()));

	// Ready?
	stabilize();
}


// Destructor.
qpwgraph_options::~qpwgraph_options (void)
{
}


// Reject options (Cancel button slot).
void qpwgraph_options::reject (void)
{
	bool ret = true;

	// Check if there's any pending changes...
	if (m_dirty > 0) {
		switch (QMessageBox::warning(this,
			tr("Warning"),
			tr("Some options have been changed.") + "\n\n" +
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


// Accept options (OK button slot).
void qpwgraph_options::accept (void)
{
	bool ret = true;

	qpwgraph_config *config = nullptr;
	qpwgraph_main *parent = qobject_cast<qpwgraph_main *> (parentWidget());
	if (parent)
		config = parent->config();
	if (config) {
	#ifdef CONFIG_SYSTEM_TRAY
		config->setSystemTrayEnabled(
			m_ui.SystemTrayEnabledCheckBox->isChecked());
		config->setSystemTrayQueryClose(
			m_ui.SystemTrayQueryCloseCheckBox->isChecked());
		config->setStartMinimized(
			m_ui.SystemTrayStartMinimizedCheckBox->isChecked());
	#endif
		config->setPatchbayQueryQuit(
			m_ui.PatchbayQueryQuitCheckBox->isChecked());
	#ifdef CONFIG_ALSA_MIDI
		config->setAlsaMidiEnabled(
			m_ui.AlsaMidiEnabledCheckBox->isChecked());
	#endif
		parent->updateOptions();
	}

	QDialog::accept();
}


// Dirty up options.
void qpwgraph_options::changed (void)
{
	++m_dirty;

	stabilize();
}


// Stabilize current form state.
void qpwgraph_options::stabilize (void)
{
#ifdef CONFIG_SYSTEM_TRAY
	const bool systray = m_ui.SystemTrayEnabledCheckBox->isChecked();
	m_ui.SystemTrayQueryCloseCheckBox->setEnabled(systray);
	m_ui.SystemTrayStartMinimizedCheckBox->setEnabled(systray);
#endif

	m_ui.DialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(m_dirty > 0);
}


// end of qpwgraph_options.cpp
