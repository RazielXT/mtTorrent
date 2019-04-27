#pragma once

extern void refreshUi();
extern void onButtonClick(System::Object^ button, System::String^ id);
extern void onButtonClick(System::Object^ button);

namespace GuiLite {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MainForm
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
	public:
		MainForm(void)
		{
			InitializeComponent();
			
			instance = this;

			updateTimer = gcnew System::Windows::Forms::Timer();
			updateTimer->Tick += gcnew EventHandler(TimerUpdateFunc);
			updateTimer->Interval = 100;
			updateTimer->Start();
		}

	private: System::Windows::Forms::DataGridView^  peersGridView;
	public: System::Windows::Forms::TextBox^  torrentInfoLabel;
	private: System::Windows::Forms::TabPage^  sourcesTab;
	public: System::Windows::Forms::DataGridView^  sourcesGrid;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  dataGridViewTextBoxColumn1;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  dataGridViewTextBoxColumn2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  dataGridViewTextBoxColumn3;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  Column1;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  dataGridViewTextBoxColumn4;





	public: System::Windows::Forms::Button^  buttonStop;
	public: System::Windows::Forms::Button^  buttonStart;
	public: System::Windows::Forms::Button^  buttonAddMagnet;
	public: System::Windows::Forms::Button^  buttonSettings;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  torrentId;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  TorrentName;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  TorrentProgress;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  Column4;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  TorrentDownload;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  torrentUpload;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  TorrentConnected;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  TorrentFoundPeers;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  TorrentDownloaded;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  Column3;
	public: System::Windows::Forms::Button^ selectButton;
	private: System::Windows::Forms::ContextMenuStrip^ peersContextMenuStrip;
	private: System::Windows::Forms::ToolStripMenuItem^ toolStripMenuItem1;
	private: System::Windows::Forms::TabPage^ tabPage3;
	private: System::Windows::Forms::Panel^ panel1;
	public: System::Windows::Forms::DataVisualization::Charting::Chart^ dlSpeedChart;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ gridPeerAddress;
	public:
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ gridPeerSpeedDownload;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ gridPeerPercentage;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ peerSource;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ peerCountry;

	public:

		static MainForm^ instance;

		System::Windows::Forms::DataGridView^  getGrid()
		{
			return torrentsGrid;
		}

		System::Windows::Forms::DataGridView^  getPeersGrid()
		{
			return peersGridView;
		}

	private: System::Windows::Forms::SplitContainer^  splitContainer2;
public: System::Windows::Forms::Button^  buttonAddTorrent;
	private: System::Windows::Forms::DataGridView^  torrentsGrid;

		static System::Windows::Forms::Timer^ updateTimer;

