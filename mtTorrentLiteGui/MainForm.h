#pragma once

#include "Buttons.h"

extern void refreshUi();

namespace GuiLite {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Reflection;

	/// <summary>
	/// Summary for MainForm
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
	public:
		MainForm(void)
		{
			InitializeComponent();

			trayIcon->Icon = Icon->ExtractAssociatedIcon(Assembly::GetExecutingAssembly()->Location);

			torrentsGrid->SortCompare += gcnew DataGridViewSortCompareEventHandler(this, &MainForm::torrentGridView_SortCompare);
			peersGridView->SortCompare += gcnew DataGridViewSortCompareEventHandler(this, &MainForm::peersView_SortCompare);

			instance = this;

			updateTimer = gcnew System::Windows::Forms::Timer();
			updateTimer->Tick += gcnew EventHandler(TimerUpdateFunc);
			updateTimer->Interval = 100;
			updateTimer->Start();

			try
			{
				this->Icon = gcnew System::Drawing::Icon("data/mttIcon.ico");// (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			}
			catch (...)
			{
			}
		}

		static System::Windows::Forms::Timer^ scheduleTimer;

	private: System::Windows::Forms::DataGridView^  peersGridView;
	public: System::Windows::Forms::TextBox^  torrentInfoLabel;
	private: System::Windows::Forms::TabPage^  sourcesTab;
	public: System::Windows::Forms::DataGridView^  sourcesGrid;
	public: System::Windows::Forms::Button^  buttonStop;
	public: System::Windows::Forms::Button^  buttonStart;
	public: System::Windows::Forms::Button^  buttonAddMagnet;
	public: System::Windows::Forms::Button^  buttonSettings;
	private: System::Windows::Forms::ContextMenuStrip^ peersContextMenuStrip;
	private: System::Windows::Forms::ToolStripMenuItem^ toolStripMenuItem1;
	private: System::Windows::Forms::TabPage^ tabPage3;
	private: System::Windows::Forms::Panel^ panel1;
	public: System::Windows::Forms::DataVisualization::Charting::Chart^ dlSpeedChart;
	public: System::Windows::Forms::Button^ buttonRemove;
	public: System::Windows::Forms::TabPage^ progressTabPage;
	public: System::Windows::Forms::DataVisualization::Charting::Chart^ pieceChart;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ dataGridViewTextBoxColumn1;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ dataGridViewTextBoxColumn2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ dataGridViewTextBoxColumn3;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column5;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column6;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column1;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ dataGridViewTextBoxColumn4;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ torrentId;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ TorrentName;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ TorrentProgress;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column4;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ DL;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ DlBytes;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ UP;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ UpBytes;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ TorrentConnected;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ TorrentFoundPeers;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ TorrentDownloaded;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column3;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ gridPeerAddress;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ DL2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ DlBytes2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ UP2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ UpBytes2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ gridPeerPercentage;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ peerSource;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ peerCountry;
	private: System::Windows::Forms::NotifyIcon^ trayIcon;
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

		enum class TabType { General, Peers, Sources, Stats, Progress };
		TabType getActiveTab()
		{
			if (tabPage2->Visible)
				return TabType::Peers;
			if (tabPage3->Visible)
				return TabType::Stats;
			if (progressTabPage->Visible)
				return TabType::Progress;
			if (sourcesTab->Visible)
				return TabType::Sources;

			return TabType::General;
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
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle244 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle252 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle253 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle245 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle246 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle247 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle248 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle249 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle250 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle251 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle254 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle258 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle259 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle255 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle256 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle257 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle234 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle262 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle263 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle238 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle239 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle260 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle261 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataVisualization::Charting::ChartArea^ chartArea25 = (gcnew System::Windows::Forms::DataVisualization::Charting::ChartArea());
			System::Windows::Forms::DataVisualization::Charting::Legend^ legend13 = (gcnew System::Windows::Forms::DataVisualization::Charting::Legend());
			System::Windows::Forms::DataVisualization::Charting::Series^ series49 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^ series50 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::ChartArea^ chartArea26 = (gcnew System::Windows::Forms::DataVisualization::Charting::ChartArea());
			System::Windows::Forms::DataVisualization::Charting::Series^ series51 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::DataPoint^ dataPoint25 = (gcnew System::Windows::Forms::DataVisualization::Charting::DataPoint(2,
				1));
			System::Windows::Forms::DataVisualization::Charting::DataPoint^ dataPoint26 = (gcnew System::Windows::Forms::DataVisualization::Charting::DataPoint(3,
				1));
			System::Windows::Forms::DataVisualization::Charting::Series^ series52 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			this->splitContainer1 = (gcnew System::Windows::Forms::SplitContainer());
			this->splitContainer2 = (gcnew System::Windows::Forms::SplitContainer());
			this->buttonRemove = (gcnew System::Windows::Forms::Button());
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
			this->DL = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->DlBytes = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->UP = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->UpBytes = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentConnected = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentFoundPeers = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentDownloaded = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Column3 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->TorrentTabs = (gcnew System::Windows::Forms::TabControl());
			this->tabPage1 = (gcnew System::Windows::Forms::TabPage());
			this->torrentInfoLabel = (gcnew System::Windows::Forms::TextBox());
			this->tabPage2 = (gcnew System::Windows::Forms::TabPage());
			this->peersGridView = (gcnew System::Windows::Forms::DataGridView());
			this->gridPeerAddress = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->DL2 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->DlBytes2 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->UP2 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->UpBytes2 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
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
			this->Column5 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Column6 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Column1 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->dataGridViewTextBoxColumn4 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->tabPage3 = (gcnew System::Windows::Forms::TabPage());
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->dlSpeedChart = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->progressTabPage = (gcnew System::Windows::Forms::TabPage());
			this->pieceChart = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->trayIcon = (gcnew System::Windows::Forms::NotifyIcon(this->components));
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
			this->progressTabPage->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pieceChart))->BeginInit();
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
			this->splitContainer1->Size = System::Drawing::Size(1350, 497);
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
			this->splitContainer2->Panel1->Controls->Add(this->buttonRemove);
			this->splitContainer2->Panel1->Controls->Add(this->buttonSettings);
			this->splitContainer2->Panel1->Controls->Add(this->buttonAddMagnet);
			this->splitContainer2->Panel1->Controls->Add(this->buttonStop);
			this->splitContainer2->Panel1->Controls->Add(this->buttonStart);
			this->splitContainer2->Panel1->Controls->Add(this->buttonAddTorrent);
			// 
			// splitContainer2.Panel2
			// 
			this->splitContainer2->Panel2->Controls->Add(this->torrentsGrid);
			this->splitContainer2->Size = System::Drawing::Size(1350, 264);
			this->splitContainer2->SplitterDistance = 43;
			this->splitContainer2->TabIndex = 2;
			// 
			// buttonRemove
			// 
			this->buttonRemove->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(238)), static_cast<System::Int32>(static_cast<System::Byte>(157)),
				static_cast<System::Int32>(static_cast<System::Byte>(157)));
			this->buttonRemove->FlatStyle = System::Windows::Forms::FlatStyle::Popup;
			this->buttonRemove->Font = (gcnew System::Drawing::Font(L"Segoe UI Symbol", 7.8F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->buttonRemove->Location = System::Drawing::Point(486, 12);
			this->buttonRemove->Name = L"buttonRemove";
			this->buttonRemove->Size = System::Drawing::Size(100, 29);
			this->buttonRemove->TabIndex = 7;
			this->buttonRemove->Text = L"✕ Remove";
			this->buttonRemove->UseVisualStyleBackColor = false;
			this->buttonRemove->Click += gcnew System::EventHandler(this, &MainForm::ButtonRemove_Click);
			// 
			// buttonSettings
			// 
			this->buttonSettings->AutoSize = true;
			this->buttonSettings->Dock = System::Windows::Forms::DockStyle::Right;
			this->buttonSettings->FlatStyle = System::Windows::Forms::FlatStyle::Popup;
			this->buttonSettings->Font = (gcnew System::Drawing::Font(L"Segoe UI Symbol", 7.8F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->buttonSettings->Location = System::Drawing::Point(1232, 0);
			this->buttonSettings->Name = L"buttonSettings";
			this->buttonSettings->Size = System::Drawing::Size(118, 43);
			this->buttonSettings->TabIndex = 6;
			this->buttonSettings->Text = L"☰ Settings";
			this->buttonSettings->UseVisualStyleBackColor = true;
			this->buttonSettings->Click += gcnew System::EventHandler(this, &MainForm::buttonSettings_Click);
			// 
			// buttonAddMagnet
			// 
			this->buttonAddMagnet->BackColor = System::Drawing::Color::CornflowerBlue;
			this->buttonAddMagnet->FlatStyle = System::Windows::Forms::FlatStyle::Popup;
			this->buttonAddMagnet->Font = (gcnew System::Drawing::Font(L"Segoe UI Symbol", 7.8F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->buttonAddMagnet->ForeColor = System::Drawing::SystemColors::ButtonHighlight;
			this->buttonAddMagnet->Location = System::Drawing::Point(112, 12);
			this->buttonAddMagnet->Name = L"buttonAddMagnet";
			this->buttonAddMagnet->Size = System::Drawing::Size(100, 29);
			this->buttonAddMagnet->TabIndex = 5;
			this->buttonAddMagnet->Text = L"+ Magnet";
			this->buttonAddMagnet->UseVisualStyleBackColor = false;
			this->buttonAddMagnet->Click += gcnew System::EventHandler(this, &MainForm::buttonAddMagnet_Click);
			// 
			// buttonStop
			// 
			this->buttonStop->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(238)), static_cast<System::Int32>(static_cast<System::Byte>(157)),
				static_cast<System::Int32>(static_cast<System::Byte>(157)));
			this->buttonStop->FlatStyle = System::Windows::Forms::FlatStyle::Popup;
			this->buttonStop->Font = (gcnew System::Drawing::Font(L"Segoe UI Symbol", 7.8F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->buttonStop->Location = System::Drawing::Point(380, 12);
			this->buttonStop->Name = L"buttonStop";
			this->buttonStop->Size = System::Drawing::Size(100, 29);
			this->buttonStop->TabIndex = 4;
			this->buttonStop->Text = L"◻ Stop";
			this->buttonStop->UseVisualStyleBackColor = false;
			this->buttonStop->Click += gcnew System::EventHandler(this, &MainForm::buttonStop_Click);
			// 
			// buttonStart
			// 
			this->buttonStart->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(162)), static_cast<System::Int32>(static_cast<System::Byte>(219)),
				static_cast<System::Int32>(static_cast<System::Byte>(151)));
			this->buttonStart->FlatStyle = System::Windows::Forms::FlatStyle::Popup;
			this->buttonStart->Font = (gcnew System::Drawing::Font(L"Segoe UI Symbol", 7.8F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->buttonStart->Location = System::Drawing::Point(274, 12);
			this->buttonStart->Name = L"buttonStart";
			this->buttonStart->Size = System::Drawing::Size(100, 29);
			this->buttonStart->TabIndex = 3;
			this->buttonStart->Text = L"🡣 Start";
			this->buttonStart->UseVisualStyleBackColor = false;
			this->buttonStart->Click += gcnew System::EventHandler(this, &MainForm::buttonStart_Click);
			// 
			// buttonAddTorrent
			// 
			this->buttonAddTorrent->BackColor = System::Drawing::Color::CornflowerBlue;
			this->buttonAddTorrent->FlatStyle = System::Windows::Forms::FlatStyle::Popup;
			this->buttonAddTorrent->Font = (gcnew System::Drawing::Font(L"Segoe UI Symbol", 7.8F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->buttonAddTorrent->ForeColor = System::Drawing::SystemColors::ButtonHighlight;
			this->buttonAddTorrent->Location = System::Drawing::Point(12, 12);
			this->buttonAddTorrent->Name = L"buttonAddTorrent";
			this->buttonAddTorrent->Size = System::Drawing::Size(94, 29);
			this->buttonAddTorrent->TabIndex = 2;
			this->buttonAddTorrent->Text = L"+ File";
			this->buttonAddTorrent->UseVisualStyleBackColor = false;
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
			dataGridViewCellStyle244->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle244->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle244->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.2F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			dataGridViewCellStyle244->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle244->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle244->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle244->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->torrentsGrid->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle244;
			this->torrentsGrid->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->torrentsGrid->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(12) {
				this->torrentId,
					this->TorrentName, this->TorrentProgress, this->Column4, this->DL, this->DlBytes, this->UP, this->UpBytes, this->TorrentConnected,
					this->TorrentFoundPeers, this->TorrentDownloaded, this->Column3
			});
			dataGridViewCellStyle252->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleLeft;
			dataGridViewCellStyle252->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle252->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle252->ForeColor = System::Drawing::SystemColors::ControlText;
			dataGridViewCellStyle252->SelectionBackColor = System::Drawing::Color::SkyBlue;
			dataGridViewCellStyle252->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle252->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->torrentsGrid->DefaultCellStyle = dataGridViewCellStyle252;
			this->torrentsGrid->Dock = System::Windows::Forms::DockStyle::Fill;
			this->torrentsGrid->Location = System::Drawing::Point(0, 0);
			this->torrentsGrid->Name = L"torrentsGrid";
			this->torrentsGrid->ReadOnly = true;
			dataGridViewCellStyle253->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle253->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle253->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle253->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle253->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle253->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle253->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->torrentsGrid->RowHeadersDefaultCellStyle = dataGridViewCellStyle253;
			this->torrentsGrid->RowHeadersVisible = false;
			this->torrentsGrid->RowHeadersWidth = 51;
			this->torrentsGrid->RowTemplate->Height = 24;
			this->torrentsGrid->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->torrentsGrid->Size = System::Drawing::Size(1350, 217);
			this->torrentsGrid->TabIndex = 1;
			this->torrentsGrid->CellClick += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &MainForm::torrentsGrid_CellContentClick);
			this->torrentsGrid->CellMouseDoubleClick += gcnew System::Windows::Forms::DataGridViewCellMouseEventHandler(this, &MainForm::TorrentsGrid_CellContentClick_1);
			this->torrentsGrid->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::torrentsGrid_MouseClick);
			// 
			// torrentId
			// 
			this->torrentId->HeaderText = L"Id";
			this->torrentId->MinimumWidth = 6;
			this->torrentId->Name = L"torrentId";
			this->torrentId->ReadOnly = true;
			this->torrentId->Visible = false;
			this->torrentId->Width = 125;
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
			dataGridViewCellStyle245->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentProgress->DefaultCellStyle = dataGridViewCellStyle245;
			this->TorrentProgress->HeaderText = L"Progress";
			this->TorrentProgress->MinimumWidth = 6;
			this->TorrentProgress->Name = L"TorrentProgress";
			this->TorrentProgress->ReadOnly = true;
			this->TorrentProgress->Width = 125;
			// 
			// Column4
			// 
			this->Column4->HeaderText = L"State";
			this->Column4->MinimumWidth = 6;
			this->Column4->Name = L"Column4";
			this->Column4->ReadOnly = true;
			this->Column4->Width = 125;
			// 
			// DL
			// 
			dataGridViewCellStyle246->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->DL->DefaultCellStyle = dataGridViewCellStyle246;
			this->DL->HeaderText = L"Download";
			this->DL->MinimumWidth = 6;
			this->DL->Name = L"DL";
			this->DL->ReadOnly = true;
			this->DL->Width = 125;
			// 
			// DlBytes
			// 
			this->DlBytes->HeaderText = L"DlBytes";
			this->DlBytes->MinimumWidth = 6;
			this->DlBytes->Name = L"DlBytes";
			this->DlBytes->ReadOnly = true;
			this->DlBytes->Visible = false;
			this->DlBytes->Width = 125;
			// 
			// UP
			// 
			dataGridViewCellStyle247->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->UP->DefaultCellStyle = dataGridViewCellStyle247;
			this->UP->HeaderText = L"Upload";
			this->UP->MinimumWidth = 6;
			this->UP->Name = L"UP";
			this->UP->ReadOnly = true;
			this->UP->Width = 125;
			// 
			// UpBytes
			// 
			this->UpBytes->HeaderText = L"UpBytes";
			this->UpBytes->MinimumWidth = 6;
			this->UpBytes->Name = L"UpBytes";
			this->UpBytes->ReadOnly = true;
			this->UpBytes->Visible = false;
			this->UpBytes->Width = 125;
			// 
			// TorrentConnected
			// 
			dataGridViewCellStyle248->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentConnected->DefaultCellStyle = dataGridViewCellStyle248;
			this->TorrentConnected->HeaderText = L"Connected";
			this->TorrentConnected->MinimumWidth = 6;
			this->TorrentConnected->Name = L"TorrentConnected";
			this->TorrentConnected->ReadOnly = true;
			this->TorrentConnected->Width = 125;
			// 
			// TorrentFoundPeers
			// 
			dataGridViewCellStyle249->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentFoundPeers->DefaultCellStyle = dataGridViewCellStyle249;
			this->TorrentFoundPeers->HeaderText = L"Found Peers";
			this->TorrentFoundPeers->MinimumWidth = 6;
			this->TorrentFoundPeers->Name = L"TorrentFoundPeers";
			this->TorrentFoundPeers->ReadOnly = true;
			this->TorrentFoundPeers->Width = 125;
			// 
			// TorrentDownloaded
			// 
			dataGridViewCellStyle250->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentDownloaded->DefaultCellStyle = dataGridViewCellStyle250;
			this->TorrentDownloaded->HeaderText = L"Downloaded";
			this->TorrentDownloaded->MinimumWidth = 6;
			this->TorrentDownloaded->Name = L"TorrentDownloaded";
			this->TorrentDownloaded->ReadOnly = true;
			this->TorrentDownloaded->Width = 125;
			// 
			// Column3
			// 
			dataGridViewCellStyle251->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Column3->DefaultCellStyle = dataGridViewCellStyle251;
			this->Column3->HeaderText = L"Uploaded";
			this->Column3->MinimumWidth = 6;
			this->Column3->Name = L"Column3";
			this->Column3->ReadOnly = true;
			this->Column3->Width = 125;
			// 
			// TorrentTabs
			// 
			this->TorrentTabs->Controls->Add(this->tabPage1);
			this->TorrentTabs->Controls->Add(this->tabPage2);
			this->TorrentTabs->Controls->Add(this->sourcesTab);
			this->TorrentTabs->Controls->Add(this->tabPage3);
			this->TorrentTabs->Controls->Add(this->progressTabPage);
			this->TorrentTabs->Dock = System::Windows::Forms::DockStyle::Fill;
			this->TorrentTabs->Location = System::Drawing::Point(0, 0);
			this->TorrentTabs->Name = L"TorrentTabs";
			this->TorrentTabs->SelectedIndex = 0;
			this->TorrentTabs->Size = System::Drawing::Size(1350, 229);
			this->TorrentTabs->TabIndex = 0;
			// 
			// tabPage1
			// 
			this->tabPage1->Controls->Add(this->torrentInfoLabel);
			this->tabPage1->Location = System::Drawing::Point(4, 25);
			this->tabPage1->Name = L"tabPage1";
			this->tabPage1->Padding = System::Windows::Forms::Padding(3);
			this->tabPage1->Size = System::Drawing::Size(1342, 200);
			this->tabPage1->TabIndex = 0;
			this->tabPage1->Text = L"General";
			this->tabPage1->UseVisualStyleBackColor = true;
			// 
			// torrentInfoLabel
			// 
			this->torrentInfoLabel->Dock = System::Windows::Forms::DockStyle::Fill;
			this->torrentInfoLabel->Font = (gcnew System::Drawing::Font(L"Verdana", 8));
			this->torrentInfoLabel->Location = System::Drawing::Point(3, 3);
			this->torrentInfoLabel->Multiline = true;
			this->torrentInfoLabel->Name = L"torrentInfoLabel";
			this->torrentInfoLabel->ReadOnly = true;
			this->torrentInfoLabel->ScrollBars = System::Windows::Forms::ScrollBars::Both;
			this->torrentInfoLabel->Size = System::Drawing::Size(1336, 194);
			this->torrentInfoLabel->TabIndex = 0;
			// 
			// tabPage2
			// 
			this->tabPage2->Controls->Add(this->peersGridView);
			this->tabPage2->Location = System::Drawing::Point(4, 25);
			this->tabPage2->Name = L"tabPage2";
			this->tabPage2->Padding = System::Windows::Forms::Padding(3);
			this->tabPage2->Size = System::Drawing::Size(1342, 200);
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
			dataGridViewCellStyle254->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle254->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle254->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.2F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			dataGridViewCellStyle254->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle254->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle254->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle254->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->peersGridView->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle254;
			this->peersGridView->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->peersGridView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(8) {
				this->gridPeerAddress,
					this->DL2, this->DlBytes2, this->UP2, this->UpBytes2, this->gridPeerPercentage, this->peerSource, this->peerCountry
			});
			this->peersGridView->ContextMenuStrip = this->peersContextMenuStrip;
			dataGridViewCellStyle258->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleLeft;
			dataGridViewCellStyle258->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle258->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle258->ForeColor = System::Drawing::SystemColors::ControlText;
			dataGridViewCellStyle258->SelectionBackColor = System::Drawing::Color::SkyBlue;
			dataGridViewCellStyle258->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle258->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->peersGridView->DefaultCellStyle = dataGridViewCellStyle258;
			this->peersGridView->Dock = System::Windows::Forms::DockStyle::Fill;
			this->peersGridView->Location = System::Drawing::Point(3, 3);
			this->peersGridView->MultiSelect = false;
			this->peersGridView->Name = L"peersGridView";
			this->peersGridView->ReadOnly = true;
			dataGridViewCellStyle259->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle259->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle259->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle259->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle259->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle259->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle259->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->peersGridView->RowHeadersDefaultCellStyle = dataGridViewCellStyle259;
			this->peersGridView->RowHeadersVisible = false;
			this->peersGridView->RowHeadersWidth = 51;
			this->peersGridView->RowTemplate->Height = 24;
			this->peersGridView->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->peersGridView->Size = System::Drawing::Size(1336, 194);
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
			// DL2
			// 
			dataGridViewCellStyle255->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->DL2->DefaultCellStyle = dataGridViewCellStyle255;
			this->DL2->HeaderText = L"Download speed";
			this->DL2->MinimumWidth = 6;
			this->DL2->Name = L"DL2";
			this->DL2->ReadOnly = true;
			this->DL2->Width = 150;
			// 
			// DlBytes2
			// 
			this->DlBytes2->HeaderText = L"DlBytes2";
			this->DlBytes2->MinimumWidth = 6;
			this->DlBytes2->Name = L"DlBytes2";
			this->DlBytes2->ReadOnly = true;
			this->DlBytes2->Visible = false;
			this->DlBytes2->Width = 125;
			// 
			// UP2
			// 
			dataGridViewCellStyle256->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->UP2->DefaultCellStyle = dataGridViewCellStyle256;
			this->UP2->HeaderText = L"Upload speed";
			this->UP2->MinimumWidth = 6;
			this->UP2->Name = L"UP2";
			this->UP2->ReadOnly = true;
			this->UP2->Width = 150;
			// 
			// UpBytes2
			// 
			this->UpBytes2->HeaderText = L"UpBytes2";
			this->UpBytes2->MinimumWidth = 6;
			this->UpBytes2->Name = L"UpBytes2";
			this->UpBytes2->ReadOnly = true;
			this->UpBytes2->Visible = false;
			this->UpBytes2->Width = 125;
			// 
			// gridPeerPercentage
			// 
			dataGridViewCellStyle257->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->gridPeerPercentage->DefaultCellStyle = dataGridViewCellStyle257;
			this->gridPeerPercentage->HeaderText = L"%";
			this->gridPeerPercentage->MinimumWidth = 6;
			this->gridPeerPercentage->Name = L"gridPeerPercentage";
			this->gridPeerPercentage->ReadOnly = true;
			this->gridPeerPercentage->Width = 125;
			// 
			// peerSource
			// 
			this->peerSource->HeaderText = L"Source";
			this->peerSource->MinimumWidth = 6;
			this->peerSource->Name = L"peerSource";
			this->peerSource->ReadOnly = true;
			this->peerSource->Width = 150;
			// 
			// peerCountry
			// 
			this->peerCountry->HeaderText = L"Country";
			this->peerCountry->MinimumWidth = 6;
			this->peerCountry->Name = L"peerCountry";
			this->peerCountry->ReadOnly = true;
			this->peerCountry->Width = 125;
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
			this->sourcesTab->Size = System::Drawing::Size(1342, 200);
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
			dataGridViewCellStyle234->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle234->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle234->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.2F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			dataGridViewCellStyle234->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle234->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle234->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle234->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->sourcesGrid->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle234;
			this->sourcesGrid->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->sourcesGrid->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(7) {
				this->dataGridViewTextBoxColumn1,
					this->dataGridViewTextBoxColumn2, this->dataGridViewTextBoxColumn3, this->Column5, this->Column6, this->Column1, this->dataGridViewTextBoxColumn4
			});
			dataGridViewCellStyle262->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleLeft;
			dataGridViewCellStyle262->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle262->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle262->ForeColor = System::Drawing::SystemColors::ControlText;
			dataGridViewCellStyle262->SelectionBackColor = System::Drawing::Color::SkyBlue;
			dataGridViewCellStyle262->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle262->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->sourcesGrid->DefaultCellStyle = dataGridViewCellStyle262;
			this->sourcesGrid->Dock = System::Windows::Forms::DockStyle::Fill;
			this->sourcesGrid->Location = System::Drawing::Point(3, 3);
			this->sourcesGrid->Name = L"sourcesGrid";
			this->sourcesGrid->ReadOnly = true;
			dataGridViewCellStyle263->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle263->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle263->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle263->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle263->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle263->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle263->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->sourcesGrid->RowHeadersDefaultCellStyle = dataGridViewCellStyle263;
			this->sourcesGrid->RowHeadersVisible = false;
			this->sourcesGrid->RowHeadersWidth = 51;
			this->sourcesGrid->RowTemplate->Height = 24;
			this->sourcesGrid->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->sourcesGrid->Size = System::Drawing::Size(1336, 194);
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
			dataGridViewCellStyle238->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->dataGridViewTextBoxColumn2->DefaultCellStyle = dataGridViewCellStyle238;
			this->dataGridViewTextBoxColumn2->HeaderText = L"State";
			this->dataGridViewTextBoxColumn2->MinimumWidth = 6;
			this->dataGridViewTextBoxColumn2->Name = L"dataGridViewTextBoxColumn2";
			this->dataGridViewTextBoxColumn2->ReadOnly = true;
			this->dataGridViewTextBoxColumn2->Width = 120;
			// 
			// dataGridViewTextBoxColumn3
			// 
			dataGridViewCellStyle239->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->dataGridViewTextBoxColumn3->DefaultCellStyle = dataGridViewCellStyle239;
			this->dataGridViewTextBoxColumn3->HeaderText = L"Peers";
			this->dataGridViewTextBoxColumn3->MinimumWidth = 6;
			this->dataGridViewTextBoxColumn3->Name = L"dataGridViewTextBoxColumn3";
			this->dataGridViewTextBoxColumn3->ReadOnly = true;
			this->dataGridViewTextBoxColumn3->Width = 125;
			// 
			// Column5
			// 
			dataGridViewCellStyle260->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Column5->DefaultCellStyle = dataGridViewCellStyle260;
			this->Column5->HeaderText = L"Seeds";
			this->Column5->MinimumWidth = 6;
			this->Column5->Name = L"Column5";
			this->Column5->ReadOnly = true;
			this->Column5->Width = 125;
			// 
			// Column6
			// 
			dataGridViewCellStyle261->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Column6->DefaultCellStyle = dataGridViewCellStyle261;
			this->Column6->HeaderText = L"Leechers";
			this->Column6->MinimumWidth = 6;
			this->Column6->Name = L"Column6";
			this->Column6->ReadOnly = true;
			this->Column6->Width = 125;
			// 
			// Column1
			// 
			this->Column1->HeaderText = L"Next check";
			this->Column1->MinimumWidth = 6;
			this->Column1->Name = L"Column1";
			this->Column1->ReadOnly = true;
			this->Column1->Width = 125;
			// 
			// dataGridViewTextBoxColumn4
			// 
			this->dataGridViewTextBoxColumn4->HeaderText = L"Interval";
			this->dataGridViewTextBoxColumn4->MinimumWidth = 6;
			this->dataGridViewTextBoxColumn4->Name = L"dataGridViewTextBoxColumn4";
			this->dataGridViewTextBoxColumn4->ReadOnly = true;
			this->dataGridViewTextBoxColumn4->Width = 125;
			// 
			// tabPage3
			// 
			this->tabPage3->Controls->Add(this->panel1);
			this->tabPage3->Location = System::Drawing::Point(4, 25);
			this->tabPage3->Name = L"tabPage3";
			this->tabPage3->Padding = System::Windows::Forms::Padding(3);
			this->tabPage3->Size = System::Drawing::Size(1342, 200);
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
			this->panel1->Size = System::Drawing::Size(1336, 194);
			this->panel1->TabIndex = 0;
			// 
			// dlSpeedChart
			// 
			this->dlSpeedChart->BorderlineWidth = 5;
			chartArea25->Name = L"ChartArea1";
			this->dlSpeedChart->ChartAreas->Add(chartArea25);
			this->dlSpeedChart->Dock = System::Windows::Forms::DockStyle::Fill;
			legend13->Name = L"Legend1";
			this->dlSpeedChart->Legends->Add(legend13);
			this->dlSpeedChart->Location = System::Drawing::Point(0, 0);
			this->dlSpeedChart->Name = L"dlSpeedChart";
			this->dlSpeedChart->Palette = System::Windows::Forms::DataVisualization::Charting::ChartColorPalette::Bright;
			series49->ChartArea = L"ChartArea1";
			series49->Legend = L"Legend1";
			series49->Name = L"DlSeries";
			series50->ChartArea = L"ChartArea1";
			series50->Legend = L"Legend1";
			series50->Name = L"UpSeries";
			this->dlSpeedChart->Series->Add(series49);
			this->dlSpeedChart->Series->Add(series50);
			this->dlSpeedChart->Size = System::Drawing::Size(1336, 194);
			this->dlSpeedChart->TabIndex = 0;
			this->dlSpeedChart->Text = L"Download speed";
			// 
			// progressTabPage
			// 
			this->progressTabPage->Controls->Add(this->pieceChart);
			this->progressTabPage->Location = System::Drawing::Point(4, 25);
			this->progressTabPage->Name = L"progressTabPage";
			this->progressTabPage->Padding = System::Windows::Forms::Padding(3);
			this->progressTabPage->Size = System::Drawing::Size(1342, 200);
			this->progressTabPage->TabIndex = 4;
			this->progressTabPage->Text = L"Progress";
			this->progressTabPage->UseVisualStyleBackColor = true;
			// 
			// pieceChart
			// 
			this->pieceChart->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->pieceChart->BorderlineWidth = 0;
			chartArea26->AxisX->Crossing = 1.7976931348623157E+308;
			chartArea26->AxisX->Enabled = System::Windows::Forms::DataVisualization::Charting::AxisEnabled::True;
			chartArea26->AxisX->InterlacedColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(255)),
				static_cast<System::Int32>(static_cast<System::Byte>(192)), static_cast<System::Int32>(static_cast<System::Byte>(255)));
			chartArea26->AxisX->Interval = 10;
			chartArea26->AxisX->IntervalAutoMode = System::Windows::Forms::DataVisualization::Charting::IntervalAutoMode::VariableCount;
			chartArea26->AxisX->IntervalOffsetType = System::Windows::Forms::DataVisualization::Charting::DateTimeIntervalType::Number;
			chartArea26->AxisX->IsMarginVisible = false;
			chartArea26->AxisX->LabelStyle->Enabled = false;
			chartArea26->AxisX->Minimum = 0;
			chartArea26->AxisX->Title = L"Pieces";
			chartArea26->AxisX->TitleAlignment = System::Drawing::StringAlignment::Near;
			chartArea26->AxisY->Crossing = 1.7976931348623157E+308;
			chartArea26->AxisY->Enabled = System::Windows::Forms::DataVisualization::Charting::AxisEnabled::False;
			chartArea26->AxisY->Interval = 1;
			chartArea26->AxisY->IsMarginVisible = false;
			chartArea26->AxisY->Maximum = 1;
			chartArea26->AxisY->MaximumAutoSize = 1;
			chartArea26->AxisY->Minimum = 0;
			chartArea26->AxisY->TitleAlignment = System::Drawing::StringAlignment::Far;
			chartArea26->BackColor = System::Drawing::Color::Silver;
			chartArea26->BorderWidth = 0;
			chartArea26->IsSameFontSizeForAllAxes = true;
			chartArea26->Name = L"ChartArea1";
			this->pieceChart->ChartAreas->Add(chartArea26);
			this->pieceChart->Location = System::Drawing::Point(3, 25);
			this->pieceChart->Name = L"pieceChart";
			this->pieceChart->Palette = System::Windows::Forms::DataVisualization::Charting::ChartColorPalette::Fire;
			series51->ChartArea = L"ChartArea1";
			series51->Color = System::Drawing::Color::RoyalBlue;
			series51->CustomProperties = L"PointWidth=2";
			series51->Name = L"HasSeries";
			series51->Points->Add(dataPoint25);
			series51->Points->Add(dataPoint26);
			series52->ChartArea = L"ChartArea1";
			series52->Color = System::Drawing::Color::DarkOrange;
			series52->Name = L"Request";
			this->pieceChart->Series->Add(series51);
			this->pieceChart->Series->Add(series52);
			this->pieceChart->Size = System::Drawing::Size(1325, 62);
			this->pieceChart->TabIndex = 0;
			this->pieceChart->Text = L"pieceChart";
			this->pieceChart->Visible = false;
			// 
			// trayIcon
			// 
			this->trayIcon->Text = L"mtTorrent";
			this->trayIcon->Visible = true;
			this->trayIcon->MouseDoubleClick += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::trayIcon_MouseDoubleClick);
			// 
			// MainForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1350, 497);
			this->Controls->Add(this->splitContainer1);
			this->Name = L"MainForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"mtTorrent";
			this->Resize += gcnew System::EventHandler(this, &MainForm::MainForm_Resize);
			this->splitContainer1->Panel1->ResumeLayout(false);
			this->splitContainer1->Panel2->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainer1))->EndInit();
			this->splitContainer1->ResumeLayout(false);
			this->splitContainer2->Panel1->ResumeLayout(false);
			this->splitContainer2->Panel1->PerformLayout();
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
			this->progressTabPage->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pieceChart))->EndInit();
			this->ResumeLayout(false);

		}
