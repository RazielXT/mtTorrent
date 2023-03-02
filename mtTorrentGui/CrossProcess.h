#pragma once

#include <QLocalServer>

class CrossProcess : public QObject
{
	Q_OBJECT
public:
	CrossProcess(QObject* parent);
	~CrossProcess();

	static bool TrySendParams(QStringList);
	void startServer();

	std::function<void(const QStringList&)> onMessage;

private slots:
	void receiveConnection();
private:
	QLocalServer* server = nullptr;
};
