#pragma once

#include "../AppCoreCallbacks.h"

namespace GuiLite {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for AddPeerForm
	/// </summary>
	public ref class AddPeerForm : public System::Windows::Forms::Form
	{
	public:
		AddPeerForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//

			if (auto val = getWindowState()->addPeer)
				textBox1->Text = val;
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~AddPeerForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Button^ addPeerButton;
	protected:
	private: System::Windows::Forms::TextBox^ textBox1;
	private: System::Windows::Forms::Label^ label1;

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
			this->addPeerButton = (gcnew System::Windows::Forms::Button());
			this->textBox1 = (gcnew System::Windows::Forms::TextBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->SuspendLayout();
			// 
			// addPeerButton
			// 
			this->addPeerButton->Location = System::Drawing::Point(266, 28);
			this->addPeerButton->Name = L"addPeerButton";
			this->addPeerButton->Size = System::Drawing::Size(75, 23);
			this->addPeerButton->TabIndex = 0;
			this->addPeerButton->Text = L"Add";
			this->addPeerButton->UseVisualStyleBackColor = true;
			this->addPeerButton->Click += gcnew System::EventHandler(this, &AddPeerForm::AddPeerButton_Click);
			// 
			// textBox1
			// 
			this->textBox1->Location = System::Drawing::Point(27, 29);
			this->textBox1->Name = L"textBox1";
			this->textBox1->Size = System::Drawing::Size(233, 22);
			this->textBox1->TabIndex = 1;
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(24, 9);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(50, 17);
			this->label1->TabIndex = 2;
			this->label1->Text = L"IP:Port";
			// 
			// AddPeerForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(365, 66);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->textBox1);
			this->Controls->Add(this->addPeerButton);
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"AddPeerForm";
			this->ShowIcon = false;
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Add Peer";
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void AddPeerButton_Click(System::Object^ sender, System::EventArgs^ e) {

		getWindowState()->addPeer = textBox1->Text;
		onButtonClick(ButtonId::AddPeer, textBox1->Text);
		Close();
	}
	};
}
