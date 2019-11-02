#pragma once

extern void scheduleTorrent(int seconds);

namespace mtTorrentLiteGui {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for ScheduleForm
	/// </summary>
	public ref class ScheduleForm : public System::Windows::Forms::Form
	{
	public:
		ScheduleForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//

			timePicker->Format = DateTimePickerFormat::Time;
			timePicker->MinDate = System::DateTime::Now;
			//timePicker->ShowUpDown = true;

			timePicker->Value = System::DateTime::Now.AddMinutes(1);
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~ScheduleForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::DateTimePicker^ timePicker;
	protected:

	protected:
	private: System::Windows::Forms::Button^ button1;
	private: System::Windows::Forms::Button^ button2;

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
			this->timePicker = (gcnew System::Windows::Forms::DateTimePicker());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// timePicker
			// 
			this->timePicker->CustomFormat = L"";
			this->timePicker->Location = System::Drawing::Point(35, 35);
			this->timePicker->Name = L"timePicker";
			this->timePicker->Size = System::Drawing::Size(262, 22);
			this->timePicker->TabIndex = 0;
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(138, 84);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(85, 26);
			this->button1->TabIndex = 1;
			this->button1->Text = L"Schedule";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &ScheduleForm::button1_Click);
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(229, 84);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(88, 26);
			this->button2->TabIndex = 2;
			this->button2->Text = L"Cancel";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &ScheduleForm::button2_Click);
			// 
			// ScheduleForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(339, 122);
			this->Controls->Add(this->button2);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->timePicker);
			this->Name = L"ScheduleForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Schedule";
			this->ResumeLayout(false);

		}
#pragma endregion
	private: System::Void button2_Click(System::Object^ sender, System::EventArgs^ e) {
		Close();
	}
	private: System::Void button1_Click(System::Object^ sender, System::EventArgs^ e) {

		System::DateTime time = timePicker->Value;
		System::DateTime now = System::DateTime::Now;
		auto span = time.Subtract(now);

		scheduleTorrent((int)span.TotalSeconds);
		Close();
	}
	};
}
