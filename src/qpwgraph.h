// qpwgraph.h
//
/****************************************************************************
   Copyright (C) 2021, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qpwgraph_h
#define __qpwgraph_h

#include "config.h"

#include <QApplication>


// Foward decls.
class QWidget;
class QSharedMemory;
class QLocalServer;


//-------------------------------------------------------------------------
// Singleton application instance - decl.
//

class qpwgraph_application : public QApplication
{
	Q_OBJECT

public:

	// Constructor.
	qpwgraph_application(int& argc, char **argv);

	// Destructor.
	~qpwgraph_application();

	// Main application widget accessors.
	void setMainWidget(QWidget *widget)
		{ m_widget = widget; }
	QWidget *mainWidget() const
		{ return m_widget; }

	// Check if another instance is running,
	// and raise its proper main widget...
	bool setup();

protected slots:

	// Local server slots.
	void newConnectionSlot();
	void readyReadSlot();

private:

	// Instance variables.
	QWidget       *m_widget;
	QString        m_unique;
	QSharedMemory *m_memory;
	QLocalServer  *m_server;
};


#endif	// __qpwgraph_h

// end of qpwgraph.h