		static void TimerUpdateFunc(System::Object^ myObject, System::EventArgs^ myEventArgs)
		{
			refreshUi();
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MainForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::SplitContainer^  splitContainer1;
	private: System::Windows::Forms::TabControl^  TorrentTabs;

	private: System::Windows::Forms::TabPage^  tabPage1;
	private: System::Windows::Forms::TabPage^  tabPage2;
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
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle1 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle9 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle10 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle2 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle3 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle4 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle5 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle6 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle7 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle8 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle11 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle15 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle16 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle12 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle13 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle14 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle17 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle20 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle21 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle18 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle19 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataVisualization::Charting::ChartArea^ chartArea1 = (gcnew System::Windows::Forms::DataVisualization::Charting::ChartArea());
			System::Windows::Forms::DataVisualization::Charting::Legend^ legend1 = (gcnew System::Windows::Forms::DataVisualization::Charting::Legend());
			System::Windows::Forms::DataVisualization::Charting::Series^ series1 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			this->splitContainer1 = (gcnew System::Windows::Forms::SplitContainer());
			this->splitContainer2 = (gcnew System::Windows::Forms::SplitContainer());
			this->buttonSettings = (gcnew System::Windows::Forms::Button());
			this->buttonAddMagnet = (gcnew System::Windows::Forms::Button());
			this->buttonStop = (gcnew System::Windows::Forms::Button());
			this->buttonStart = (gcnew System::Windows::Forms::Button());
			this->buttonAddTorrent = (gcnew System::Windows::Forms::Button());
			this->torrentsGrid = (gcnew System::Windows::Forms::DataGridView());
			this->torrentId = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentName = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentProgress = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Column4 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentDownload = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->torrentUpload = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentConnected = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentFoundPeers = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentDownloaded = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Column3 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentTabs = (gcnew System::Windows::Forms::TabControl());
			this->tabPage1 = (gcnew System::Windows::Forms::TabPage());
			this->selectButton = (gcnew System::Windows::Forms::Button());
			this->torrentInfoLabel = (gcnew System::Windows::Forms::TextBox());
			this->tabPage2 = (gcnew System::Windows::Forms::TabPage());
			this->peersGridView = (gcnew System::Windows::Forms::DataGridView());
			this->gridPeerAddress = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->gridPeerSpeedDownload = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Column2 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->gridPeerPercentage = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->peerSource = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->peerCountry = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->peersContextMenuStrip = (gcnew System::Windows::Forms::ContextMenuStrip(this->components));
			this->toolStripMenuItem1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->sourcesTab = (gcnew System::Windows::Forms::TabPage());
			this->sourcesGrid = (gcnew System::Windows::Forms::DataGridView());
			this->dataGridViewTextBoxColumn1 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->dataGridViewTextBoxColumn2 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->dataGridViewTextBoxColumn3 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Column1 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->dataGridViewTextBoxColumn4 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->tabPage3 = (gcnew System::Windows::Forms::TabPage());
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->dlSpeedChart = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainer1))->BeginInit();
			this->splitContainer1->Panel1->SuspendLayout();
			this->splitContainer1->Panel2->SuspendLayout();
			this->splitContainer1->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainer2))->BeginInit();
			this->splitContainer2->Panel1->SuspendLayout();
			this->splitContainer2->Panel2->SuspendLayout();
			this->splitContainer2->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->torrentsGrid))->BeginInit();
			this->TorrentTabs->SuspendLayout();
			this->tabPage1->SuspendLayout();
			this->tabPage2->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->peersGridView))->BeginInit();
			this->peersContextMenuStrip->SuspendLayout();
			this->sourcesTab->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->sourcesGrid))->BeginInit();
			this->tabPage3->SuspendLayout();
			this->panel1->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->dlSpeedChart))->BeginInit();
			this->SuspendLayout();
			// 
			// splitContainer1
			// 
			this->splitContainer1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->splitContainer1->Location = System::Drawing::Point(0, 0);
			this->splitContainer1->Name = L"splitContainer1";
			this->splitContainer1->Orientation = System::Windows::Forms::Orientation::Horizontal;
			// 
			// splitContainer1.Panel1
			// 
			this->splitContainer1->Panel1->Controls->Add(this->splitContainer2);
			// 
			// splitContainer1.Panel2
			// 
			this->splitContainer1->Panel2->Controls->Add(this->TorrentTabs);
			this->splitContainer1->Size = System::Drawing::Size(1339, 497);
			this->splitContainer1->SplitterDistance = 264;
			this->splitContainer1->TabIndex = 3;
			// 
			// splitContainer2
			// 
			this->splitContainer2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->splitContainer2->FixedPanel = System::Windows::Forms::FixedPanel::Panel1;
			this->splitContainer2->Location = System::Drawing::Point(0, 0);
			this->splitContainer2->Name = L"splitContainer2";
			this->splitContainer2->Orientation = System::Windows::Forms::Orientation::Horizontal;
			// 
			// splitContainer2.Panel1
			// 
			this->splitContainer2->Panel1->Controls->Add(this->buttonSettings);
			this->splitContainer2->Panel1->Controls->Add(this->buttonAddMagnet);
			this->splitContainer2->Panel1->Controls->Add(this->buttonStop);
			this->splitContainer2->Panel1->Controls->Add(this->buttonStart);
			this->splitContainer2->Panel1->Controls->Add(this->buttonAddTorrent);
			// 
			// splitContainer2.Panel2
			// 
			this->splitContainer2->Panel2->Controls->Add(this->torrentsGrid);
			this->splitContainer2->Size = System::Drawing::Size(1339, 264);
			this->splitContainer2->SplitterDistance = 43;
			this->splitContainer2->TabIndex = 2;
			// 
			// buttonSettings
			// 
			this->buttonSettings->Location = System::Drawing::Point(1252, 11);
			this->buttonSettings->Name = L"buttonSettings";
			this->buttonSettings->Size = System::Drawing::Size(75, 23);
			this->buttonSettings->TabIndex = 6;
			this->buttonSettings->Text = L"Settings";
			this->buttonSettings->UseVisualStyleBackColor = true;
			this->buttonSettings->Click += gcnew System::EventHandler(this, &MainForm::buttonSettings_Click);
			// 
			// buttonAddMagnet
			// 
			this->buttonAddMagnet->Location = System::Drawing::Point(93, 12);
			this->buttonAddMagnet->Name = L"buttonAddMagnet";
			this->buttonAddMagnet->Size = System::Drawing::Size(75, 23);
			this->buttonAddMagnet->TabIndex = 5;
			this->buttonAddMagnet->Text = L"Magnet";
			this->buttonAddMagnet->UseVisualStyleBackColor = true;
			this->buttonAddMagnet->Click += gcnew System::EventHandler(this, &MainForm::buttonAddMagnet_Click);
			// 
			// buttonStop
			// 
			this->buttonStop->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(255)), static_cast<System::Int32>(static_cast<System::Byte>(128)),
				static_cast<System::Int32>(static_cast<System::Byte>(128)));
			this->buttonStop->Location = System::Drawing::Point(285, 12);
			this->buttonStop->Name = L"buttonStop";
			this->buttonStop->Size = System::Drawing::Size(75, 23);
			this->buttonStop->TabIndex = 4;
			this->buttonStop->Text = L"Stop";
			this->buttonStop->UseVisualStyleBackColor = false;
			this->buttonStop->Click += gcnew System::EventHandler(this, &MainForm::buttonStop_Click);
			// 
			// buttonStart
			// 
			this->buttonStart->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(0)), static_cast<System::Int32>(static_cast<System::Byte>(192)),
				static_cast<System::Int32>(static_cast<System::Byte>(0)));
			this->buttonStart->Location = System::Drawing::Point(204, 12);
			this->buttonStart->Name = L"buttonStart";
			this->buttonStart->Size = System::Drawing::Size(75, 23);
			this->buttonStart->TabIndex = 3;
			this->buttonStart->Text = L"Start";
			this->buttonStart->UseVisualStyleBackColor = false;
			this->buttonStart->Click += gcnew System::EventHandler(this, &MainForm::buttonStart_Click);
			// 
			// buttonAddTorrent
			// 
			this->buttonAddTorrent->Location = System::Drawing::Point(12, 12);
			this->buttonAddTorrent->Name = L"buttonAddTorrent";
			this->buttonAddTorrent->Size = System::Drawing::Size(75, 23);
			this->buttonAddTorrent->TabIndex = 2;
			this->buttonAddTorrent->Text = L"File";
			this->buttonAddTorrent->UseVisualStyleBackColor = true;
			this->buttonAddTorrent->Click += gcnew System::EventHandler(this, &MainForm::button1_Click);
			// 
			// torrentsGrid
			// 
			this->torrentsGrid->AllowUserToAddRows = false;
			this->torrentsGrid->AllowUserToDeleteRows = false;
			this->torrentsGrid->AllowUserToOrderColumns = true;
			this->torrentsGrid->AllowUserToResizeRows = false;
			this->torrentsGrid->BackgroundColor = System::Drawing::SystemColors::Window;
			this->torrentsGrid->CellBorderStyle = System::Windows::Forms::DataGridViewCellBorderStyle::SingleHorizontal;
			dataGridViewCellStyle1->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle1->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			dataGridViewCellStyle1->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle1->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle1->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle1->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->torrentsGrid->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle1;
			this->torrentsGrid->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->torrentsGrid->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(10) {
				this->torrentId,
					this->TorrentName, this->TorrentProgress, this->Column4, this->TorrentDownload, this->torrentUpload, this->TorrentConnected,
					this->TorrentFoundPeers, this->TorrentDownloaded, this->Column3
			});
			dataGridViewCellStyle9->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleLeft;
			dataGridViewCellStyle9->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle9->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(134)));
			dataGridViewCellStyle9->ForeColor = System::Drawing::SystemColors::ControlText;
			dataGridViewCellStyle9->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle9->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle9->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->torrentsGrid->DefaultCellStyle = dataGridViewCellStyle9;
			this->torrentsGrid->Dock = System::Windows::Forms::DockStyle::Fill;
			this->torrentsGrid->Location = System::Drawing::Point(0, 0);
			this->torrentsGrid->Name = L"torrentsGrid";
			this->torrentsGrid->ReadOnly = true;
			dataGridViewCellStyle10->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle10->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle10->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle10->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle10->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle10->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle10->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->torrentsGrid->RowHeadersDefaultCellStyle = dataGridViewCellStyle10;
			this->torrentsGrid->RowHeadersVisible = false;
			this->torrentsGrid->RowTemplate->Height = 24;
			this->torrentsGrid->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->torrentsGrid->Size = System::Drawing::Size(1339, 217);
			this->torrentsGrid->TabIndex = 1;
			this->torrentsGrid->CellClick += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &MainForm::torrentsGrid_CellContentClick);
			this->torrentsGrid->CellMouseDoubleClick += gcnew System::Windows::Forms::DataGridViewCellMouseEventHandler(this, &MainForm::TorrentsGrid_CellContentClick_1);
			// 
			// torrentId
			// 
			this->torrentId->HeaderText = L"Id";
			this->torrentId->Name = L"torrentId";
			this->torrentId->ReadOnly = true;
			this->torrentId->Visible = false;
			// 
			// TorrentName
			// 
			this->TorrentName->HeaderText = L"Name";
			this->TorrentName->MinimumWidth = 20;
			this->TorrentName->Name = L"TorrentName";
			this->TorrentName->ReadOnly = true;
			this->TorrentName->Width = 200;
			// 
			// TorrentProgress
			// 
			dataGridViewCellStyle2->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentProgress->DefaultCellStyle = dataGridViewCellStyle2;
			this->TorrentProgress->HeaderText = L"Progress";
			this->TorrentProgress->Name = L"TorrentProgress";
			this->TorrentProgress->ReadOnly = true;
			// 
			// Column4
			// 
			this->Column4->HeaderText = L"State";
			this->Column4->Name = L"Column4";
			this->Column4->ReadOnly = true;
			// 
			// TorrentDownload
			// 
			dataGridViewCellStyle3->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentDownload->DefaultCellStyle = dataGridViewCellStyle3;
			this->TorrentDownload->HeaderText = L"Download";
			this->TorrentDownload->Name = L"TorrentDownload";
			this->TorrentDownload->ReadOnly = true;
			// 
			// torrentUpload
			// 
			dataGridViewCellStyle4->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->torrentUpload->DefaultCellStyle = dataGridViewCellStyle4;
			this->torrentUpload->HeaderText = L"Upload";
			this->torrentUpload->Name = L"torrentUpload";
			this->torrentUpload->ReadOnly = true;
			// 
			// TorrentConnected
			// 
			dataGridViewCellStyle5->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentConnected->DefaultCellStyle = dataGridViewCellStyle5;
			this->TorrentConnected->HeaderText = L"Connected";
			this->TorrentConnected->Name = L"TorrentConnected";
			this->TorrentConnected->ReadOnly = true;
			// 
			// TorrentFoundPeers
			// 
			dataGridViewCellStyle6->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentFoundPeers->DefaultCellStyle = dataGridViewCellStyle6;
			this->TorrentFoundPeers->HeaderText = L"Found Peers";
			this->TorrentFoundPeers->Name = L"TorrentFoundPeers";
			this->TorrentFoundPeers->ReadOnly = true;
			// 
			// TorrentDownloaded
			// 
			dataGridViewCellStyle7->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentDownloaded->DefaultCellStyle = dataGridViewCellStyle7;
			this->TorrentDownloaded->HeaderText = L"Downloaded";
			this->TorrentDownloaded->Name = L"TorrentDownloaded";
			this->TorrentDownloaded->ReadOnly = true;
			// 
			// Column3
			// 
			dataGridViewCellStyle8->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Column3->DefaultCellStyle = dataGridViewCellStyle8;
			this->Column3->HeaderText = L"Uploaded";
			this->Column3->Name = L"Column3";
			this->Column3->ReadOnly = true;
			// 
			// TorrentTabs
			// 
			this->TorrentTabs->Controls->Add(this->tabPage1);
			this->TorrentTabs->Controls->Add(this->tabPage2);
			this->TorrentTabs->Controls->Add(this->sourcesTab);
			this->TorrentTabs->Controls->Add(this->tabPage3);
			this->TorrentTabs->Dock = System::Windows::Forms::DockStyle::Fill;
			this->TorrentTabs->Location = System::Drawing::Point(0, 0);
			this->TorrentTabs->Name = L"TorrentTabs";
			this->TorrentTabs->SelectedIndex = 0;
			this->TorrentTabs->Size = System::Drawing::Size(1339, 229);
			this->TorrentTabs->TabIndex = 0;
			// 
			// tabPage1
			// 
			this->tabPage1->Controls->Add(this->selectButton);
			this->tabPage1->Controls->Add(this->torrentInfoLabel);
			this->tabPage1->Location = System::Drawing::Point(4, 25);
			this->tabPage1->Name = L"tabPage1";
			this->tabPage1->Padding = System::Windows::Forms::Padding(3);
			this->tabPage1->Size = System::Drawing::Size(1331, 200);
			this->tabPage1->TabIndex = 0;
			this->tabPage1->Text = L"General";
			this->tabPage1->UseVisualStyleBackColor = true;
			// 
			// selectButton
			// 
			this->selectButton->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->selectButton->Location = System::Drawing::Point(3, 174);
			this->selectButton->MaximumSize = System::Drawing::Size(300, 23);
			this->selectButton->Name = L"selectButton";
			this->selectButton->Size = System::Drawing::Size(300, 23);
			this->selectButton->TabIndex = 1;
			this->selectButton->Text = L"Select";
			this->selectButton->UseVisualStyleBackColor = true;
			this->selectButton->Visible = false;
			this->selectButton->Click += gcnew System::EventHandler(this, &MainForm::SelectButton_Click);
			// 
			// torrentInfoLabel
			// 
			this->torrentInfoLabel->Dock = System::Windows::Forms::DockStyle::Fill;
			this->torrentInfoLabel->Font = (gcnew System::Drawing::Font(L"Verdana", 7.8F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->torrentInfoLabel->Location = System::Drawing::Point(3, 3);
			this->torrentInfoLabel->Multiline = true;
			this->torrentInfoLabel->Name = L"torrentInfoLabel";
			this->torrentInfoLabel->ReadOnly = true;
			this->torrentInfoLabel->ScrollBars = System::Windows::Forms::ScrollBars::Both;
			this->torrentInfoLabel->Size = System::Drawing::Size(1325, 194);
			this->torrentInfoLabel->TabIndex = 0;
			// 
			// tabPage2
			// 
			this->tabPage2->Controls->Add(this->peersGridView);
			this->tabPage2->Location = System::Drawing::Point(4, 25);
			this->tabPage2->Name = L"tabPage2";
			this->tabPage2->Padding = System::Windows::Forms::Padding(3);
			this->tabPage2->Size = System::Drawing::Size(1331, 200);
			this->tabPage2->TabIndex = 1;
			this->tabPage2->Text = L"Peers";
			this->tabPage2->UseVisualStyleBackColor = true;
			// 
			// peersGridView
			// 
			this->peersGridView->AllowUserToAddRows = false;
			this->peersGridView->AllowUserToDeleteRows = false;
			this->peersGridView->AllowUserToOrderColumns = true;
			this->peersGridView->AllowUserToResizeRows = false;
			this->peersGridView->BackgroundColor = System::Drawing::SystemColors::Window;
			this->peersGridView->CellBorderStyle = System::Windows::Forms::DataGridViewCellBorderStyle::SingleHorizontal;
			dataGridViewCellStyle11->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle11->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle11->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.2F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			dataGridViewCellStyle11->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle11->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle11->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle11->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->peersGridView->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle11;
			this->peersGridView->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->peersGridView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(6) {
				this->gridPeerAddress,
					this->gridPeerSpeedDownload, this->Column2, this->gridPeerPercentage, this->peerSource, this->peerCountry
			});
			this->peersGridView->ContextMenuStrip = this->peersContextMenuStrip;
			dataGridViewCellStyle15->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleLeft;
			dataGridViewCellStyle15->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle15->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle15->ForeColor = System::Drawing::SystemColors::ControlText;
			dataGridViewCellStyle15->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle15->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle15->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->peersGridView->DefaultCellStyle = dataGridViewCellStyle15;
			this->peersGridView->Dock = System::Windows::Forms::DockStyle::Fill;
			this->peersGridView->Location = System::Drawing::Point(3, 3);
			this->peersGridView->Name = L"peersGridView";
			this->peersGridView->ReadOnly = true;
			dataGridViewCellStyle16->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle16->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle16->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle16->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle16->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle16->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle16->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->peersGridView->RowHeadersDefaultCellStyle = dataGridViewCellStyle16;
			this->peersGridView->RowHeadersVisible = false;
			this->peersGridView->RowTemplate->Height = 24;
			this->peersGridView->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->peersGridView->Size = System::Drawing::Size(1325, 194);
			this->peersGridView->TabIndex = 2;
			// 
			// gridPeerAddress
			// 
			this->gridPeerAddress->HeaderText = L"Address";
			this->gridPeerAddress->MinimumWidth = 20;
			this->gridPeerAddress->Name = L"gridPeerAddress";
			this->gridPeerAddress->ReadOnly = true;
			this->gridPeerAddress->Width = 200;
			// 
			// gridPeerSpeedDownload
			// 
			dataGridViewCellStyle12->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->gridPeerSpeedDownload->DefaultCellStyle = dataGridViewCellStyle12;
			this->gridPeerSpeedDownload->HeaderText = L"Download speed";
			this->gridPeerSpeedDownload->Name = L"gridPeerSpeedDownload";
			this->gridPeerSpeedDownload->ReadOnly = true;
			this->gridPeerSpeedDownload->Width = 150;
			// 
			// Column2
			// 
			dataGridViewCellStyle13->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Column2->DefaultCellStyle = dataGridViewCellStyle13;
			this->Column2->HeaderText = L"Upload speed";
			this->Column2->Name = L"Column2";
			this->Column2->ReadOnly = true;
			this->Column2->Width = 150;
			// 
			// gridPeerPercentage
			// 
			dataGridViewCellStyle14->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->gridPeerPercentage->DefaultCellStyle = dataGridViewCellStyle14;
			this->gridPeerPercentage->HeaderText = L"%";
			this->gridPeerPercentage->Name = L"gridPeerPercentage";
			this->gridPeerPercentage->ReadOnly = true;
			// 
			// peerSource
			// 
			this->peerSource->HeaderText = L"Source";
			this->peerSource->Name = L"peerSource";
			this->peerSource->ReadOnly = true;
			this->peerSource->Width = 150;
			// 
			// peerCountry
			// 
			this->peerCountry->HeaderText = L"Country";
			this->peerCountry->Name = L"peerCountry";
			this->peerCountry->ReadOnly = true;
			// 
			// peersContextMenuStrip
			// 
			this->peersContextMenuStrip->ImageScalingSize = System::Drawing::Size(20, 20);
			this->peersContextMenuStrip->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) { this->toolStripMenuItem1 });
			this->peersContextMenuStrip->Name = L"peersContextMenuStrip";
			this->peersContextMenuStrip->Size = System::Drawing::Size(141, 28);
			// 
			// toolStripMenuItem1
			// 
			this->toolStripMenuItem1->Name = L"toolStripMenuItem1";
			this->toolStripMenuItem1->Size = System::Drawing::Size(140, 24);
			this->toolStripMenuItem1->Text = L"Add peer";
			this->toolStripMenuItem1->Click += gcnew System::EventHandler(this, &MainForm::ToolStripMenuItem1_Click);
			// 
			// sourcesTab
			// 
			this->sourcesTab->Controls->Add(this->sourcesGrid);
			this->sourcesTab->Location = System::Drawing::Point(4, 25);
			this->sourcesTab->Name = L"sourcesTab";
			this->sourcesTab->Padding = System::Windows::Forms::Padding(3);
			this->sourcesTab->Size = System::Drawing::Size(1331, 200);
			this->sourcesTab->TabIndex = 2;
			this->sourcesTab->Text = L"Sources";
			this->sourcesTab->UseVisualStyleBackColor = true;
			// 
			// sourcesGrid
			// 
			this->sourcesGrid->AllowUserToAddRows = false;
			this->sourcesGrid->AllowUserToDeleteRows = false;
			this->sourcesGrid->AllowUserToOrderColumns = true;
			this->sourcesGrid->AllowUserToResizeRows = false;
			this->sourcesGrid->BackgroundColor = System::Drawing::SystemColors::Window;
			this->sourcesGrid->CellBorderStyle = System::Windows::Forms::DataGridViewCellBorderStyle::SingleHorizontal;
			dataGridViewCellStyle17->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle17->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle17->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.2F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			dataGridViewCellStyle17->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle17->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle17->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle17->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->sourcesGrid->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle17;
			this->sourcesGrid->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->sourcesGrid->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(5) {
				this->dataGridViewTextBoxColumn1,
					this->dataGridViewTextBoxColumn2, this->dataGridViewTextBoxColumn3, this->Column1, this->dataGridViewTextBoxColumn4
			});
			dataGridViewCellStyle20->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleLeft;
			dataGridViewCellStyle20->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle20->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle20->ForeColor = System::Drawing::SystemColors::ControlText;
			dataGridViewCellStyle20->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle20->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle20->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->sourcesGrid->DefaultCellStyle = dataGridViewCellStyle20;
			this->sourcesGrid->Dock = System::Windows::Forms::DockStyle::Fill;
			this->sourcesGrid->Location = System::Drawing::Point(3, 3);
			this->sourcesGrid->Name = L"sourcesGrid";
			this->sourcesGrid->ReadOnly = true;
			dataGridViewCellStyle21->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle21->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle21->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle21->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle21->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle21->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle21->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->sourcesGrid->RowHeadersDefaultCellStyle = dataGridViewCellStyle21;
			this->sourcesGrid->RowHeadersVisible = false;
			this->sourcesGrid->RowTemplate->Height = 24;
			this->sourcesGrid->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->sourcesGrid->Size = System::Drawing::Size(1325, 194);
			this->sourcesGrid->TabIndex = 3;
			this->sourcesGrid->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::sourcesGrid_MouseClick);
			// 
			// dataGridViewTextBoxColumn1
			// 
			this->dataGridViewTextBoxColumn1->HeaderText = L"Name";
			this->dataGridViewTextBoxColumn1->MinimumWidth = 20;
			this->dataGridViewTextBoxColumn1->Name = L"dataGridViewTextBoxColumn1";
			this->dataGridViewTextBoxColumn1->ReadOnly = true;
			this->dataGridViewTextBoxColumn1->Width = 230;
			// 
			// dataGridViewTextBoxColumn2
			// 
			dataGridViewCellStyle18->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->dataGridViewTextBoxColumn2->DefaultCellStyle = dataGridViewCellStyle18;
			this->dataGridViewTextBoxColumn2->HeaderText = L"State";
			this->dataGridViewTextBoxColumn2->Name = L"dataGridViewTextBoxColumn2";
			this->dataGridViewTextBoxColumn2->ReadOnly = true;
			this->dataGridViewTextBoxColumn2->Width = 120;
			// 
			// dataGridViewTextBoxColumn3
			// 
			dataGridViewCellStyle19->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->dataGridViewTextBoxColumn3->DefaultCellStyle = dataGridViewCellStyle19;
			this->dataGridViewTextBoxColumn3->HeaderText = L"Peers";
			this->dataGridViewTextBoxColumn3->Name = L"dataGridViewTextBoxColumn3";
			this->dataGridViewTextBoxColumn3->ReadOnly = true;
			// 
			// Column1
			// 
			this->Column1->HeaderText = L"Next check";
			this->Column1->Name = L"Column1";
			this->Column1->ReadOnly = true;
			// 
			// dataGridViewTextBoxColumn4
			// 
			this->dataGridViewTextBoxColumn4->HeaderText = L"Interval";
			this->dataGridViewTextBoxColumn4->Name = L"dataGridViewTextBoxColumn4";
			this->dataGridViewTextBoxColumn4->ReadOnly = true;
			// 
			// tabPage3
			// 
			this->tabPage3->Controls->Add(this->panel1);
			this->tabPage3->Location = System::Drawing::Point(4, 25);
			this->tabPage3->Name = L"tabPage3";
			this->tabPage3->Padding = System::Windows::Forms::Padding(3);
			this->tabPage3->Size = System::Drawing::Size(1331, 200);
			this->tabPage3->TabIndex = 3;
			this->tabPage3->Text = L"Statistics";
			this->tabPage3->UseVisualStyleBackColor = true;
			// 
			// panel1
			// 
			this->panel1->Controls->Add(this->dlSpeedChart);
			this->panel1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel1->Location = System::Drawing::Point(3, 3);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(1325, 194);
			this->panel1->TabIndex = 0;
			// 
			// dlSpeedChart
			// 
			this->dlSpeedChart->BorderlineWidth = 5;
			chartArea1->Name = L"ChartArea1";
			this->dlSpeedChart->ChartAreas->Add(chartArea1);
			this->dlSpeedChart->Dock = System::Windows::Forms::DockStyle::Fill;
			legend1->Name = L"Legend1";
			this->dlSpeedChart->Legends->Add(legend1);
			this->dlSpeedChart->Location = System::Drawing::Point(0, 0);
			this->dlSpeedChart->Name = L"dlSpeedChart";
			this->dlSpeedChart->Palette = System::Windows::Forms::DataVisualization::Charting::ChartColorPalette::Bright;
			series1->ChartArea = L"ChartArea1";
			series1->Legend = L"Legend1";
			series1->Name = L"Series1";
			this->dlSpeedChart->Series->Add(series1);
			this->dlSpeedChart->Size = System::Drawing::Size(1325, 194);
			this->dlSpeedChart->TabIndex = 0;
			this->dlSpeedChart->Text = L"Download speed";
			// 
			// MainForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1339, 497);
			this->Controls->Add(this->splitContainer1);
			this->Name = L"MainForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"mtTorrent";
			this->splitContainer1->Panel1->ResumeLayout(false);
			this->splitContainer1->Panel2->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainer1))->EndInit();
			this->splitContainer1->ResumeLayout(false);
			this->splitContainer2->Panel1->ResumeLayout(false);
			this->splitContainer2->Panel2->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainer2))->EndInit();
			this->splitContainer2->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->torrentsGrid))->EndInit();
			this->TorrentTabs->ResumeLayout(false);
			this->tabPage1->ResumeLayout(false);
			this->tabPage1->PerformLayout();
			this->tabPage2->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->peersGridView))->EndInit();
			this->peersContextMenuStrip->ResumeLayout(false);
			this->sourcesTab->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->sourcesGrid))->EndInit();
			this->tabPage3->ResumeLayout(false);
			this->panel1->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->dlSpeedChart))->EndInit();
			this->ResumeLayout(false);

		}
