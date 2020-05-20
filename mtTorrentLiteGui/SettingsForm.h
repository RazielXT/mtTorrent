#pragma once

namespace GuiLite {
	ref class SettingsForm;
}

extern void applySettings(GuiLite::SettingsForm^ form);
extern System::String^ getUpnpInfo();

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
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->tcpPortNumeric))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->udpPortNumeric))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->maxConnectionsNumeric))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->numericUpSpeed))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->numericDlSpeed))->BeginInit();
			this->SuspendLayout();
			this->checkBoxDht->AutoSize = true;
			this->checkBoxDht->Location = System::Drawing::Point(58, 276);
			this->checkBoxDht->Name = L"checkBoxDht";
			this->checkBoxDht->Size = System::Drawing::Size(59, 21);
			this->checkBoxDht->TabIndex = 0;
			this->checkBoxDht->Text = L"DHT";
			this->checkBoxDht->UseVisualStyleBackColor = true;
			this->tcpPortNumeric->Location = System::Drawing::Point(228, 85);
			this->tcpPortNumeric->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 65535, 0, 0, 0 });
			this->tcpPortNumeric->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			this->tcpPortNumeric->Name = L"tcpPortNumeric";
			this->tcpPortNumeric->Size = System::Drawing::Size(120, 22);
			this->tcpPortNumeric->TabIndex = 1;
			this->tcpPortNumeric->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			this->udpPortNumeric->Location = System::Drawing::Point(228, 113);
			this->udpPortNumeric->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 65535, 0, 0, 0 });
			this->udpPortNumeric->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			this->udpPortNumeric->Name = L"udpPortNumeric";
			this->udpPortNumeric->Size = System::Drawing::Size(120, 22);
			this->udpPortNumeric->TabIndex = 2;
			this->udpPortNumeric->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			this->maxConnectionsNumeric->Location = System::Drawing::Point(228, 141);
			this->maxConnectionsNumeric->Name = L"maxConnectionsNumeric";
			this->maxConnectionsNumeric->Size = System::Drawing::Size(120, 22);
			this->maxConnectionsNumeric->TabIndex = 3;
			this->directoryTextBox->Location = System::Drawing::Point(59, 47);
			this->directoryTextBox->Name = L"directoryTextBox";
			this->directoryTextBox->Size = System::Drawing::Size(291, 22);
			this->directoryTextBox->TabIndex = 4;
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(56, 85);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(101, 17);
			this->label1->TabIndex = 5;
			this->label1->Text = L"TCP listen port";
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(56, 113);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(103, 17);
			this->label2->TabIndex = 6;
			this->label2->Text = L"UDP listen port";
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(56, 143);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(113, 17);
			this->label3->TabIndex = 7;
			this->label3->Text = L"Max connections";
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(56, 24);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(112, 17);
			this->label4->TabIndex = 8;
			this->label4->Text = L"Default directory";
			this->buttonOk->Location = System::Drawing::Point(119, 326);
			this->buttonOk->Name = L"buttonOk";
			this->buttonOk->Size = System::Drawing::Size(81, 27);
			this->buttonOk->TabIndex = 9;
			this->buttonOk->Text = L"OK";
			this->buttonOk->UseVisualStyleBackColor = true;
			this->buttonOk->Click += gcnew System::EventHandler(this, &SettingsForm::buttonOk_Click);
			this->buttonCancel->Location = System::Drawing::Point(206, 326);
			this->buttonCancel->Name = L"buttonCancel";
			this->buttonCancel->Size = System::Drawing::Size(84, 27);
			this->buttonCancel->TabIndex = 10;
			this->buttonCancel->Text = L"Cancel";
			this->buttonCancel->UseVisualStyleBackColor = true;
			this->buttonCancel->Click += gcnew System::EventHandler(this, &SettingsForm::buttonCancel_Click);
			this->upnpMapCheckBox->AutoSize = true;
			this->upnpMapCheckBox->Location = System::Drawing::Point(58, 249);
			this->upnpMapCheckBox->Name = L"upnpMapCheckBox";
			this->upnpMapCheckBox->Size = System::Drawing::Size(151, 21);
			this->upnpMapCheckBox->TabIndex = 11;
			this->upnpMapCheckBox->Text = L"Upnp port mapping";
			this->upnpMapCheckBox->UseVisualStyleBackColor = true;
			this->upnpMapCheckBox->CheckedChanged += gcnew System::EventHandler(this, &SettingsForm::upnpMapCheckBox_CheckedChanged);
			this->upnpToolTip->AutomaticDelay = 100;
			this->upnpToolTip->AutoPopDelay = 10000;
			this->upnpToolTip->InitialDelay = 100;
			this->upnpToolTip->ReshowDelay = 20;
			this->upnpToolTip->ShowAlways = true;
			this->upnpToolTip->Popup += gcnew System::Windows::Forms::PopupEventHandler(this, &SettingsForm::UpnpToolTip_Popup);
			this->dirButton->Location = System::Drawing::Point(273, 19);
			this->dirButton->Name = L"dirButton";
			this->dirButton->Size = System::Drawing::Size(77, 27);
			this->dirButton->TabIndex = 12;
			this->dirButton->Text = L"Browse";
			this->dirButton->UseVisualStyleBackColor = true;
			this->dirButton->Click += gcnew System::EventHandler(this, &SettingsForm::dirButton_Click);
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(56, 205);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(113, 17);
			this->label5->TabIndex = 16;
			this->label5->Text = L"Max upload KB/s";
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(56, 177);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(130, 17);
			this->label6->TabIndex = 15;
			this->label6->Text = L"Max download KB/s";
			this->numericUpSpeed->Location = System::Drawing::Point(228, 205);
			this->numericUpSpeed->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 65535, 0, 0, 0 });
			this->numericUpSpeed->Name = L"numericUpSpeed";
			this->numericUpSpeed->Size = System::Drawing::Size(120, 22);
			this->numericUpSpeed->TabIndex = 14;
			this->numericDlSpeed->Location = System::Drawing::Point(228, 177);
			this->numericDlSpeed->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 65535, 0, 0, 0 });
			this->numericDlSpeed->Name = L"numericUpSpeed";
			this->numericDlSpeed->Size = System::Drawing::Size(120, 22);
			this->numericDlSpeed->TabIndex = 13;
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(400, 365);
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
};
}
