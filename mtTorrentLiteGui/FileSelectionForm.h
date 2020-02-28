#pragma once

#include "Buttons.h"
#include <vector>
extern void fillFilesSelectionForm();
extern void fileSelectionChanged(int id, bool selected);
extern void filePriorityChanged(const std::vector<int>&, System::String^);

namespace GuiLite {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for FileSelectionForm
	/// </summary>
	public ref class FileSelectionForm : public System::Windows::Forms::Form
	{
	public:
		FileSelectionForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			instance = this;
			fillFilesSelectionForm();
			initialized = true;
		}

		bool initialized = false;

	public: System::Windows::Forms::DataGridView^ filesGridView;
	public: System::Windows::Forms::Button^ selectAllButton;
	private:
	public: System::Windows::Forms::Button^ deselectAllButton;
	private: System::Windows::Forms::Label^ label1;
	public:
	public: System::Windows::Forms::Button^ buttonBrowse;
	private:
	public: System::Windows::Forms::Label^ labelError;
	public: System::Windows::Forms::TextBox^ textBoxPath;
	public: System::Windows::Forms::CheckBox^ checkBoxStart;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Id;
	public:
	private: System::Windows::Forms::DataGridViewCheckBoxColumn^ Selected;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ NameColumn;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Size;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Priority;

