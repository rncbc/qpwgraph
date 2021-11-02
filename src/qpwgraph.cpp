// qpwgraph.cpp
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

#include "qpwgraph.h"
#include "qpwgraph_form.h"

#include <QApplication>

//----------------------------------------------------------------------------
// main.


int main ( int argc, char *argv[] )
{
	Q_INIT_RESOURCE(qpwgraph);
#if defined(Q_OS_LINUX)
	::setenv("QT_QPA_PLATFORM", "xcb", 0);
#endif
	QApplication app(argc, argv);

	qpwgraph_form form;
	form.show();

	return app.exec();
}


// end of qpwgraph.cpp

