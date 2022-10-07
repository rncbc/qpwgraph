// qpwgraph_alsamidi.h
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

#ifndef __qpwgraph_alsamidi_h
#define __qpwgraph_alsamidi_h

#include "config.h"
#include "qpwgraph_sect.h"


#ifdef CONFIG_ALSA_MIDI

#include <alsa/asoundlib.h>

#include <QMutex>


// Forwards decls.
class QSocketNotifier;


//----------------------------------------------------------------------------
// qpwgraph_alsamidi -- ALSA graph driver

class qpwgraph_alsamidi : public qpwgraph_sect
{
	Q_OBJECT

public:

	// Constructor.
	qpwgraph_alsamidi(qpwgraph_canvas *canvas);

	// Destructor.
	~qpwgraph_alsamidi();

	// Client methods.
	bool open();
	void close();

	// ALSA port (dis)connection.
	void connectPorts(qpwgraph_port *port1, qpwgraph_port *port2, bool is_connect);

	// ALSA graph updaters.
	void updateItems();
	void clearItems();

	// Special port-type colors defaults (virtual).
	void resetPortTypeColors();

	// ALSA node type inquirer.
	static bool isNodeType(uint node_type);
	// ALSA node type.
	static uint nodeType();

	// ALSA port type inquirer.
	static bool isPortType(uint port_type);
	// ALSA port type.
	static uint midiPortType();

signals:

	void changed();

protected slots:

	// Callback notifiers.
	void changedNotify();

protected:

	// ALSA client:port finder and creator if not existing.
	bool findClientPort(snd_seq_client_info_t *client_info,
		snd_seq_port_info_t *port_info, qpwgraph_item::Mode port_mode,
		qpwgraph_node **node, qpwgraph_port **port, bool add_new);

private:

	// Instance variables.
	snd_seq_t *m_seq;

	QSocketNotifier *m_notifier;

	// Notifier sanity mutex.
	static QMutex g_mutex;
};


#endif	// CONFIG_ALSA_MIDI

#endif	// __qpwgraph_alsamidi_h

// end of qpwgraph_alsamidi.h