#pragma endregion
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
		onButtonClick(ButtonId::AddTorrentFile);
	}

private: System::Void torrentsGrid_CellContentClick(System::Object^  sender, System::Windows::Forms::DataGridViewCellEventArgs^  e) {
	onButtonClick(ButtonId::TorrentGrid);
}
	private: System::Void torrentsGrid_MouseClick(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e) {
		if (e->Button == System::Windows::Forms::MouseButtons::Right)
		{
			int currentMouseOverRow = torrentsGrid->HitTest(e->X, e->Y)->RowIndex;

			if (currentMouseOverRow >= 0)
			{
				torrentsGrid->ClearSelection();
				torrentsGrid->Rows[currentMouseOverRow]->Selected = true;
				onButtonClick(ButtonId::TorrentGrid);

				auto info = getTorrentContexMenuInfo();

				auto m = gcnew System::Windows::Forms::ContextMenu();
				auto clickEvent = gcnew System::Windows::Forms::MenuItem("Open location");
				clickEvent->Click += gcnew System::EventHandler(this, &MainForm::menuItem_Click);
				m->MenuItems->Add(clickEvent);

				if (!info.active)
				{
					clickEvent = gcnew System::Windows::Forms::MenuItem("Schedule");
					clickEvent->Click += gcnew System::EventHandler(this, &MainForm::menuItem_Click);
					m->MenuItems->Add(clickEvent);
				}

				if (info.utmLogs)
				{
					clickEvent = gcnew System::Windows::Forms::MenuItem("Magnet logs");
					clickEvent->Click += gcnew System::EventHandler(this, &MainForm::menuItem_Click);
					m->MenuItems->Add(clickEvent);
				}

				if (!info.noInfo)
				{
					clickEvent = gcnew System::Windows::Forms::MenuItem("Files");
					clickEvent->Click += gcnew System::EventHandler(this, &MainForm::menuItem_Click);
					m->MenuItems->Add(clickEvent);

					clickEvent = gcnew System::Windows::Forms::MenuItem("Check files");
					clickEvent->Click += gcnew System::EventHandler(this, &MainForm::menuItem_Click);
					m->MenuItems->Add(clickEvent);
				}

				m->Show(torrentsGrid, System::Drawing::Point(e->X, e->Y));
			}
		}
	}
	private: System::Void menuItem_Click(System::Object^ sender, System::EventArgs^ e) {
		auto buttonText = ((System::Windows::Forms::MenuItem^)sender)->Text;

		if(buttonText == "Open location")
			onButtonClick(ButtonId::OpenLocation);
		else if (buttonText == "Schedule")
			onButtonClick(ButtonId::Schedule);
		else if (buttonText == "Magnet logs")
			onButtonClick(ButtonId::MagnetLogs);
		else if (buttonText == "Files")
			onButtonClick(ButtonId::SelectFiles);
		else if (buttonText == "Check files")
			onButtonClick(ButtonId::CheckFiles);
	}
	private: System::Void buttonStart_Click(System::Object^  sender, System::EventArgs^  e) {
		onButtonClick(ButtonId::Start);
	}
	private: System::Void buttonStop_Click(System::Object^  sender, System::EventArgs^  e) {
		onButtonClick(ButtonId::Stop);
	}
