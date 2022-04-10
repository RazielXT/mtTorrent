#include "Settings.h"
#include "../AppCore.h"

Settings::Settings(AppCore& c) : core(c)
{
}

void Settings::applySettings(GuiLite::SettingsForm^ form)
{
	mtBI::SettingsInfo info;
	info.dhtEnabled = form->checkBoxDht->Checked;
	info.directory = getUtf8String(form->directoryTextBox->Text);
	info.maxConnections = (unsigned int)form->maxConnectionsNumeric->Value;
	info.udpPort = (unsigned int)form->udpPortNumeric->Value;
	info.tcpPort = (unsigned int)form->tcpPortNumeric->Value;
	info.upnpEnabled = form->upnpMapCheckBox->Checked;
	info.maxDownloadSpeed = (unsigned int)form->numericDlSpeed->Value * 1024;
	info.maxUploadSpeed = (unsigned int)form->numericUpSpeed->Value * 1024;
	info.utpEnabled = form->utpCheckBox->Checked;
	info.encryption = (mtBI::SettingsInfo::ProtocolEncryption)form->encryptComboBox->SelectedIndex;

	core.IoctlFunc(mtBI::MessageId::SetSettings, &info, nullptr);
}

void Settings::showSettingsForm()
{
	GuiLite::SettingsForm form;
	mtBI::SettingsInfo info;
	if (core.IoctlFunc(mtBI::MessageId::GetSettings, nullptr, &info) == mtt::Status::Success)
	{
		form.checkBoxDht->Checked = info.dhtEnabled;
		form.directoryTextBox->Text = gcnew System::String(info.directory.data);
		form.maxConnectionsNumeric->Value = info.maxConnections;
		form.udpPortNumeric->Value = info.udpPort;
		form.tcpPortNumeric->Value = info.tcpPort;
		form.upnpMapCheckBox->Checked = info.upnpEnabled;
		form.numericDlSpeed->Value = info.maxDownloadSpeed / 1024;
		form.numericUpSpeed->Value = info.maxUploadSpeed / 1024;
		form.utpCheckBox->Checked = info.utpEnabled;
		form.encryptComboBox->SelectedIndex = (int)info.encryption;
	}

	form.ShowDialog();
}
