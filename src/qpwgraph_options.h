// qpwgraph_options.h
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

#ifndef __qpwgraph_options_h
#define __qpwgraph_options_h

#include "ui_qpwgraph_options.h"


// Forward decls.
class qpwgraph_main;


//----------------------------------------------------------------------------
// qpwgraph_options -- UI wrapper form.

class qpwgraph_options : public QDialog
{
	Q_OBJECT

public:

	// Constructor.
	qpwgraph_options(qpwgraph_main *parent);
	// Destructor.
	~qpwgraph_options();

protected slots:

	void changed();

	void accept();
	void reject();

	// Filter/hide list management slots.
	void selectFilterNodes();
	void addFilterNodes();
	void removeFilterNodes();
	void clearFilterNodes();
	void changedFilterNodes();

	// Merger/unify list management slots.
	void selectMergerNodes();
	void addMergerNodes();
	void removeMergerNodes();
	void clearMergerNodes();
	void changedMergerNodes();

protected:

	void stabilize();

private:

	// The Qt-designer UI struct...
	Ui::qpwgraph_options m_ui;

	int m_dirty;
	int m_dirty_filter;
	int m_dirty_merger;
};


#endif	// __qpwgraph_options_h


// end of qpwgraph_options.h
