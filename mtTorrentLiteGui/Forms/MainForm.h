#pragma once

#include "../AppCoreCallbacks.h"

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
			filesProgressGridView->SortCompare += gcnew DataGridViewSortCompareEventHandler(this, &MainForm::filesProgressGridView_SortCompare);

			instance = this;

			try
			{
				this->Icon = gcnew System::Drawing::Icon("data/mttIcon.ico");// (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			}
			catch (...)
			{
			}
		}

		static System::Windows::Forms::Timer^ schedulerTimer;
		static System::Windows::Forms::Timer^ updateTimer;

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












	private: System::Windows::Forms::DataGridViewTextBoxColumn^ gridPeerAddress;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ DL2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ DlBytes2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ UP2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ UpBytes2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ gridPeerPercentage;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ peerSource;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ peerCountry;
	private: System::Windows::Forms::NotifyIcon^ trayIcon;
	public: System::Windows::Forms::DataGridView^ filesProgressGridView;

	public:


























































private: System::Windows::Forms::DataGridViewTextBoxColumn^ Index;
private: System::Windows::Forms::DataGridViewCheckBoxColumn^ Selected;
private: System::Windows::Forms::DataGridViewTextBoxColumn^ dataGridViewTextBoxColumn5;
private: System::Windows::Forms::DataGridViewTextBoxColumn^ Progress;
private: System::Windows::Forms::DataGridViewTextBoxColumn^ Size;
private: System::Windows::Forms::DataGridViewTextBoxColumn^ SizeBytes;
private: System::Windows::Forms::DataGridViewTextBoxColumn^ PiecesCount;
private: System::Windows::Forms::DataGridViewTextBoxColumn^ PiecesRemaining;
private: System::Windows::Forms::DataGridViewTextBoxColumn^ PiecesActive;
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





































































































































































































































































	private:





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
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle10 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle11 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle12 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle16 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle17 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle13 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle14 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle15 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle18 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle23 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle24 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle19 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle20 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle21 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle22 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataVisualization::Charting::ChartArea^ chartArea1 = (gcnew System::Windows::Forms::DataVisualization::Charting::ChartArea());
			System::Windows::Forms::DataVisualization::Charting::Legend^ legend1 = (gcnew System::Windows::Forms::DataVisualization::Charting::Legend());
			System::Windows::Forms::DataVisualization::Charting::Series^ series1 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Series^ series2 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle25 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle31 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle32 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle26 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle27 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle28 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle29 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle30 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataVisualization::Charting::ChartArea^ chartArea2 = (gcnew System::Windows::Forms::DataVisualization::Charting::ChartArea());
			System::Windows::Forms::DataVisualization::Charting::Series^ series3 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::DataPoint^ dataPoint1 = (gcnew System::Windows::Forms::DataVisualization::Charting::DataPoint(2,
				1));
			System::Windows::Forms::DataVisualization::Charting::DataPoint^ dataPoint2 = (gcnew System::Windows::Forms::DataVisualization::Charting::DataPoint(3,
				1));
			System::Windows::Forms::DataVisualization::Charting::Series^ series4 = (gcnew System::Windows::Forms::DataVisualization::Charting::Series());
			System::Windows::Forms::DataVisualization::Charting::Title^ title1 = (gcnew System::Windows::Forms::DataVisualization::Charting::Title());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle2 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle3 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle4 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle5 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle6 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle7 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle8 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle9 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			this->splitContainer1 = (gcnew System::Windows::Forms::SplitContainer());
			this->splitContainer2 = (gcnew System::Windows::Forms::SplitContainer());
			this->buttonRemove = (gcnew System::Windows::Forms::Button());
			this->buttonSettings = (gcnew System::Windows::Forms::Button());
			this->buttonAddMagnet = (gcnew System::Windows::Forms::Button());
			this->buttonStop = (gcnew System::Windows::Forms::Button());
			this->buttonStart = (gcnew System::Windows::Forms::Button());
			this->buttonAddTorrent = (gcnew System::Windows::Forms::Button());
			this->torrentsGrid = (gcnew System::Windows::Forms::DataGridView());
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
			this->filesProgressGridView = (gcnew System::Windows::Forms::DataGridView());
			this->Index = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Selected = (gcnew System::Windows::Forms::DataGridViewCheckBoxColumn());
			this->dataGridViewTextBoxColumn5 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Progress = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Size = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->SizeBytes = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->PiecesCount = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->PiecesRemaining = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->PiecesActive = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->pieceChart = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->trayIcon = (gcnew System::Windows::Forms::NotifyIcon(this->components));
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
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->filesProgressGridView))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pieceChart))->BeginInit();
			this->SuspendLayout();
			// 
			// splitContainer1
			// 
			this->splitContainer1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->splitContainer1->Location = System::Drawing::Point(0, 0);
			this->splitContainer1->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
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
			this->splitContainer1->Size = System::Drawing::Size(1377, 525);
			this->splitContainer1->SplitterDistance = 277;
			this->splitContainer1->TabIndex = 3;
			// 
			// splitContainer2
			// 
			this->splitContainer2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->splitContainer2->FixedPanel = System::Windows::Forms::FixedPanel::Panel1;
			this->splitContainer2->Location = System::Drawing::Point(0, 0);
			this->splitContainer2->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
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
			this->splitContainer2->Size = System::Drawing::Size(1377, 277);
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
			this->buttonRemove->Location = System::Drawing::Point(485, 12);
			this->buttonRemove->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->buttonRemove->Name = L"buttonRemove";
			this->buttonRemove->Size = System::Drawing::Size(100, 30);
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
			this->buttonSettings->Location = System::Drawing::Point(1260, 0);
			this->buttonSettings->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->buttonSettings->Name = L"buttonSettings";
			this->buttonSettings->Size = System::Drawing::Size(117, 43);
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
			this->buttonAddMagnet->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->buttonAddMagnet->Name = L"buttonAddMagnet";
			this->buttonAddMagnet->Size = System::Drawing::Size(100, 30);
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
			this->buttonStop->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->buttonStop->Name = L"buttonStop";
			this->buttonStop->Size = System::Drawing::Size(100, 30);
			this->buttonStop->TabIndex = 4;
			this->buttonStop->Text = L"⧠ Stop";
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
			this->buttonStart->Location = System::Drawing::Point(275, 12);
			this->buttonStart->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->buttonStart->Name = L"buttonStart";
			this->buttonStart->Size = System::Drawing::Size(100, 30);
			this->buttonStart->TabIndex = 3;
			this->buttonStart->Text = L"➔ Start";
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
			this->buttonAddTorrent->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->buttonAddTorrent->Name = L"buttonAddTorrent";
			this->buttonAddTorrent->Size = System::Drawing::Size(93, 30);
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
			this->torrentsGrid->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(12) {
				this->torrentId,
					this->TorrentName, this->TorrentProgress, this->Column4, this->DL, this->DlBytes, this->UP, this->UpBytes, this->TorrentConnected,
					this->TorrentFoundPeers, this->TorrentDownloaded, this->Column3
			});
			dataGridViewCellStyle10->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleLeft;
			dataGridViewCellStyle10->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle10->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle10->ForeColor = System::Drawing::SystemColors::ControlText;
			dataGridViewCellStyle10->SelectionBackColor = System::Drawing::Color::SkyBlue;
			dataGridViewCellStyle10->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle10->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->torrentsGrid->DefaultCellStyle = dataGridViewCellStyle10;
			this->torrentsGrid->Dock = System::Windows::Forms::DockStyle::Fill;
			this->torrentsGrid->Location = System::Drawing::Point(0, 0);
			this->torrentsGrid->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->torrentsGrid->Name = L"torrentsGrid";
			this->torrentsGrid->ReadOnly = true;
			dataGridViewCellStyle11->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle11->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle11->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle11->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle11->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle11->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle11->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->torrentsGrid->RowHeadersDefaultCellStyle = dataGridViewCellStyle11;
			this->torrentsGrid->RowHeadersVisible = false;
			this->torrentsGrid->RowHeadersWidth = 51;
			this->torrentsGrid->RowTemplate->Height = 24;
			this->torrentsGrid->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->torrentsGrid->Size = System::Drawing::Size(1377, 230);
			this->torrentsGrid->TabIndex = 1;
			this->torrentsGrid->CellClick += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &MainForm::torrentsGrid_CellContentClick);
			this->torrentsGrid->CellMouseDoubleClick += gcnew System::Windows::Forms::DataGridViewCellMouseEventHandler(this, &MainForm::TorrentsGrid_CellContentClick_1);
			this->torrentsGrid->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::torrentsGrid_MouseClick);
			this->torrentsGrid->Sorted += gcnew System::EventHandler(this, &MainForm::torrentsGridView_Sorted);
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
			this->TorrentTabs->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->TorrentTabs->Name = L"TorrentTabs";
			this->TorrentTabs->SelectedIndex = 0;
			this->TorrentTabs->Size = System::Drawing::Size(1377, 244);
			this->TorrentTabs->TabIndex = 0;
			this->TorrentTabs->Selecting += gcnew System::Windows::Forms::TabControlCancelEventHandler(this, &MainForm::filesProgressGridView_TabClick);
			// 
			// tabPage1
			// 
			this->tabPage1->Controls->Add(this->torrentInfoLabel);
			this->tabPage1->Location = System::Drawing::Point(4, 25);
			this->tabPage1->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->tabPage1->Name = L"tabPage1";
			this->tabPage1->Padding = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->tabPage1->Size = System::Drawing::Size(1369, 215);
			this->tabPage1->TabIndex = 0;
			this->tabPage1->Text = L"General";
			this->tabPage1->UseVisualStyleBackColor = true;
			// 
			// torrentInfoLabel
			// 
			this->torrentInfoLabel->Dock = System::Windows::Forms::DockStyle::Fill;
			this->torrentInfoLabel->Font = (gcnew System::Drawing::Font(L"Verdana", 8));
			this->torrentInfoLabel->Location = System::Drawing::Point(3, 2);
			this->torrentInfoLabel->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->torrentInfoLabel->Multiline = true;
			this->torrentInfoLabel->Name = L"torrentInfoLabel";
			this->torrentInfoLabel->ReadOnly = true;
			this->torrentInfoLabel->ScrollBars = System::Windows::Forms::ScrollBars::Both;
			this->torrentInfoLabel->Size = System::Drawing::Size(1363, 211);
			this->torrentInfoLabel->TabIndex = 0;
			// 
			// tabPage2
			// 
			this->tabPage2->Controls->Add(this->peersGridView);
			this->tabPage2->Location = System::Drawing::Point(4, 25);
			this->tabPage2->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->tabPage2->Name = L"tabPage2";
			this->tabPage2->Padding = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->tabPage2->Size = System::Drawing::Size(1336, 215);
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
			dataGridViewCellStyle12->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle12->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle12->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.2F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			dataGridViewCellStyle12->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle12->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle12->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle12->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->peersGridView->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle12;
			this->peersGridView->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->peersGridView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(8) {
				this->gridPeerAddress,
					this->DL2, this->DlBytes2, this->UP2, this->UpBytes2, this->gridPeerPercentage, this->peerSource, this->peerCountry
			});
			this->peersGridView->ContextMenuStrip = this->peersContextMenuStrip;
			dataGridViewCellStyle16->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleLeft;
			dataGridViewCellStyle16->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle16->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle16->ForeColor = System::Drawing::SystemColors::ControlText;
			dataGridViewCellStyle16->SelectionBackColor = System::Drawing::Color::SkyBlue;
			dataGridViewCellStyle16->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle16->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->peersGridView->DefaultCellStyle = dataGridViewCellStyle16;
			this->peersGridView->Dock = System::Windows::Forms::DockStyle::Fill;
			this->peersGridView->Location = System::Drawing::Point(3, 2);
			this->peersGridView->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->peersGridView->MultiSelect = false;
			this->peersGridView->Name = L"peersGridView";
			this->peersGridView->ReadOnly = true;
			dataGridViewCellStyle17->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle17->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle17->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle17->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle17->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle17->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle17->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->peersGridView->RowHeadersDefaultCellStyle = dataGridViewCellStyle17;
			this->peersGridView->RowHeadersVisible = false;
			this->peersGridView->RowHeadersWidth = 51;
			this->peersGridView->RowTemplate->Height = 24;
			this->peersGridView->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->peersGridView->Size = System::Drawing::Size(1330, 211);
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
			dataGridViewCellStyle13->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->DL2->DefaultCellStyle = dataGridViewCellStyle13;
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
			dataGridViewCellStyle14->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->UP2->DefaultCellStyle = dataGridViewCellStyle14;
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
			dataGridViewCellStyle15->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->gridPeerPercentage->DefaultCellStyle = dataGridViewCellStyle15;
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
			this->sourcesTab->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->sourcesTab->Name = L"sourcesTab";
			this->sourcesTab->Padding = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->sourcesTab->Size = System::Drawing::Size(1336, 215);
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
			dataGridViewCellStyle18->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle18->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle18->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.2F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			dataGridViewCellStyle18->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle18->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle18->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle18->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->sourcesGrid->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle18;
			this->sourcesGrid->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->sourcesGrid->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(7) {
				this->dataGridViewTextBoxColumn1,
					this->dataGridViewTextBoxColumn2, this->dataGridViewTextBoxColumn3, this->Column5, this->Column6, this->Column1, this->dataGridViewTextBoxColumn4
			});
			dataGridViewCellStyle23->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleLeft;
			dataGridViewCellStyle23->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle23->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle23->ForeColor = System::Drawing::SystemColors::ControlText;
			dataGridViewCellStyle23->SelectionBackColor = System::Drawing::Color::SkyBlue;
			dataGridViewCellStyle23->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle23->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->sourcesGrid->DefaultCellStyle = dataGridViewCellStyle23;
			this->sourcesGrid->Dock = System::Windows::Forms::DockStyle::Fill;
			this->sourcesGrid->Location = System::Drawing::Point(3, 2);
			this->sourcesGrid->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->sourcesGrid->Name = L"sourcesGrid";
			this->sourcesGrid->ReadOnly = true;
			dataGridViewCellStyle24->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle24->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle24->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle24->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle24->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle24->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle24->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->sourcesGrid->RowHeadersDefaultCellStyle = dataGridViewCellStyle24;
			this->sourcesGrid->RowHeadersVisible = false;
			this->sourcesGrid->RowHeadersWidth = 51;
			this->sourcesGrid->RowTemplate->Height = 24;
			this->sourcesGrid->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->sourcesGrid->Size = System::Drawing::Size(1330, 211);
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
			dataGridViewCellStyle19->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->dataGridViewTextBoxColumn2->DefaultCellStyle = dataGridViewCellStyle19;
			this->dataGridViewTextBoxColumn2->HeaderText = L"State";
			this->dataGridViewTextBoxColumn2->MinimumWidth = 6;
			this->dataGridViewTextBoxColumn2->Name = L"dataGridViewTextBoxColumn2";
			this->dataGridViewTextBoxColumn2->ReadOnly = true;
			this->dataGridViewTextBoxColumn2->Width = 120;
			// 
			// dataGridViewTextBoxColumn3
			// 
			dataGridViewCellStyle20->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->dataGridViewTextBoxColumn3->DefaultCellStyle = dataGridViewCellStyle20;
			this->dataGridViewTextBoxColumn3->HeaderText = L"Peers";
			this->dataGridViewTextBoxColumn3->MinimumWidth = 6;
			this->dataGridViewTextBoxColumn3->Name = L"dataGridViewTextBoxColumn3";
			this->dataGridViewTextBoxColumn3->ReadOnly = true;
			this->dataGridViewTextBoxColumn3->Width = 125;
			// 
			// Column5
			// 
			dataGridViewCellStyle21->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Column5->DefaultCellStyle = dataGridViewCellStyle21;
			this->Column5->HeaderText = L"Seeds";
			this->Column5->MinimumWidth = 6;
			this->Column5->Name = L"Column5";
			this->Column5->ReadOnly = true;
			this->Column5->Width = 125;
			// 
			// Column6
			// 
			dataGridViewCellStyle22->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Column6->DefaultCellStyle = dataGridViewCellStyle22;
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
			this->tabPage3->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->tabPage3->Name = L"tabPage3";
			this->tabPage3->Padding = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->tabPage3->Size = System::Drawing::Size(1336, 215);
			this->tabPage3->TabIndex = 3;
			this->tabPage3->Text = L"Statistics";
			this->tabPage3->UseVisualStyleBackColor = true;
			// 
			// panel1
			// 
			this->panel1->Controls->Add(this->dlSpeedChart);
			this->panel1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel1->Location = System::Drawing::Point(3, 2);
			this->panel1->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(1330, 211);
			this->panel1->TabIndex = 0;
			// 
			// dlSpeedChart
			// 
			this->dlSpeedChart->BorderlineWidth = 5;
			chartArea1->AxisX->Title = L"Time (seconds)";
			chartArea1->AxisY->Title = L"MB/s";
			chartArea1->Name = L"ChartArea1";
			this->dlSpeedChart->ChartAreas->Add(chartArea1);
			this->dlSpeedChart->Dock = System::Windows::Forms::DockStyle::Fill;
			legend1->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(224)), static_cast<System::Int32>(static_cast<System::Byte>(224)),
				static_cast<System::Int32>(static_cast<System::Byte>(224)));
			legend1->Docking = System::Windows::Forms::DataVisualization::Charting::Docking::Top;
			legend1->IsDockedInsideChartArea = false;
			legend1->ItemColumnSpacing = 0;
			legend1->LegendStyle = System::Windows::Forms::DataVisualization::Charting::LegendStyle::Row;
			legend1->MaximumAutoSize = 10;
			legend1->Name = L"Legend1";
			legend1->TableStyle = System::Windows::Forms::DataVisualization::Charting::LegendTableStyle::Tall;
			legend1->TextWrapThreshold = 0;
			this->dlSpeedChart->Legends->Add(legend1);
			this->dlSpeedChart->Location = System::Drawing::Point(0, 0);
			this->dlSpeedChart->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->dlSpeedChart->Name = L"dlSpeedChart";
			this->dlSpeedChart->Palette = System::Windows::Forms::DataVisualization::Charting::ChartColorPalette::Bright;
			series1->ChartArea = L"ChartArea1";
			series1->Legend = L"Legend1";
			series1->LegendText = L"Download";
			series1->Name = L"DlSeries";
			series2->ChartArea = L"ChartArea1";
			series2->Legend = L"Legend1";
			series2->LegendText = L"Upload";
			series2->Name = L"UpSeries";
			this->dlSpeedChart->Series->Add(series1);
			this->dlSpeedChart->Series->Add(series2);
			this->dlSpeedChart->Size = System::Drawing::Size(1330, 211);
			this->dlSpeedChart->TabIndex = 0;
			this->dlSpeedChart->Text = L"Download speed";
			// 
			// progressTabPage
			// 
			this->progressTabPage->Controls->Add(this->filesProgressGridView);
			this->progressTabPage->Controls->Add(this->pieceChart);
			this->progressTabPage->Location = System::Drawing::Point(4, 25);
			this->progressTabPage->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->progressTabPage->Name = L"progressTabPage";
			this->progressTabPage->Padding = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->progressTabPage->Size = System::Drawing::Size(1336, 215);
			this->progressTabPage->TabIndex = 4;
			this->progressTabPage->Text = L"Progress";
			this->progressTabPage->UseVisualStyleBackColor = true;
			// 
			// filesProgressGridView
			// 
			this->filesProgressGridView->AllowUserToAddRows = false;
			this->filesProgressGridView->AllowUserToDeleteRows = false;
			this->filesProgressGridView->AllowUserToOrderColumns = true;
			this->filesProgressGridView->AllowUserToResizeRows = false;
			this->filesProgressGridView->AutoSizeColumnsMode = System::Windows::Forms::DataGridViewAutoSizeColumnsMode::Fill;
			this->filesProgressGridView->BackgroundColor = System::Drawing::SystemColors::Window;
			this->filesProgressGridView->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->filesProgressGridView->CellBorderStyle = System::Windows::Forms::DataGridViewCellBorderStyle::SunkenHorizontal;
			dataGridViewCellStyle25->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle25->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle25->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.2F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			dataGridViewCellStyle25->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle25->SelectionBackColor = System::Drawing::SystemColors::Desktop;
			dataGridViewCellStyle25->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle25->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->filesProgressGridView->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle25;
			this->filesProgressGridView->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->filesProgressGridView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(9) {
				this->Index,
					this->Selected, this->dataGridViewTextBoxColumn5, this->Progress, this->Size, this->SizeBytes, this->PiecesCount, this->PiecesRemaining,
					this->PiecesActive
			});
			dataGridViewCellStyle31->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleLeft;
			dataGridViewCellStyle31->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle31->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle31->ForeColor = System::Drawing::SystemColors::ControlText;
			dataGridViewCellStyle31->SelectionBackColor = System::Drawing::Color::SkyBlue;
			dataGridViewCellStyle31->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle31->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->filesProgressGridView->DefaultCellStyle = dataGridViewCellStyle31;
			this->filesProgressGridView->Dock = System::Windows::Forms::DockStyle::Fill;
			this->filesProgressGridView->GridColor = System::Drawing::Color::LightGray;
			this->filesProgressGridView->Location = System::Drawing::Point(3, 42);
			this->filesProgressGridView->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->filesProgressGridView->MultiSelect = false;
			this->filesProgressGridView->Name = L"filesProgressGridView";
			dataGridViewCellStyle32->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle32->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle32->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(134)));
			dataGridViewCellStyle32->ForeColor = System::Drawing::SystemColors::WindowText;
			dataGridViewCellStyle32->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle32->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle32->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->filesProgressGridView->RowHeadersDefaultCellStyle = dataGridViewCellStyle32;
			this->filesProgressGridView->RowHeadersVisible = false;
			this->filesProgressGridView->RowHeadersWidth = 51;
			this->filesProgressGridView->RowTemplate->Height = 24;
			this->filesProgressGridView->SelectionMode = System::Windows::Forms::DataGridViewSelectionMode::FullRowSelect;
			this->filesProgressGridView->Size = System::Drawing::Size(1330, 171);
			this->filesProgressGridView->TabIndex = 3;
			this->filesProgressGridView->CellClick += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &MainForm::filesProgressGridView_CellClick);
			this->filesProgressGridView->CellMouseUp += gcnew System::Windows::Forms::DataGridViewCellMouseEventHandler(this, &MainForm::filesProgressGridView_CellMouseUp);
			this->filesProgressGridView->CellValueChanged += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &MainForm::filesProgressGridView_CellValueChanged);
			this->filesProgressGridView->SelectionChanged += gcnew System::EventHandler(this, &MainForm::FilesProgressSelection);
			this->filesProgressGridView->Sorted += gcnew System::EventHandler(this, &MainForm::filesProgressGridView_Sorted);
			// 
			// Index
			// 
			this->Index->HeaderText = L"Index";
			this->Index->MinimumWidth = 6;
			this->Index->Name = L"Index";
			this->Index->Visible = false;
			// 
			// Selected
			// 
			this->Selected->AutoSizeMode = System::Windows::Forms::DataGridViewAutoSizeColumnMode::Fill;
			this->Selected->FillWeight = 5;
			this->Selected->HeaderText = L"";
			this->Selected->MinimumWidth = 6;
			this->Selected->Name = L"Selected";
			this->Selected->SortMode = System::Windows::Forms::DataGridViewColumnSortMode::Automatic;
			// 
			// dataGridViewTextBoxColumn5
			// 
			this->dataGridViewTextBoxColumn5->AutoSizeMode = System::Windows::Forms::DataGridViewAutoSizeColumnMode::Fill;
			this->dataGridViewTextBoxColumn5->HeaderText = L"Filename";
			this->dataGridViewTextBoxColumn5->MinimumWidth = 150;
			this->dataGridViewTextBoxColumn5->Name = L"dataGridViewTextBoxColumn5";
			this->dataGridViewTextBoxColumn5->ReadOnly = true;
			// 
			// Progress
			// 
			dataGridViewCellStyle26->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Progress->DefaultCellStyle = dataGridViewCellStyle26;
			this->Progress->FillWeight = 20;
			this->Progress->HeaderText = L"Progress";
			this->Progress->MinimumWidth = 50;
			this->Progress->Name = L"Progress";
			this->Progress->ReadOnly = true;
			// 
			// Size
			// 
			dataGridViewCellStyle27->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Size->DefaultCellStyle = dataGridViewCellStyle27;
			this->Size->FillWeight = 20;
			this->Size->HeaderText = L"Size";
			this->Size->MinimumWidth = 80;
			this->Size->Name = L"Size";
			this->Size->ReadOnly = true;
			// 
			// SizeBytes
			// 
			this->SizeBytes->HeaderText = L"SizeBytes";
			this->SizeBytes->MinimumWidth = 6;
			this->SizeBytes->Name = L"SizeBytes";
			this->SizeBytes->Visible = false;
			// 
			// PiecesCount
			// 
			dataGridViewCellStyle28->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->PiecesCount->DefaultCellStyle = dataGridViewCellStyle28;
			this->PiecesCount->FillWeight = 10;
			this->PiecesCount->HeaderText = L"Pieces";
			this->PiecesCount->MinimumWidth = 50;
			this->PiecesCount->Name = L"PiecesCount";
			this->PiecesCount->ReadOnly = true;
			// 
			// PiecesRemaining
			// 
			dataGridViewCellStyle29->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->PiecesRemaining->DefaultCellStyle = dataGridViewCellStyle29;
			this->PiecesRemaining->FillWeight = 10;
			this->PiecesRemaining->HeaderText = L"Remaining";
			this->PiecesRemaining->MinimumWidth = 50;
			this->PiecesRemaining->Name = L"PiecesRemaining";
			this->PiecesRemaining->ReadOnly = true;
			// 
			// PiecesActive
			// 
			dataGridViewCellStyle30->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->PiecesActive->DefaultCellStyle = dataGridViewCellStyle30;
			this->PiecesActive->FillWeight = 10;
			this->PiecesActive->HeaderText = L"Active";
			this->PiecesActive->MinimumWidth = 50;
			this->PiecesActive->Name = L"PiecesActive";
			this->PiecesActive->ReadOnly = true;
			// 
			// pieceChart
			// 
			this->pieceChart->BorderlineWidth = 0;
			chartArea2->AxisX->Crossing = 1.7976931348623157E+308;
			chartArea2->AxisX->Enabled = System::Windows::Forms::DataVisualization::Charting::AxisEnabled::True;
			chartArea2->AxisX->InterlacedColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(255)),
				static_cast<System::Int32>(static_cast<System::Byte>(192)), static_cast<System::Int32>(static_cast<System::Byte>(255)));
			chartArea2->AxisX->Interval = 10;
			chartArea2->AxisX->IntervalAutoMode = System::Windows::Forms::DataVisualization::Charting::IntervalAutoMode::VariableCount;
			chartArea2->AxisX->IntervalOffsetType = System::Windows::Forms::DataVisualization::Charting::DateTimeIntervalType::Number;
			chartArea2->AxisX->IsMarginVisible = false;
			chartArea2->AxisX->LabelStyle->Enabled = false;
			chartArea2->AxisX->Minimum = 0;
			chartArea2->AxisY->Crossing = 1.7976931348623157E+308;
			chartArea2->AxisY->Enabled = System::Windows::Forms::DataVisualization::Charting::AxisEnabled::False;
			chartArea2->AxisY->Interval = 1;
			chartArea2->AxisY->IsMarginVisible = false;
			chartArea2->AxisY->Maximum = 1;
			chartArea2->AxisY->MaximumAutoSize = 1;
			chartArea2->AxisY->Minimum = 0;
			chartArea2->AxisY->TitleAlignment = System::Drawing::StringAlignment::Far;
			chartArea2->BackColor = System::Drawing::Color::Silver;
			chartArea2->BorderWidth = 0;
			chartArea2->IsSameFontSizeForAllAxes = true;
			chartArea2->Name = L"ChartArea1";
			chartArea2->Position->Auto = false;
			chartArea2->Position->Height = 94;
			chartArea2->Position->Width = 90;
			chartArea2->Position->X = 1;
			chartArea2->Position->Y = 1;
			this->pieceChart->ChartAreas->Add(chartArea2);
			this->pieceChart->Dock = System::Windows::Forms::DockStyle::Top;
			this->pieceChart->Location = System::Drawing::Point(3, 2);
			this->pieceChart->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
			this->pieceChart->Name = L"pieceChart";
			this->pieceChart->Palette = System::Windows::Forms::DataVisualization::Charting::ChartColorPalette::Fire;
			series3->ChartArea = L"ChartArea1";
			series3->Color = System::Drawing::Color::RoyalBlue;
			series3->CustomProperties = L"PointWidth=2";
			series3->Name = L"HasSeries";
			series3->Points->Add(dataPoint1);
			series3->Points->Add(dataPoint2);
			series4->ChartArea = L"ChartArea1";
			series4->Color = System::Drawing::Color::DarkOrange;
			series4->CustomProperties = L"EmptyPointValue=Zero, PointWidth=2";
			series4->Name = L"Request";
			this->pieceChart->Series->Add(series3);
			this->pieceChart->Series->Add(series4);
			this->pieceChart->Size = System::Drawing::Size(1330, 40);
			this->pieceChart->TabIndex = 0;
			this->pieceChart->Text = L"pieceChart";
			title1->Alignment = System::Drawing::ContentAlignment::MiddleRight;
			title1->Docking = System::Windows::Forms::DataVisualization::Charting::Docking::Right;
			title1->Name = L"Title";
			title1->Position->Auto = false;
			title1->Position->Height = 100;
			title1->Position->X = 99;
			title1->Text = L"10000/10000";
			title1->TextOrientation = System::Windows::Forms::DataVisualization::Charting::TextOrientation::Horizontal;
			this->pieceChart->Titles->Add(title1);
			// 
			// trayIcon
			// 
			this->trayIcon->Text = L"mtTorrent";
			this->trayIcon->Visible = true;
			this->trayIcon->MouseDoubleClick += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::trayIcon_MouseDoubleClick);
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
			this->TorrentName->AutoSizeMode = System::Windows::Forms::DataGridViewAutoSizeColumnMode::Fill;
			this->TorrentName->FillWeight = 110;
			this->TorrentName->HeaderText = L"Name";
			this->TorrentName->MinimumWidth = 350;
			this->TorrentName->Name = L"TorrentName";
			this->TorrentName->ReadOnly = true;
			// 
			// TorrentProgress
			// 
			dataGridViewCellStyle2->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentProgress->DefaultCellStyle = dataGridViewCellStyle2;
			this->TorrentProgress->FillWeight = 15;
			this->TorrentProgress->HeaderText = L"Progress";
			this->TorrentProgress->MinimumWidth = 100;
			this->TorrentProgress->Name = L"TorrentProgress";
			this->TorrentProgress->ReadOnly = true;
			this->TorrentProgress->Width = 125;
			// 
			// Column4
			// 
			dataGridViewCellStyle3->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Column4->DefaultCellStyle = dataGridViewCellStyle3;
			this->Column4->FillWeight = 15;
			this->Column4->HeaderText = L"State";
			this->Column4->MinimumWidth = 50;
			this->Column4->Name = L"Column4";
			this->Column4->ReadOnly = true;
			this->Column4->Width = 70;
			// 
			// DL
			// 
			dataGridViewCellStyle4->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->DL->DefaultCellStyle = dataGridViewCellStyle4;
			this->DL->FillWeight = 30;
			this->DL->HeaderText = L"Download";
			this->DL->MinimumWidth = 80;
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
			dataGridViewCellStyle5->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->UP->DefaultCellStyle = dataGridViewCellStyle5;
			this->UP->FillWeight = 15;
			this->UP->HeaderText = L"Upload";
			this->UP->MinimumWidth = 50;
			this->UP->Name = L"UP";
			this->UP->ReadOnly = true;
			this->UP->Width = 90;
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
			dataGridViewCellStyle6->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentConnected->DefaultCellStyle = dataGridViewCellStyle6;
			this->TorrentConnected->FillWeight = 10;
			this->TorrentConnected->HeaderText = L"Connections";
			this->TorrentConnected->MinimumWidth = 40;
			this->TorrentConnected->Name = L"TorrentConnected";
			this->TorrentConnected->ReadOnly = true;
			this->TorrentConnected->Width = 65;
			// 
			// TorrentFoundPeers
			// 
			dataGridViewCellStyle7->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentFoundPeers->DefaultCellStyle = dataGridViewCellStyle7;
			this->TorrentFoundPeers->FillWeight = 5;
			this->TorrentFoundPeers->HeaderText = L"Peers";
			this->TorrentFoundPeers->MinimumWidth = 40;
			this->TorrentFoundPeers->Name = L"TorrentFoundPeers";
			this->TorrentFoundPeers->ReadOnly = true;
			this->TorrentFoundPeers->Width = 55;
			// 
			// TorrentDownloaded
			// 
			dataGridViewCellStyle8->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->TorrentDownloaded->DefaultCellStyle = dataGridViewCellStyle8;
			this->TorrentDownloaded->FillWeight = 8;
			this->TorrentDownloaded->HeaderText = L"Downloaded";
			this->TorrentDownloaded->MinimumWidth = 30;
			this->TorrentDownloaded->Name = L"TorrentDownloaded";
			this->TorrentDownloaded->ReadOnly = true;
			this->TorrentDownloaded->Width = 65;
			// 
			// Column3
			// 
			dataGridViewCellStyle9->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->Column3->DefaultCellStyle = dataGridViewCellStyle9;
			this->Column3->FillWeight = 8;
			this->Column3->HeaderText = L"Uploaded";
			this->Column3->MinimumWidth = 30;
			this->Column3->Name = L"Column3";
			this->Column3->ReadOnly = true;
			this->Column3->Width = 60;
			// 
			// MainForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1377, 525);
			this->Controls->Add(this->splitContainer1);
			this->Margin = System::Windows::Forms::Padding(3, 2, 3, 2);
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
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->filesProgressGridView))->EndInit();
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

