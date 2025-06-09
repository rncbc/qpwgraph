// qpwgraph_patchman.h
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

#ifndef __qpwgraph_patchman_h
#define __qpwgraph_patchman_h

#include "qpwgraph_patchbay.h"

#include <QDialog>

#include <QHash>


// Forward decls.
class QPushButton;
class QDialogButtonBox;


//----------------------------------------------------------------------------
// qpwgraph_patchman -- main dialog decl.

class qpwgraph_patchman : public QDialog
{
	Q_OBJECT

public:

	// Constructor.
	qpwgraph_patchman(QWidget *parent);
	// Destructor.
	~qpwgraph_patchman();

	// Patchbay accessors.
	void setPatchbay(qpwgraph_patchbay *patchbay);
	qpwgraph_patchbay *patchbay() const;

	// Patchbay view refresh.
	void refresh();

protected slots:

	void addClicked();
	void removeClicked();
	void removeAllClicked();
	void cleanupClicked();
	void resetClicked();

	void accept();
	void reject();

	void stabilize();

protected:

	// Forward decls.
	class MainWidget;
	class TreeWidget;
	class LineWidget;

	class ItemDelegate;

private:

	// Instance members.
	qpwgraph_patchbay *m_patchbay;

	int m_dirty;

	MainWidget *m_main;

	QPushButton *m_add_button;
	QPushButton *m_remove_button;
	QPushButton *m_remove_all_button;
	QPushButton *m_cleanup_button;
	QPushButton *m_reset_button;

	QDialogButtonBox *m_button_box;
};


#endif	// __qpwgraph_patchman_h


// end of qpwgraph_patchman.h
