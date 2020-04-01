#pragma once

#include "Buttons.h"

namespace GuiLite {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MagnetInputForm
	/// </summary>
	public ref class MagnetInputForm : public System::Windows::Forms::Form
	{
	public:
		MagnetInputForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			instance = this;
		}

	public: System::Windows::Forms::Label^  labelText;
	public: System::Windows::Forms::TextBox^ textBoxMagnet;
	public: System::Windows::Forms::ProgressBar^ progressBarMagnet;
	public: System::Windows::Forms::Button^ magnetFormButton;
	private: System::Windows::Forms::Panel^ panel1;
	public: System::Windows::Forms::RichTextBox^ logsTextBox;

		static MagnetInputForm^ instance;
	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MagnetInputForm()
		{
			if (components)
			{
				delete components;
			}
		}

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
			this->labelText = (gcnew System::Windows::Forms::Label());
			this->textBoxMagnet = (gcnew System::Windows::Forms::TextBox());
			this->progressBarMagnet = (gcnew System::Windows::Forms::ProgressBar());
			this->magnetFormButton = (gcnew System::Windows::Forms::Button());
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->logsTextBox = (gcnew System::Windows::Forms::RichTextBox());
			this->panel1->SuspendLayout();
			this->SuspendLayout();
			// 
			// labelText
			// 
			this->labelText->AutoSize = true;
			this->labelText->BackColor = System::Drawing::Color::Transparent;
			this->labelText->Location = System::Drawing::Point(225, 49);
			this->labelText->Name = L"labelText";
			this->labelText->Size = System::Drawing::Size(0, 17);
			this->labelText->TabIndex = 4;
			this->labelText->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// textBoxMagnet
			// 
			this->textBoxMagnet->Location = System::Drawing::Point(14, 14);
			this->textBoxMagnet->Name = L"textBoxMagnet";
			this->textBoxMagnet->Size = System::Drawing::Size(456, 22);
			this->textBoxMagnet->TabIndex = 8;
			// 
			// progressBarMagnet
			// 
			this->progressBarMagnet->Location = System::Drawing::Point(125, 42);
			this->progressBarMagnet->Name = L"progressBarMagnet";
			this->progressBarMagnet->Size = System::Drawing::Size(345, 31);
			this->progressBarMagnet->TabIndex = 6;
			// 
			// magnetFormButton
			// 
			this->magnetFormButton->Location = System::Drawing::Point(14, 42);
			this->magnetFormButton->Name = L"magnetFormButton";
			this->magnetFormButton->Size = System::Drawing::Size(105, 31);
			this->magnetFormButton->TabIndex = 7;
			this->magnetFormButton->Text = L"Add";
			this->magnetFormButton->UseVisualStyleBackColor = true;
			this->magnetFormButton->Click += gcnew System::EventHandler(this, &MagnetInputForm::MagnetFormButton_Click_1);
			// 
			// panel1
			// 
			this->panel1->AutoSize = true;
			this->panel1->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
			this->panel1->Controls->Add(this->logsTextBox);
			this->panel1->Location = System::Drawing::Point(14, 71);
			this->panel1->MaximumSize = System::Drawing::Size(455, 400);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(455, 258);
			this->panel1->TabIndex = 10;
			// 
			// logsTextBox
			// 
			this->logsTextBox->Location = System::Drawing::Point(-1, 0);
			this->logsTextBox->Name = L"logsTextBox";
			this->logsTextBox->ReadOnly = true;
			this->logsTextBox->Size = System::Drawing::Size(457, 255);
			this->logsTextBox->TabIndex = 10;
			this->logsTextBox->Text = L"";
			this->logsTextBox->Visible = false;
			// 
			// MagnetInputForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->AutoSize = true;
			this->BackColor = System::Drawing::SystemColors::Control;
			this->ClientSize = System::Drawing::Size(482, 85);
			this->Controls->Add(this->labelText);
			this->Controls->Add(this->panel1);
			this->Controls->Add(this->textBoxMagnet);
			this->Controls->Add(this->progressBarMagnet);
			this->Controls->Add(this->magnetFormButton);
			this->MaximizeBox = false;
			this->MaximumSize = System::Drawing::Size(500, 600);
			this->MinimizeBox = false;
			this->Name = L"MagnetInputForm";
			this->ShowIcon = false;
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Magnet link";
			this->panel1->ResumeLayout(false);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
private: System::Void MagnetFormButton_Click_1(System::Object^ sender, System::EventArgs^ e) {
	onButtonClick(ButtonId::MagnetButton);
}
};
}