private: System::Void  filesProgressGridView_SortCompare(System::Object^ sender, DataGridViewSortCompareEventArgs^ e)
{
	if (e->Column->Name == "Size")
	{
		auto bytesID = gcnew String("SizeBytes");
		auto v1 = Int32::Parse(filesProgressGridView->Rows[e->RowIndex1]->Cells[bytesID]->Value->ToString());
		auto v2 = Int32::Parse(filesProgressGridView->Rows[e->RowIndex2]->Cells[bytesID]->Value->ToString());
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
		//Hide();
	}
}
private: System::Void FilesProgressSelection(System::Object^ sender, System::EventArgs^ e) {
	
	static bool firstIngnored = false;

	if (!firstIngnored)
	{
		firstIngnored = true;
		filesProgressGridView->ClearSelection();
	}
}
private: System::Void filesProgressGridView_CellValueChanged(System::Object^ sender, System::Windows::Forms::DataGridViewCellEventArgs^ e) {
	if (e->ColumnIndex == Selected->Index && e->RowIndex != -1)
	{
		fileProgressSelectionChanged(e->RowIndex);
	}
}
private: System::Void filesProgressGridView_CellMouseUp(System::Object^ sender, System::Windows::Forms::DataGridViewCellMouseEventArgs^ e) {
	if (e->ColumnIndex == Selected->Index && e->RowIndex != -1)
	{
		filesProgressGridView->EndEdit();
	}
}
private: System::Void filesProgressGridView_CellClick(System::Object^ sender, System::Windows::Forms::DataGridViewCellEventArgs^ e) {
	if (e->RowIndex != -1)
		forceGuiRefresh();
}
private: System::Void filesProgressGridView_TabClick(System::Object^ sender, System::Windows::Forms::TabControlCancelEventArgs^ e) {
	forceGuiRefresh();
}
private: System::Void  filesProgressGridView_Sorted(System::Object^ sender, System::EventArgs^ e)
{
	filesProgressGridView->ClearSelection();
}
private: System::Void  torrentsGridView_Sorted(System::Object^ sender, System::EventArgs^ e)
{
	torrentsGridSorted();
}
};
}
