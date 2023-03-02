#include "AddPeerWindow.h"
#include "QToolTip"

AddPeerWindow::AddPeerWindow(mttApi::TorrentPtr t, QWidget *parent)
	: QDialog(parent), torrent(t)
{
	ui.setupUi(this);
	ui.peerEdit->setText(app::getSetting("addPeer").toString());

	connect(ui.addButton, &QPushButton::clicked, [this]() 
		{
			if (!ui.peerEdit->text().isEmpty())
			{
				if (torrent->getPeers().connect(ui.peerEdit->text().toLocal8Bit().data()) == mtt::Status::E_InvalidInput)
				{
					QPoint point = QPoint(geometry().left() + ui.peerEdit->geometry().left(), geometry().top() + ui.peerEdit->geometry().bottom());
					QToolTip::showText(point, "Invalid address");
				}
				else
				{
					app::setSetting("addPeer", ui.peerEdit->text());
					close();
				}
			}
		});
}

AddPeerWindow::~AddPeerWindow()
{
}
