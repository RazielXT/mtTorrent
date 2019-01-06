#pragma once

extern void onButtonClick(System::Object^ button);

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
			//labelText->Parent = progressBarMagnet;
			labelText->BackColor = Drawing::Color::Transparent;
		}

	public: System::Windows::Forms::Label^  labelText;
	private:
	public:

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
	public: System::Windows::Forms::ProgressBar^  progressBarMagnet;
	protected:

	public: System::Windows::Forms::Button^  magnetFormButton;
	public: System::Windows::Forms::TextBox^  textBoxMagnet;
	private:
	protected:




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
			this->progressBarMagnet = (gcnew System::Windows::Forms::ProgressBar());
			this->magnetFormButton = (gcnew System::Windows::Forms::Button());
			this->textBoxMagnet = (gcnew System::Windows::Forms::TextBox());
			this->labelText = (gcnew System::Windows::Forms::Label());
			this->SuspendLayout();
			// 
			// progressBarMagnet
			// 
			this->progressBarMagnet->Location = System::Drawing::Point(112, 78);
			this->progressBarMagnet->Name = L"progressBarMagnet";
			this->progressBarMagnet->Size = System::Drawing::Size(296, 23);
			this->progressBarMagnet->TabIndex = 0;
			// 
			// magnetFormButton
			// 
			this->magnetFormButton->Location = System::Drawing::Point(15, 78);
			this->magnetFormButton->Name = L"magnetFormButton";
			this->magnetFormButton->Size = System::Drawing::Size(75, 23);
			this->magnetFormButton->TabIndex = 1;
			this->magnetFormButton->Text = L"Add";
			this->magnetFormButton->UseVisualStyleBackColor = true;
			this->magnetFormButton->Click += gcnew System::EventHandler(this, &MagnetInputForm::magnetFormButton_Click);
			// 
			// textBoxMagnet
			// 
			this->textBoxMagnet->Location = System::Drawing::Point(12, 29);
			this->textBoxMagnet->Name = L"textBoxMagnet";
			this->textBoxMagnet->Size = System::Drawing::Size(396, 22);
			this->textBoxMagnet->TabIndex = 2;
			// 
			// labelText
			// 
			this->labelText->Anchor = System::Windows::Forms::AnchorStyles::Top;
			this->labelText->AutoSize = true;
			this->labelText->BackColor = System::Drawing::Color::Transparent;
			this->labelText->Location = System::Drawing::Point(163, 58);
			this->labelText->Name = L"labelText";
			this->labelText->Size = System::Drawing::Size(0, 17);
			this->labelText->TabIndex = 4;
			this->labelText->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// MagnetInputForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackColor = System::Drawing::SystemColors::Control;
			this->ClientSize = System::Drawing::Size(421, 120);
			this->Controls->Add(this->labelText);
			this->Controls->Add(this->textBoxMagnet);
			this->Controls->Add(this->magnetFormButton);
			this->Controls->Add(this->progressBarMagnet);
			this->Name = L"MagnetInputForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Magnet link";
			this->Load += gcnew System::EventHandler(this, &MagnetInputForm::MagnetInputForm_Load);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void magnetFormButton_Click(System::Object^  sender, System::EventArgs^  e) {
		onButtonClick(sender);
	}
	private: System::Void MagnetInputForm_Load(System::Object^  sender, System::EventArgs^  e) {
	}
};
}
