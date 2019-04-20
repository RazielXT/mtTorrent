#pragma once

extern void onButtonClick(System::Object^ button);
extern void fillFilesSelectionForm();
extern void fileSelectionChanged(int id, bool selected);

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

	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Id;
	private: System::Windows::Forms::DataGridViewCheckBoxColumn^ Selected;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ NameColumn;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Size;
	public: System::Windows::Forms::Button^ selectAllButton;
	private:
	public: System::Windows::Forms::Button^ deselectAllButton;

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
			this->Id = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Selected = (gcnew System::Windows::Forms::DataGridViewCheckBoxColumn());
			this->NameColumn = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Size = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->infoLabel = (gcnew System::Windows::Forms::Label());
			this->selectAllButton = (gcnew System::Windows::Forms::Button());
			this->deselectAllButton = (gcnew System::Windows::Forms::Button());
			this->panel1->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->filesGridView))->BeginInit();
			this->SuspendLayout();
			// 
			// cancelButton
			// 
			this->cancelButton->Location = System::Drawing::Point(630, 445);
			this->cancelButton->Name = L"cancelButton";
			this->cancelButton->Size = System::Drawing::Size(103, 30);
			this->cancelButton->TabIndex = 1;
			this->cancelButton->Text = L"Cancel";
			this->cancelButton->UseVisualStyleBackColor = true;
			this->cancelButton->Click += gcnew System::EventHandler(this, &FileSelectionForm::Button2_Click);
			// 
			// okButton
			// 
			this->okButton->Location = System::Drawing::Point(521, 445);
			this->okButton->Name = L"okButton";
			this->okButton->Size = System::Drawing::Size(103, 30);
			this->okButton->TabIndex = 2;
			this->okButton->Text = L"Ok";
			this->okButton->UseVisualStyleBackColor = true;
			this->okButton->Click += gcnew System::EventHandler(this, &FileSelectionForm::Button1_Click);
			// 
			// panel1
			// 
			this->panel1->AutoScroll = true;
			this->panel1->Controls->Add(this->filesGridView);
			this->panel1->Location = System::Drawing::Point(13, 13);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(720, 411);
			this->panel1->TabIndex = 3;
			// 
			// filesGridView
			// 
			this->filesGridView->AllowUserToAddRows = false;
			this->filesGridView->AllowUserToDeleteRows = false;
			this->filesGridView->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->filesGridView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(4) {
				this->Id, this->Selected,
					this->NameColumn, this->Size
			});
			this->filesGridView->Dock = System::Windows::Forms::DockStyle::Fill;
			this->filesGridView->Location = System::Drawing::Point(0, 0);
			this->filesGridView->MultiSelect = false;
			this->filesGridView->Name = L"filesGridView";
			this->filesGridView->RowHeadersVisible = false;
			this->filesGridView->RowTemplate->Height = 24;
			this->filesGridView->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->filesGridView->Size = System::Drawing::Size(720, 411);
			this->filesGridView->TabIndex = 0;
			this->filesGridView->CellContentClick += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &FileSelectionForm::FilesGridView_CellContentClick);
			this->filesGridView->CellContentDoubleClick += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &FileSelectionForm::FilesGridView_CellContentClick);
			this->filesGridView->CellValueChanged += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &FileSelectionForm::FilesGridView_CellValueChanged);
			// 
			// Id
			// 
			this->Id->HeaderText = L"Id";
			this->Id->Name = L"Id";
			this->Id->Visible = false;
			// 
			// Selected
			// 
			this->Selected->HeaderText = L"";
			this->Selected->Name = L"Selected";
			this->Selected->SortMode = System::Windows::Forms::DataGridViewColumnSortMode::Automatic;
			this->Selected->Width = 50;
			// 
			// NameColumn
			// 
			this->NameColumn->AutoSizeMode = System::Windows::Forms::DataGridViewAutoSizeColumnMode::Fill;
			this->NameColumn->HeaderText = L"Name";
			this->NameColumn->Name = L"NameColumn";
			this->NameColumn->ReadOnly = true;
			// 
			// Size
			// 
			this->Size->HeaderText = L"Size";
			this->Size->Name = L"Size";
			this->Size->ReadOnly = true;
			// 
			// infoLabel
			// 
			this->infoLabel->AutoSize = true;
			this->infoLabel->Location = System::Drawing::Point(21, 458);
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
			this->selectAllButton->Text = L"Select all";
			this->selectAllButton->UseVisualStyleBackColor = true;
			this->selectAllButton->Click += gcnew System::EventHandler(this, &FileSelectionForm::SelectAllButton_Click);
			// 
			// deselectAllButton
			// 
			this->deselectAllButton->Location = System::Drawing::Point(122, 430);
			this->deselectAllButton->Name = L"deselectAllButton";
			this->deselectAllButton->Size = System::Drawing::Size(103, 25);
			this->deselectAllButton->TabIndex = 5;
			this->deselectAllButton->Text = L"Deselect all";
			this->deselectAllButton->UseVisualStyleBackColor = true;
			this->deselectAllButton->Click += gcnew System::EventHandler(this, &FileSelectionForm::DeselectAllButton_Click);
			// 
			// FileSelectionForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(745, 487);
			this->Controls->Add(this->selectAllButton);
			this->Controls->Add(this->deselectAllButton);
			this->Controls->Add(this->infoLabel);
			this->Controls->Add(this->panel1);
			this->Controls->Add(this->okButton);
			this->Controls->Add(this->cancelButton);
			this->Name = L"FileSelectionForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Select files";
			this->panel1->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->filesGridView))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void Button1_Click(System::Object^ sender, System::EventArgs^ e) {
		onButtonClick(sender);
	}
	private: System::Void Button2_Click(System::Object^ sender, System::EventArgs^ e) {
		onButtonClick(sender);
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
private: System::Void SelectAllButton_Click(System::Object^ sender, System::EventArgs^ e) {
	onButtonClick(sender);
}
private: System::Void DeselectAllButton_Click(System::Object^ sender, System::EventArgs^ e) {
	onButtonClick(sender);
}
};
}
