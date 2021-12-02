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

#include <QTextStream>

#ifdef CONFIG_SYSTEM_TRAY
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include <QHostInfo>
#endif


//-------------------------------------------------------------------------
// Singleton application instance - impl.
//

// Constructor.
qpwgraph_application::qpwgraph_application ( int& argc, char **argv )
	: QApplication(argc, argv), m_widget(nullptr)
#ifdef CONFIG_SYSTEM_TRAY
	, m_memory(nullptr), m_server(nullptr)
#endif
{
	QApplication::setApplicationName(PROJECT_NAME);
	QApplication::setApplicationDisplayName(PROJECT_DESCRIPTION);
}


// Destructor.
qpwgraph_application::~qpwgraph_application (void)
{
#ifdef CONFIG_SYSTEM_TRAY
	if (m_server) {
		m_server->close();
		delete m_server;
		m_server = nullptr;
	}
	if (m_memory) {
		delete m_memory;
		m_memory = nullptr;
	}
#endif
}


// Parse command line arguments.
bool qpwgraph_application::parse_args (  )
{
	QTextStream out(stderr);

	const QStringList& args
		= QApplication::arguments();
	const int argc = args.count();

	for (int i = 1; i < argc; ++i) {
		const QString& arg = args.at(i);
		if (arg == "-h" || arg == "--help") {
			print_usage(args.at(0));
			return false;
		}
		else if (arg == "-v" || arg == "--version") {
			out << QString("Qt: %1").arg(qVersion());
		#if defined(QT_STATIC)
			out << "-static";
		#endif
			out << '\n';
			out << QString("%1: %2\n")
				.arg(PROJECT_NAME)
				.arg(PROJECT_VERSION);
			return false;
		}
	}

	// Alright with argument parsing.
	return true;
}


// Help about command line options.
void qpwgraph_application::print_usage ( const QString& arg0 )
{
	QTextStream out(stderr);

	out << PROJECT_NAME " - " << QObject::tr(PROJECT_DESCRIPTION) << '\n'
		<< '\n';
	out << QObject::tr("Usage: %1 [options]").arg(arg0) << '\n'
		<< '\n';
	out << QObject::tr("Options:") << '\n'
		<< '\n';
	out << "  -h, --help" << '\n'
		<< '\t' << QObject::tr("Show help about command line options") << '\n'
		<< '\n';
	out << "  -v, --version" << '\n'
		<< '\t' << QObject::tr("Show version information") << '\n'
		<< '\n';
}


#ifdef CONFIG_SYSTEM_TRAY

// Check if another instance is running,
// and raise its proper main widget...
bool qpwgraph_application::setup (void)
{
	m_unique = QCoreApplication::applicationName();
	m_unique += '@';
	m_unique += QHostInfo::localHostName();
#if defined(Q_OS_UNIX)
	m_memory = new QSharedMemory(m_unique);
	m_memory->attach();
	delete m_memory;
#endif
	m_memory = new QSharedMemory(m_unique);
	bool is_server = false;
	const qint64 pid = QCoreApplication::applicationPid();
	struct Data { qint64 pid; };
	if (m_memory->create(sizeof(Data))) {
		m_memory->lock();
		Data *data = static_cast<Data *> (m_memory->data());
		if (data) {
			data->pid = pid;
			is_server = true;
		}
		m_memory->unlock();
	}
	else
	if (m_memory->attach()) {
		m_memory->lock(); // maybe not necessary?
		Data *data = static_cast<Data *> (m_memory->data());
		if (data)
			is_server = (data->pid == pid);
		m_memory->unlock();
	}

	if (is_server) {
		QLocalServer::removeServer(m_unique);
		m_server = new QLocalServer();
		m_server->setSocketOptions(QLocalServer::UserAccessOption);
		m_server->listen(m_unique);
		QObject::connect(m_server,
			SIGNAL(newConnection()),
			SLOT(newConnectionSlot()));
	} else {
		QLocalSocket socket;
		socket.connectToServer(m_unique);
		if (socket.state() == QLocalSocket::ConnectingState)
			socket.waitForConnected(200);
		if (socket.state() == QLocalSocket::ConnectedState) {
			socket.write(QCoreApplication::arguments().join(' ').toUtf8());
			socket.flush();
			socket.waitForBytesWritten(200);
		}
	}

	return is_server;
}


// Local server conection slot.
void qpwgraph_application::newConnectionSlot (void)
{
	QLocalSocket *socket = m_server->nextPendingConnection();
	QObject::connect(socket,
		SIGNAL(readyRead()),
		SLOT(readyReadSlot()));
}


// Local server data-ready slot.
void qpwgraph_application::readyReadSlot (void)
{
	QLocalSocket *socket = qobject_cast<QLocalSocket *> (sender());
	if (socket) {
		const qint64 nread = socket->bytesAvailable();
		if (nread > 0) {
			const QByteArray data = socket->read(nread);
			// Just make it always shows up fine...
			if (m_widget) {
				m_widget->hide();
				m_widget->show();
				m_widget->raise();
				m_widget->activateWindow();
			}
		}
	}
}

#endif	// CONFIG_SYSTEM_TRAY


//----------------------------------------------------------------------------
// main.


int main ( int argc, char *argv[] )
{
	Q_INIT_RESOURCE(qpwgraph);
#if defined(Q_OS_LINUX)
	::setenv("QT_QPA_PLATFORM", "xcb", 0);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
		Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
#endif

	qpwgraph_application app(argc, argv);

	if (!app.parse_args()) {
		app.quit();
		return 1;
	}

#ifdef CONFIG_SYSTEM_TRAY
	// Have another instance running?
	if (!app.setup()) {
		app.quit();
		return 2;
	}
#endif

	qpwgraph_form form;
	app.setMainWidget(&form);
	form.show();

	return app.exec();
}


// end of qpwgraph.cpp