private: System::Void buttonAddMagnet_Click(System::Object^  sender, System::EventArgs^  e) {
	onButtonClick(ButtonId::AddTorrentMagnet);
}
private: System::Void buttonSettings_Click(System::Object^  sender, System::EventArgs^  e) {
	onButtonClick(ButtonId::Settings);
}
System::String^ ctxMenuHint;
private: System::Void ctxMenu_Click(System::Object^  sender, System::EventArgs^  e) {
	onButtonClick(ButtonId::SourceRefresh, ctxMenuHint);
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
private: System::Void ToolStripMenuItem1_Click(System::Object^ sender, System::EventArgs^ e) {
	onButtonClick(ButtonId::AddPeerMenu);
}
private: System::Void TorrentsGrid_CellContentClick_1(System::Object^ sender, System::Windows::Forms::DataGridViewCellMouseEventArgs^ e) {
	onButtonClick(ButtonId::TorrentDoubleClick);
}
private: System::Void ButtonRemove_Click(System::Object^ sender, System::EventArgs^ e) {
	onButtonClick(ButtonId::Remove);
}

private: System::Void  torrentGridView_SortCompare(System::Object^ sender, DataGridViewSortCompareEventArgs^ e)
{
	if(e->Column->Name == "UP" || e->Column->Name == "DL")
	{
		auto bytesID = gcnew String(e->Column->Name == "DL" ? "DlBytes" : "UpBytes");
		auto v1 = Int32::Parse(torrentsGrid->Rows[e->RowIndex1]->Cells[bytesID]->Value->ToString());
		auto v2 = Int32::Parse(torrentsGrid->Rows[e->RowIndex2]->Cells[bytesID]->Value->ToString());
		e->SortResult = v1 > v2 ? 1 : -1;
	}
	else
		e->SortResult = System::String::Compare(e->CellValue1->ToString(), e->CellValue2->ToString());

	e->Handled = true;
}

private: System::Void  peersView_SortCompare(System::Object^ sender, DataGridViewSortCompareEventArgs^ e)
{
	if (e->Column->Name == "UP2" || e->Column->Name == "DL2")
	{
		auto bytesID = gcnew String(e->Column->Name == "DL2" ? "DlBytes2" : "UpBytes2");
		auto v1 = Int32::Parse(peersGridView->Rows[e->RowIndex1]->Cells[bytesID]->Value->ToString());
		auto v2 = Int32::Parse(peersGridView->Rows[e->RowIndex2]->Cells[bytesID]->Value->ToString());
		e->SortResult = v1 > v2 ? 1 : -1;
	}
	else
		e->SortResult = System::String::Compare(e->CellValue1->ToString(), e->CellValue2->ToString());

	e->Handled = true;
}

private: System::Void trayIcon_MouseDoubleClick(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e) {
	Show();
	WindowState = FormWindowState::Normal;
}
private: System::Void MainForm_Resize(System::Object^ sender, System::EventArgs^ e) {
	if (WindowState == FormWindowState::Minimized)
	{
		Hide();
	}
}
};
}
