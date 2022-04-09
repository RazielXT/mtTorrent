#pragma once

#include "../AppCoreCallbacks.h"
extern void RegisterMagnetAssociation(long long parent);

namespace GuiLite {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for SettingsForm
	/// </summary>
	public ref class SettingsForm : public System::Windows::Forms::Form
	{
	public:
		SettingsForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			upnpToolTip->SetToolTip(upnpMapCheckBox, "");
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~SettingsForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::ToolTip^ upnpToolTip;
	public: System::Windows::Forms::CheckBox^  checkBoxDht;
	public: System::Windows::Forms::NumericUpDown^  tcpPortNumeric;
	public: System::Windows::Forms::NumericUpDown^  udpPortNumeric;
	public: System::Windows::Forms::NumericUpDown^  maxConnectionsNumeric;
	public: System::Windows::Forms::TextBox^  directoryTextBox;
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::Button^  buttonOk;
	private: System::Windows::Forms::Button^  buttonCancel;
	public: System::Windows::Forms::CheckBox^ upnpMapCheckBox;
	private: System::Windows::Forms::Button^ dirButton;
	private: System::Windows::Forms::Label^ label5;
	private: System::Windows::Forms::Label^ label6;
	public: System::Windows::Forms::NumericUpDown^ numericUpSpeed;
	private:

	public: System::Windows::Forms::NumericUpDown^ numericDlSpeed;
	private: System::Windows::Forms::Button^ buttonMagnet;
	public: System::Windows::Forms::CheckBox^ utpCheckBox;
	private:

	private:
	public:

	public:
	private:


	private:


	private:



	private:

	public:

	public:
	private: System::ComponentModel::IContainer^ components;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->checkBoxDht = (gcnew System::Windows::Forms::CheckBox());
			this->tcpPortNumeric = (gcnew System::Windows::Forms::NumericUpDown());
			this->udpPortNumeric = (gcnew System::Windows::Forms::NumericUpDown());
			this->maxConnectionsNumeric = (gcnew System::Windows::Forms::NumericUpDown());
			this->directoryTextBox = (gcnew System::Windows::Forms::TextBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->buttonOk = (gcnew System::Windows::Forms::Button());
			this->buttonCancel = (gcnew System::Windows::Forms::Button());
			this->upnpMapCheckBox = (gcnew System::Windows::Forms::CheckBox());
			this->upnpToolTip = (gcnew System::Windows::Forms::ToolTip(this->components));
			this->dirButton = (gcnew System::Windows::Forms::Button());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->numericUpSpeed = (gcnew System::Windows::Forms::NumericUpDown());
			this->numericDlSpeed = (gcnew System::Windows::Forms::NumericUpDown());
			this->buttonMagnet = (gcnew System::Windows::Forms::Button());
			this->utpCheckBox = (gcnew System::Windows::Forms::CheckBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->tcpPortNumeric))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->udpPortNumeric))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->maxConnectionsNumeric))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->numericUpSpeed))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->numericDlSpeed))->BeginInit();
			this->SuspendLayout();
			// 
			// checkBoxDht
			// 
			this->checkBoxDht->AutoSize = true;
			this->checkBoxDht->Location = System::Drawing::Point(65, 345);
			this->checkBoxDht->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->checkBoxDht->Name = L"checkBoxDht";
			this->checkBoxDht->Size = System::Drawing::Size(68, 24);
			this->checkBoxDht->TabIndex = 0;
			this->checkBoxDht->Text = L"DHT";
			this->checkBoxDht->UseVisualStyleBackColor = true;
			// 
			// tcpPortNumeric
			// 
			this->tcpPortNumeric->Location = System::Drawing::Point(256, 106);
			this->tcpPortNumeric->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->tcpPortNumeric->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 65535, 0, 0, 0 });
			this->tcpPortNumeric->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			this->tcpPortNumeric->Name = L"tcpPortNumeric";
			this->tcpPortNumeric->Size = System::Drawing::Size(135, 26);
			this->tcpPortNumeric->TabIndex = 1;
			this->tcpPortNumeric->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			// 
			// udpPortNumeric
			// 
			this->udpPortNumeric->Location = System::Drawing::Point(256, 141);
			this->udpPortNumeric->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->udpPortNumeric->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 65535, 0, 0, 0 });
			this->udpPortNumeric->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			this->udpPortNumeric->Name = L"udpPortNumeric";
			this->udpPortNumeric->Size = System::Drawing::Size(135, 26);
			this->udpPortNumeric->TabIndex = 2;
			this->udpPortNumeric->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			// 
			// maxConnectionsNumeric
			// 
			this->maxConnectionsNumeric->Location = System::Drawing::Point(256, 176);
			this->maxConnectionsNumeric->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->maxConnectionsNumeric->Name = L"maxConnectionsNumeric";
			this->maxConnectionsNumeric->Size = System::Drawing::Size(135, 26);
			this->maxConnectionsNumeric->TabIndex = 3;
			// 
			// directoryTextBox
			// 
			this->directoryTextBox->Location = System::Drawing::Point(66, 59);
			this->directoryTextBox->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->directoryTextBox->Name = L"directoryTextBox";
			this->directoryTextBox->Size = System::Drawing::Size(327, 26);
			this->directoryTextBox->TabIndex = 4;
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(63, 106);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(112, 20);
			this->label1->TabIndex = 5;
			this->label1->Text = L"TCP listen port";
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(63, 141);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(116, 20);
			this->label2->TabIndex = 6;
			this->label2->Text = L"UDP listen port";
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(63, 179);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(128, 20);
			this->label3->TabIndex = 7;
			this->label3->Text = L"Max connections";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(63, 30);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(125, 20);
			this->label4->TabIndex = 8;
			this->label4->Text = L"Default directory";
			// 
			// buttonOk
			// 
			this->buttonOk->Location = System::Drawing::Point(133, 478);
			this->buttonOk->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->buttonOk->Name = L"buttonOk";
			this->buttonOk->Size = System::Drawing::Size(91, 34);
			this->buttonOk->TabIndex = 9;
			this->buttonOk->Text = L"OK";
			this->buttonOk->UseVisualStyleBackColor = true;
			this->buttonOk->Click += gcnew System::EventHandler(this, &SettingsForm::buttonOk_Click);
			// 
			// buttonCancel
			// 
			this->buttonCancel->Location = System::Drawing::Point(231, 478);
			this->buttonCancel->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->buttonCancel->Name = L"buttonCancel";
			this->buttonCancel->Size = System::Drawing::Size(94, 34);
			this->buttonCancel->TabIndex = 10;
			this->buttonCancel->Text = L"Cancel";
			this->buttonCancel->UseVisualStyleBackColor = true;
			this->buttonCancel->Click += gcnew System::EventHandler(this, &SettingsForm::buttonCancel_Click);
			// 
			// upnpMapCheckBox
			// 
			this->upnpMapCheckBox->AutoSize = true;
			this->upnpMapCheckBox->Location = System::Drawing::Point(65, 311);
			this->upnpMapCheckBox->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->upnpMapCheckBox->Name = L"upnpMapCheckBox";
			this->upnpMapCheckBox->Size = System::Drawing::Size(171, 24);
			this->upnpMapCheckBox->TabIndex = 11;
			this->upnpMapCheckBox->Text = L"Upnp port mapping";
			this->upnpMapCheckBox->UseVisualStyleBackColor = true;
			this->upnpMapCheckBox->CheckedChanged += gcnew System::EventHandler(this, &SettingsForm::upnpMapCheckBox_CheckedChanged);
			// 
			// upnpToolTip
			// 
			this->upnpToolTip->AutomaticDelay = 100;
			this->upnpToolTip->AutoPopDelay = 10000;
			this->upnpToolTip->InitialDelay = 100;
			this->upnpToolTip->ReshowDelay = 20;
			this->upnpToolTip->ShowAlways = true;
			this->upnpToolTip->Popup += gcnew System::Windows::Forms::PopupEventHandler(this, &SettingsForm::UpnpToolTip_Popup);
			// 
			// dirButton
			// 
			this->dirButton->Location = System::Drawing::Point(307, 24);
			this->dirButton->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->dirButton->Name = L"dirButton";
			this->dirButton->Size = System::Drawing::Size(87, 34);
			this->dirButton->TabIndex = 12;
			this->dirButton->Text = L"Browse";
			this->dirButton->UseVisualStyleBackColor = true;
			this->dirButton->Click += gcnew System::EventHandler(this, &SettingsForm::dirButton_Click);
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(63, 256);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(127, 20);
			this->label5->TabIndex = 16;
			this->label5->Text = L"Max upload KB/s";
			// 
			// label6
			// 
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(63, 221);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(147, 20);
			this->label6->TabIndex = 15;
			this->label6->Text = L"Max download KB/s";
			// 
			// numericUpSpeed
			// 
			this->numericUpSpeed->Location = System::Drawing::Point(256, 256);
			this->numericUpSpeed->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->numericUpSpeed->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 65535, 0, 0, 0 });
			this->numericUpSpeed->Name = L"numericUpSpeed";
			this->numericUpSpeed->Size = System::Drawing::Size(135, 26);
			this->numericUpSpeed->TabIndex = 14;
			// 
			// numericDlSpeed
			// 
			this->numericDlSpeed->Location = System::Drawing::Point(256, 221);
			this->numericDlSpeed->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->numericDlSpeed->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 65535, 0, 0, 0 });
			this->numericDlSpeed->Name = L"numericDlSpeed";
			this->numericDlSpeed->Size = System::Drawing::Size(135, 26);
			this->numericDlSpeed->TabIndex = 13;
			// 
			// buttonMagnet
			// 
			this->buttonMagnet->Location = System::Drawing::Point(65, 421);
			this->buttonMagnet->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->buttonMagnet->Name = L"buttonMagnet";
			this->buttonMagnet->Size = System::Drawing::Size(327, 34);
			this->buttonMagnet->TabIndex = 17;
			this->buttonMagnet->Text = L"Associate with magnet links";
			this->buttonMagnet->UseVisualStyleBackColor = true;
			this->buttonMagnet->Click += gcnew System::EventHandler(this, &SettingsForm::buttonMagnet_Click);
			// 
			// utpCheckBox
			// 
			this->utpCheckBox->AutoSize = true;
			this->utpCheckBox->Location = System::Drawing::Point(65, 377);
			this->utpCheckBox->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->utpCheckBox->Name = L"utpCheckBox";
			this->utpCheckBox->Size = System::Drawing::Size(167, 24);
			this->utpCheckBox->TabIndex = 18;
			this->utpCheckBox->Text = L"uTP (experimental)";
			this->utpCheckBox->UseVisualStyleBackColor = true;
			// 
			// SettingsForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(9, 20);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(450, 528);
			this->Controls->Add(this->utpCheckBox);
			this->Controls->Add(this->buttonMagnet);
			this->Controls->Add(this->label5);
			this->Controls->Add(this->label6);
			this->Controls->Add(this->numericUpSpeed);
			this->Controls->Add(this->numericDlSpeed);
			this->Controls->Add(this->dirButton);
			this->Controls->Add(this->upnpMapCheckBox);
			this->Controls->Add(this->buttonCancel);
			this->Controls->Add(this->buttonOk);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->directoryTextBox);
			this->Controls->Add(this->maxConnectionsNumeric);
			this->Controls->Add(this->udpPortNumeric);
			this->Controls->Add(this->tcpPortNumeric);
			this->Controls->Add(this->checkBoxDht);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
			this->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"SettingsForm";
			this->ShowIcon = false;
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Settings";
			this->TopMost = true;
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->tcpPortNumeric))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->udpPortNumeric))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->maxConnectionsNumeric))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->numericUpSpeed))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->numericDlSpeed))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
private: System::Void buttonOk_Click(System::Object^  sender, System::EventArgs^  e) {
	applySettings(this);
	Close();
}
private: System::Void buttonCancel_Click(System::Object^  sender, System::EventArgs^  e) {
	Close();
}
private: bool requestTooltip = false;
private: System::Void UpnpToolTip_Popup(System::Object^ sender, System::Windows::Forms::PopupEventArgs^ e) {

	if (!requestTooltip)
	{
		requestTooltip = true;
		upnpToolTip->SetToolTip(upnpMapCheckBox, getUpnpInfo());
	}
	else
		requestTooltip = false;
}
private: System::Void upnpMapCheckBox_CheckedChanged(System::Object^ sender, System::EventArgs^ e) {
	upnpToolTip->Show("", upnpMapCheckBox);
}
private: System::Void dirButton_Click(System::Object^ sender, System::EventArgs^ e) {
	System::Windows::Forms::FolderBrowserDialog dialog;
	auto result = dialog.ShowDialog(this);

	if (result == System::Windows::Forms::DialogResult::OK && dialog.SelectedPath->Length > 0)
	{
		directoryTextBox->Text = dialog.SelectedPath;
	}
}
private: System::Void buttonMagnet_Click(System::Object^ sender, System::EventArgs^ e) {
	RegisterMagnetAssociation(this->Handle.ToInt64());
}

};
}