	public:  static FileSelectionForm^ instance;

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~FileSelectionForm()
		{
			if (components)
			{
				delete components;
			}
		}
	public: System::Windows::Forms::Button^ cancelButton;
	public: System::Windows::Forms::Button^ okButton;
	private: System::Windows::Forms::Panel^ panel1;
	public: System::Windows::Forms::Label^ infoLabel;

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
			this->cancelButton = (gcnew System::Windows::Forms::Button());
			this->okButton = (gcnew System::Windows::Forms::Button());
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->filesGridView = (gcnew System::Windows::Forms::DataGridView());
			this->infoLabel = (gcnew System::Windows::Forms::Label());
			this->selectAllButton = (gcnew System::Windows::Forms::Button());
			this->deselectAllButton = (gcnew System::Windows::Forms::Button());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->buttonBrowse = (gcnew System::Windows::Forms::Button());
			this->labelError = (gcnew System::Windows::Forms::Label());
			this->textBoxPath = (gcnew System::Windows::Forms::TextBox());
			this->checkBoxStart = (gcnew System::Windows::Forms::CheckBox());
			this->Id = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Selected = (gcnew System::Windows::Forms::DataGridViewCheckBoxColumn());
			this->NameColumn = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Size = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Priority = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->panel1->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->filesGridView))->BeginInit();
			this->SuspendLayout();
			// 
			// cancelButton
			// 
			this->cancelButton->Location = System::Drawing::Point(738, 560);
			this->cancelButton->Name = L"cancelButton";
			this->cancelButton->Size = System::Drawing::Size(103, 30);
			this->cancelButton->TabIndex = 1;
			this->cancelButton->Text = L"Cancel";
			this->cancelButton->UseVisualStyleBackColor = true;
			this->cancelButton->Click += gcnew System::EventHandler(this, &FileSelectionForm::Button2_Click);
			// 
			// okButton
			// 
			this->okButton->Location = System::Drawing::Point(629, 560);
			this->okButton->Name = L"okButton";
			this->okButton->Size = System::Drawing::Size(103, 30);
			this->okButton->TabIndex = 2;
			this->okButton->Text = L"OK";
			this->okButton->UseVisualStyleBackColor = true;
			this->okButton->Click += gcnew System::EventHandler(this, &FileSelectionForm::Button1_Click);
			// 
			// panel1
			// 
			this->panel1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->panel1->AutoScroll = true;
			this->panel1->AutoSize = true;
			this->panel1->Controls->Add(this->filesGridView);
			this->panel1->Location = System::Drawing::Point(13, 13);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(883, 411);
			this->panel1->TabIndex = 3;
			// 
			// filesGridView
			// 
			this->filesGridView->AllowUserToAddRows = false;
			this->filesGridView->AllowUserToDeleteRows = false;
			this->filesGridView->AllowUserToResizeRows = false;
			this->filesGridView->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->filesGridView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(5) {
				this->Id, this->Selected,
					this->NameColumn, this->Size, this->Priority
			});
			this->filesGridView->Dock = System::Windows::Forms::DockStyle::Fill;
			this->filesGridView->Location = System::Drawing::Point(0, 0);
			this->filesGridView->Name = L"filesGridView";
			this->filesGridView->ReadOnly = true;
			this->filesGridView->RowHeadersVisible = false;
			this->filesGridView->RowHeadersWidth = 51;
			this->filesGridView->RowTemplate->Height = 24;
			this->filesGridView->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->filesGridView->Size = System::Drawing::Size(883, 411);
			this->filesGridView->TabIndex = 0;
			this->filesGridView->CellContentClick += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &FileSelectionForm::FilesGridView_CellContentClick);
			this->filesGridView->CellContentDoubleClick += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &FileSelectionForm::FilesGridView_CellContentClick);
			this->filesGridView->CellValueChanged += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &FileSelectionForm::FilesGridView_CellValueChanged);
			this->filesGridView->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &FileSelectionForm::FilesGridView_MouseClick);
			// 
			// infoLabel
			// 
			this->infoLabel->AutoSize = true;
			this->infoLabel->Location = System::Drawing::Point(21, 465);
			this->infoLabel->Name = L"infoLabel";
			this->infoLabel->Size = System::Drawing::Size(0, 17);
			this->infoLabel->TabIndex = 4;
			// 
			// selectAllButton
			// 
			this->selectAllButton->Location = System::Drawing::Point(13, 430);
			this->selectAllButton->Name = L"selectAllButton";
			this->selectAllButton->Size = System::Drawing::Size(103, 25);
			this->selectAllButton->TabIndex = 6;
			this->selectAllButton->Text = L"Select All";
			this->selectAllButton->UseVisualStyleBackColor = true;
			this->selectAllButton->Click += gcnew System::EventHandler(this, &FileSelectionForm::SelectAllButton_Click);
			// 
			// deselectAllButton
			// 
			this->deselectAllButton->Location = System::Drawing::Point(122, 430);
			this->deselectAllButton->Name = L"deselectAllButton";
			this->deselectAllButton->Size = System::Drawing::Size(103, 25);
			this->deselectAllButton->TabIndex = 5;
			this->deselectAllButton->Text = L"Deselect All";
			this->deselectAllButton->UseVisualStyleBackColor = true;
			this->deselectAllButton->Click += gcnew System::EventHandler(this, &FileSelectionForm::DeselectAllButton_Click);
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(134)));
			this->label1->Location = System::Drawing::Point(12, 490);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(116, 17);
			this->label1->TabIndex = 7;
			this->label1->Text = L"Save Location:";
			// 
			// buttonBrowse
			// 
			this->buttonBrowse->Location = System::Drawing::Point(13, 506);
			this->buttonBrowse->Name = L"buttonBrowse";
			this->buttonBrowse->Size = System::Drawing::Size(81, 30);
			this->buttonBrowse->TabIndex = 9;
			this->buttonBrowse->Text = L"Browse..";
			this->buttonBrowse->UseVisualStyleBackColor = true;
			this->buttonBrowse->Click += gcnew System::EventHandler(this, &FileSelectionForm::buttonBrowse_Click);
			// 
			// labelError
			// 
			this->labelError->AutoSize = true;
			this->labelError->ForeColor = System::Drawing::Color::Red;
			this->labelError->Location = System::Drawing::Point(132, 490);
			this->labelError->Name = L"labelError";
			this->labelError->Size = System::Drawing::Size(39, 17);
			this->labelError->TabIndex = 10;
			this->labelError->Text = L"error";
			// 
			// textBoxPath
			// 
			this->textBoxPath->Location = System::Drawing::Point(100, 510);
			this->textBoxPath->Name = L"textBoxPath";
			this->textBoxPath->ReadOnly = true;
			this->textBoxPath->Size = System::Drawing::Size(739, 22);
			this->textBoxPath->TabIndex = 11;
			this->textBoxPath->Text = L"path";
			// 
			// checkBoxStart
			// 
			this->checkBoxStart->AutoSize = true;
			this->checkBoxStart->Checked = true;
			this->checkBoxStart->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkBoxStart->Location = System::Drawing::Point(15, 560);
			this->checkBoxStart->Name = L"checkBoxStart";
			this->checkBoxStart->Size = System::Drawing::Size(60, 21);
			this->checkBoxStart->TabIndex = 12;
			this->checkBoxStart->Text = L"Start";
			this->checkBoxStart->UseVisualStyleBackColor = true;
			// 
			// Id
			// 
			this->Id->HeaderText = L"Id";
			this->Id->MinimumWidth = 6;
			this->Id->Name = L"Id";
			this->Id->Visible = false;
			// 
			// Selected
			// 
			this->Selected->HeaderText = L"";
			this->Selected->MinimumWidth = 6;
			this->Selected->Name = L"Selected";
			this->Selected->SortMode = System::Windows::Forms::DataGridViewColumnSortMode::Automatic;
			this->Selected->Width = 50;
			// 
			// NameColumn
			// 
			this->NameColumn->AutoSizeMode = System::Windows::Forms::DataGridViewAutoSizeColumnMode::Fill;
			this->NameColumn->HeaderText = L"Name";
			this->NameColumn->MinimumWidth = 6;
			this->NameColumn->Name = L"NameColumn";
			this->NameColumn->ReadOnly = true;
			// 
			// Size
			// 
			this->Size->HeaderText = L"Size";
			this->Size->MinimumWidth = 6;
			this->Size->Name = L"Size";
			this->Size->ReadOnly = true;
			this->Size->Width = 125;
			// 
			// Priority
			// 
			this->Priority->HeaderText = L"Priority";
			this->Priority->MinimumWidth = 6;
			this->Priority->Name = L"Priority";
			this->Priority->ReadOnly = true;
			this->Priority->Resizable = System::Windows::Forms::DataGridViewTriState::True;
			this->Priority->SortMode = System::Windows::Forms::DataGridViewColumnSortMode::NotSortable;
			this->Priority->Width = 125;
			// 
			// FileSelectionForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(920, 600);
			this->Controls->Add(this->checkBoxStart);
			this->Controls->Add(this->textBoxPath);
			this->Controls->Add(this->labelError);
			this->Controls->Add(this->buttonBrowse);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->selectAllButton);
			this->Controls->Add(this->deselectAllButton);
			this->Controls->Add(this->infoLabel);
			this->Controls->Add(this->panel1);
			this->Controls->Add(this->okButton);
			this->Controls->Add(this->cancelButton);
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"FileSelectionForm";
			this->ShowIcon = false;
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Select files";
			this->panel1->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->filesGridView))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void Button1_Click(System::Object^ sender, System::EventArgs^ e) {
		onButtonClick(ButtonId::SelectionOk);
	}
	private: System::Void Button2_Click(System::Object^ sender, System::EventArgs^ e) {
		onButtonClick(ButtonId::SelectionCancel);
	}

