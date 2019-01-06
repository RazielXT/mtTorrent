#pragma once

extern void applySettings(System::Object^ form);

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
	public: System::Windows::Forms::CheckBox^  checkBoxDht;
	protected:

	public: System::Windows::Forms::NumericUpDown^  tcpPortNumeric;
	protected:

	public: System::Windows::Forms::NumericUpDown^  udpPortNumeric;
	public: System::Windows::Forms::NumericUpDown^  maxConnectionsNumeric;
	public: System::Windows::Forms::TextBox^  directoryTextBox;

	public:

	public:





	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::Button^  buttonOk;
	private: System::Windows::Forms::Button^  buttonCancel;

	protected:

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
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
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->tcpPortNumeric))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->udpPortNumeric))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->maxConnectionsNumeric))->BeginInit();
			this->SuspendLayout();
			// 
			// checkBoxDht
			// 
			this->checkBoxDht->AutoSize = true;
			this->checkBoxDht->Location = System::Drawing::Point(49, 29);
			this->checkBoxDht->Name = L"checkBoxDht";
			this->checkBoxDht->Size = System::Drawing::Size(52, 21);
			this->checkBoxDht->TabIndex = 0;
			this->checkBoxDht->Text = L"Dht";
			this->checkBoxDht->UseVisualStyleBackColor = true;
			// 
			// tcpPortNumeric
			// 
			this->tcpPortNumeric->Location = System::Drawing::Point(163, 62);
			this->tcpPortNumeric->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 65535, 0, 0, 0 });
			this->tcpPortNumeric->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			this->tcpPortNumeric->Name = L"tcpPortNumeric";
			this->tcpPortNumeric->Size = System::Drawing::Size(120, 22);
			this->tcpPortNumeric->TabIndex = 1;
			this->tcpPortNumeric->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			// 
			// udpPortNumeric
			// 
			this->udpPortNumeric->Location = System::Drawing::Point(163, 90);
			this->udpPortNumeric->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 65535, 0, 0, 0 });
			this->udpPortNumeric->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			this->udpPortNumeric->Name = L"udpPortNumeric";
			this->udpPortNumeric->Size = System::Drawing::Size(120, 22);
			this->udpPortNumeric->TabIndex = 2;
			this->udpPortNumeric->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			// 
			// maxConnectionsNumeric
			// 
			this->maxConnectionsNumeric->Location = System::Drawing::Point(163, 118);
			this->maxConnectionsNumeric->Name = L"maxConnectionsNumeric";
			this->maxConnectionsNumeric->Size = System::Drawing::Size(120, 22);
			this->maxConnectionsNumeric->TabIndex = 3;
			// 
			// directoryTextBox
			// 
			this->directoryTextBox->Location = System::Drawing::Point(115, 157);
			this->directoryTextBox->Name = L"directoryTextBox";
			this->directoryTextBox->Size = System::Drawing::Size(168, 22);
			this->directoryTextBox->TabIndex = 4;
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(46, 64);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(101, 17);
			this->label1->TabIndex = 5;
			this->label1->Text = L"TCP listen port";
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(46, 92);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(103, 17);
			this->label2->TabIndex = 6;
			this->label2->Text = L"UDP listen port";
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(46, 120);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(113, 17);
			this->label3->TabIndex = 7;
			this->label3->Text = L"Max connections";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(44, 157);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(65, 17);
			this->label4->TabIndex = 8;
			this->label4->Text = L"Directory";
			// 
			// buttonOk
			// 
			this->buttonOk->Location = System::Drawing::Point(84, 219);
			this->buttonOk->Name = L"buttonOk";
			this->buttonOk->Size = System::Drawing::Size(75, 23);
			this->buttonOk->TabIndex = 9;
			this->buttonOk->Text = L"Ok";
			this->buttonOk->UseVisualStyleBackColor = true;
			this->buttonOk->Click += gcnew System::EventHandler(this, &SettingsForm::buttonOk_Click);
			// 
			// buttonCancel
			// 
			this->buttonCancel->Location = System::Drawing::Point(165, 219);
			this->buttonCancel->Name = L"buttonCancel";
			this->buttonCancel->Size = System::Drawing::Size(75, 23);
			this->buttonCancel->TabIndex = 10;
			this->buttonCancel->Text = L"Cancel";
			this->buttonCancel->UseVisualStyleBackColor = true;
			this->buttonCancel->Click += gcnew System::EventHandler(this, &SettingsForm::buttonCancel_Click);
			// 
			// SettingsForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(331, 270);
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
			this->Name = L"SettingsForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Settings";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->tcpPortNumeric))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->udpPortNumeric))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->maxConnectionsNumeric))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void folderBrowserDialog1_HelpRequest(System::Object^  sender, System::EventArgs^  e) {
	}
private: System::Void buttonOk_Click(System::Object^  sender, System::EventArgs^  e) {
	applySettings(this);
	Close();
}
private: System::Void buttonCancel_Click(System::Object^  sender, System::EventArgs^  e) {
	Close();
}
};
}