#pragma endregion
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
		onButtonClick(sender);
	}

private: System::Void torrentsGrid_CellContentClick(System::Object^  sender, System::Windows::Forms::DataGridViewCellEventArgs^  e) {
		onButtonClick(sender);
}
	private: System::Void buttonStart_Click(System::Object^  sender, System::EventArgs^  e) {
		onButtonClick(sender);
	}
	private: System::Void buttonStop_Click(System::Object^  sender, System::EventArgs^  e) {
		onButtonClick(sender);
	}
private: System::Void buttonAddMagnet_Click(System::Object^  sender, System::EventArgs^  e) {
	onButtonClick(sender);
}
private: System::Void buttonSettings_Click(System::Object^  sender, System::EventArgs^  e) {
	onButtonClick(sender);
}
System::String^ ctxMenuHint;
private: System::Void ctxMenu_Click(System::Object^  sender, System::EventArgs^  e) {
	onButtonClick(sender, ctxMenuHint);
}
private: System::Void sourcesGrid_MouseClick(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
	if (e->Button == System::Windows::Forms::MouseButtons::Right)
	{
		int currentMouseOverRow = sourcesGrid->HitTest(e->X, e->Y)->RowIndex;

		if (currentMouseOverRow >= 0)
		{
			ctxMenuHint = (System::String^)sourcesGrid->Rows[currentMouseOverRow]->Cells[0]->Value;
			System::Windows::Forms::ContextMenu^ m = gcnew System::Windows::Forms::ContextMenu();
			m->MenuItems->Add(gcnew System::Windows::Forms::MenuItem("Refresh"));
			m->MenuItems[0]->Click += gcnew System::EventHandler(this, &MainForm::ctxMenu_Click);
			m->Show(sourcesGrid, Point(e->X, e->Y));
		}
	}
}
private: System::Void SelectButton_Click(System::Object^ sender, System::EventArgs^ e) {
	onButtonClick(sender);
}
private: System::Void ToolStripMenuItem1_Click(System::Object^ sender, System::EventArgs^ e) {
	onButtonClick(nullptr, "AddPeer");
}
private: System::Void TorrentsGrid_CellContentClick_1(System::Object^ sender, System::Windows::Forms::DataGridViewCellMouseEventArgs^ e) {
	onButtonClick(nullptr, "Info");
}
};
}