private: System::Void FilesGridView_CellContentClick(System::Object^ sender, System::Windows::Forms::DataGridViewCellEventArgs^ e) {

	filesGridView->CommitEdit(DataGridViewDataErrorContexts::Commit);

}
		 private: System::Void FilesGridView_CellValueChanged(System::Object^ sender, System::Windows::Forms::DataGridViewCellEventArgs^ e)
		 {
			 // End of edition on each click on column of checkbox
			 if (initialized && e->ColumnIndex == 1)
			 {
				 int id = int::Parse((String^)filesGridView->Rows[e->RowIndex]->Cells[0]->EditedFormattedValue);
				 bool selected = (bool)filesGridView->Rows[e->RowIndex]->Cells[1]->EditedFormattedValue;
				 fileSelectionChanged(id, selected);
			 }
		 }

	private: System::Void FilesGridView_MouseClick(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e) {
		if (e->Button == System::Windows::Forms::MouseButtons::Right)
		{
			int currentMouseOverRow = filesGridView->HitTest(e->X, e->Y)->RowIndex;

			if (currentMouseOverRow >= 0)
			{
				auto m = gcnew System::Windows::Forms::ContextMenu();
				auto menuItem = gcnew System::Windows::Forms::MenuItem("Priority");

				auto submenuItem = gcnew System::Windows::Forms::MenuItem("Low");
				submenuItem->Click += gcnew System::EventHandler(this, &FileSelectionForm::menuItem_Click);
				menuItem->MenuItems->Add(submenuItem);
				submenuItem = gcnew System::Windows::Forms::MenuItem("Normal");
				submenuItem->Click += gcnew System::EventHandler(this, &FileSelectionForm::menuItem_Click);
				menuItem->MenuItems->Add(submenuItem);
				submenuItem = gcnew System::Windows::Forms::MenuItem("High");
				submenuItem->Click += gcnew System::EventHandler(this, &FileSelectionForm::menuItem_Click);
				menuItem->MenuItems->Add(submenuItem);

				m->MenuItems->Add(menuItem);

				m->Show(filesGridView, System::Drawing::Point(e->X, e->Y));
			}
		}
	}
	private: System::Void menuItem_Click(System::Object^ sender, System::EventArgs^ e) {
		auto buttonText = ((System::Windows::Forms::MenuItem^)sender)->Text;

		std::vector<int> index;
		for (int i = 0; i < filesGridView->SelectedRows->Count; i++)
		{
			filesGridView->SelectedRows[i]->Cells[4]->Value = buttonText;
			index.push_back(int::Parse((String^)filesGridView->SelectedRows[i]->Cells[0]->EditedFormattedValue));
		}

		filePriorityChanged(index, buttonText);
	}

private: System::Void SelectAllButton_Click(System::Object^ sender, System::EventArgs^ e) {
	onButtonClick(ButtonId::SelectionAll);
}
private: System::Void DeselectAllButton_Click(System::Object^ sender, System::EventArgs^ e) {
	onButtonClick(ButtonId::SelectionNone);
}
private: System::Void buttonBrowse_Click(System::Object^ sender, System::EventArgs^ e) {

	System::Windows::Forms::FolderBrowserDialog dialog;
	auto result = dialog.ShowDialog(this);

	if (result == System::Windows::Forms::DialogResult::OK && dialog.SelectedPath->Length > 0)
	{
		textBoxPath->Text = dialog.SelectedPath;
		labelError->Visible = false;
	}
}
};
}
