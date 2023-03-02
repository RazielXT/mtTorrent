#include "CrossProcess.h"
#include <QLocalSocket>
#include <QFile>
#include "QMessageBox"

static const char* SocketName = "MTTQAPP";
static const char* OpenCommand = "Open";
static const char* ParamSeparator = "|&|";

CrossProcess::CrossProcess(QObject* parent) : QObject(parent), server(new QLocalServer(this))
{
}

CrossProcess::~CrossProcess()
{
	server->close();
}

bool CrossProcess::TrySendParams(QStringList params)
{
	QByteArray uMsg;
	if (params.isEmpty())
		uMsg.append(OpenCommand);
	else
		uMsg.append(params.join(ParamSeparator).toLocal8Bit());

	QLocalSocket socket;
	socket.connectToServer(SocketName, QIODeviceBase::WriteOnly);

	int timeout = 1000;
	if (!socket.waitForConnected(timeout))
		return false;

	QDataStream ds(&socket);
	ds.writeBytes(uMsg.constData(), uMsg.size());

	return socket.waitForBytesWritten(timeout);
}

void CrossProcess::startServer()
{
	bool res = server->listen(SocketName);
#if defined(Q_OS_UNIX)
	if (!res && server->serverError() == QAbstractSocket::AddressInUseError)
	{
		QFile::remove(SocketName);
		res = server->listen(SocketName);
	}
#endif

	connect(server, &QLocalServer::newConnection, this, &CrossProcess::receiveConnection);
}

void CrossProcess::receiveConnection()
{
	QLocalSocket* socket = server->nextPendingConnection();
	if (!socket)
		return;

	while (socket->state() != QLocalSocket::UnconnectedState && !socket->bytesAvailable())
		socket->waitForReadyRead();

	if (socket->state() == QLocalSocket::UnconnectedState)
		return;

	QDataStream ds(socket);
	QByteArray uMsg;
	quint32 remaining;
	ds >> remaining;
	if (remaining > 65535)
	{
		delete socket;
		return;
	}

	uMsg.resize(remaining);
	int got = 0;
	char* uMsgBuf = uMsg.data();
	do
	{
		got = ds.readRawData(uMsgBuf, remaining);
		remaining -= got;
		uMsgBuf += got;
	} while (remaining && got >= 0 && socket->waitForReadyRead(2000));

	delete socket;
	if (got > 0 && onMessage)
	{
		auto list = QString::fromUtf8(uMsg).split(ParamSeparator);
		if (list.size() == 1 && list.front() == OpenCommand)
			list.clear();

		onMessage(list);
	}
}
