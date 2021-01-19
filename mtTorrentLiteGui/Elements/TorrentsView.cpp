#include "TorrentsView.h"
#include "MainForm.h"
#include "../AppCore.h"
#include "../Utils/Utils.h"
#include "../../mtTorrent/utils/HexEncoding.h"
#include <map>

TorrentsView::TorrentsView(AppCore& c) : core(c), piecesProgress(c)
{

}

void TorrentsView::update()
{
	refreshTorrentsGrid();

	refreshSelection();

	if (core.selectionChanged)
		refreshTorrentInfo(core.firstSelectedHash);

	auto activeTab = GuiLite::MainForm::instance->getActiveTab();

	if (activeTab == GuiLite::MainForm::TabType::Peers)
		refreshPeers();

	if (activeTab == GuiLite::MainForm::TabType::Sources)
		refreshSources();

	if (activeTab == GuiLite::MainForm::TabType::Progress)
		piecesProgress.update();
}

void TorrentsView::refreshSelection()
{
	core.selected = GuiLite::MainForm::instance->getGrid()->SelectedRows->Count > 0;

	if (!core.selected)
	{
		GuiLite::MainForm::instance->getGrid()->ClearSelection();
		GuiLite::MainForm::instance->torrentInfoLabel->Clear();
		GuiLite::MainForm::instance->dlSpeedChart->Visible = false;
		memset(core.firstSelectedHash, 0, 20);
	}
	else
	{
		auto idStr = (System::String^)GuiLite::MainForm::instance->getGrid()->SelectedRows[0]->Cells[0]->Value;
		auto stdStr = getUtf8String(idStr);
		uint8_t selectedHash[20];
		decodeHexa(stdStr, selectedHash);

		if (memcmp(core.firstSelectedHash, selectedHash, 20) != 0)
		{
			memcpy(core.firstSelectedHash, selectedHash, 20);
			core.selectionChanged = true;
		}
	}
}

void TorrentsView::updateList()
{
	listChanged = true;
}

std::vector<TorrentsView::SelectedTorrent> TorrentsView::getAllSelectedTorrents()
{
	std::vector<SelectedTorrent> out;

	for (int i = 0; i < GuiLite::MainForm::instance->getGrid()->SelectedRows->Count; i++)
	{
		auto idStr = (System::String^)GuiLite::MainForm::instance->getGrid()->SelectedRows[i]->Cells[0]->Value;
		auto stdStr = getUtf8String(idStr);
		SelectedTorrent t;
		decodeHexa(stdStr, t.hash);

		out.push_back(t);
	}

	return out;
}

void TorrentsView::refreshTorrentInfo(uint8_t* hash)
{
	if (!core.IoctlFunc)
		return;

	mtBI::TorrentInfo info;
	core.IoctlFunc(mtBI::MessageId::GetTorrentInfo, hash, &info);

	GuiLite::MainForm::instance->torrentInfoLabel->Clear();
	auto infoLines = GuiLite::MainForm::instance->torrentInfoLabel;

	if (info.name.length)
	{
		infoLines->AppendText(gcnew System::String(info.name.data, 0, (int)info.name.length, System::Text::Encoding::UTF8));
		infoLines->AppendText(System::Environment::NewLine);
		infoLines->AppendText(System::Environment::NewLine);

	}
	if (info.fullsize)
	{
		infoLines->AppendText("Total size: ");
		infoLines->AppendText(formatBytes(info.fullsize));
		infoLines->AppendText(System::Environment::NewLine);
		infoLines->AppendText(System::Environment::NewLine);
	}
	if (info.downloadLocation.length)
	{
		infoLines->AppendText("Save in: \t");
		info.downloadLocation.append(info.name);
		infoLines->AppendText(gcnew System::String(info.downloadLocation.data, 0, (int)info.downloadLocation.length, System::Text::Encoding::UTF8));
		infoLines->AppendText(System::Environment::NewLine);
		infoLines->AppendText(System::Environment::NewLine);
	}

	infoLines->AppendText("Hash: \t");
	auto hashStr = hexToString(hash, 20);
	for (int i = 0; i < 4; i++)
		hashStr.insert(8 + i * 8, 1, ' ');
	infoLines->AppendText(gcnew System::String(hashStr.data()));

	System::String^ creationStr = "";
	if (info.creationDate != 0)
	{
		creationStr = System::DateTimeOffset::FromUnixTimeSeconds(info.creationDate).ToString("MM/dd/yyyy");
		creationStr += " ";
	}

	if (info.createdBy.length != 0)
	{
		creationStr += "by ";
		creationStr += gcnew System::String(info.createdBy.data);
	}

	if (creationStr->Length > 0)
	{
		infoLines->AppendText(System::Environment::NewLine);
		infoLines->AppendText(System::Environment::NewLine);
		infoLines->AppendText("Created: \t");
		infoLines->AppendText(creationStr);
	}

	speedChart.resetChart();
}

void TorrentsView::refreshTorrentsGrid()
{
	mtBI::TorrentsList torrents;
	if (core.IoctlFunc(mtBI::MessageId::GetTorrents, nullptr, &torrents) != mtt::Status::Success)
		return;

	mtBI::TorrentStateInfo info;
	info.connectedPeers = 0;

	bool selectionActive = false;
	bool selectionStopped = false;

	auto torrentGrid = GuiLite::MainForm::instance->getGrid();

	auto hashToInt = [](const std::string& hash)
	{
		int i = 1;
		for (int j = 0; j < 20; j++)
			i += ((uint8_t)hash[j]) * i;
		return i;
	};

	auto hashToInt2 = []( System::String^ hash)
	{
		int i = 1;
		for (int j = 0; j < 20; j++)
			i += ((uint8_t)hash[j]) * i;
		return i;
	};

	if (listChanged || torrentRows.size() != torrentGrid->Rows->Count)
	{
		torrentRows.clear();

		for (int i = 0; i < torrentGrid->Rows->Count;)
		{
			bool found = false;
			auto rowHashId = hashToInt2(torrentGrid->Rows[i]->Cells[0]->Value->ToString());

			for (auto& t : torrents.list)
			{
				auto hashId = hashToInt(hexToString(t.hash, 20));

				if (hashId == rowHashId)
				{
					torrentRows[hashId] = i;
					found = true;
					break;
				}
			}
			
			if (found)
				i++;
			else
				torrentGrid->Rows->RemoveAt(i);
		}

		listChanged = false;
	}

	for (size_t i = 0; i < torrents.list.size(); i++)
	{
		auto& t = torrents.list[i];

		int rowId = 0;
		auto hashStr = hexToString(t.hash, 20);
		{
			auto hashStr = hexToString(t.hash, 20);
			int hashId = hashToInt(hashStr);

			auto existingRowId = torrentRows.find(hashId);
			if (existingRowId != torrentRows.end())
			{
				rowId = existingRowId->second;
			}
			else
			{
				rowId = (int)i;
				torrentGrid->Rows->Insert(rowId);

				for (auto& t : torrentRows)
					if (t.second >= rowId)
						torrentRows[t.first] = t.second + 1;

				torrentRows[hashId] = rowId;
			}

			if (torrentGrid->Rows[rowId]->Selected)
			{
				if (t.active)
					selectionActive = true;

				if (!t.active)
					selectionStopped = true;
			}

			//no need to update stopped info state
			if (!t.active && !torrentState[hashId].active)
				continue;

			torrentState[hashId] = { t.active };
		}

		if (core.IoctlFunc(mtBI::MessageId::GetTorrentStateInfo, t.hash, &info) == mtt::Status::Success)
		{
			bool isSelected = (memcmp(t.hash, core.firstSelectedHash, 20) == 0);

			System::String^ activeStatus;
			if (info.activeStatus != mtt::Status::Success)
			{
				if (info.activeStatus == mtt::Status::E_NotEnoughSpace)
					activeStatus = "Not enough space";
				else if (info.activeStatus == mtt::Status::E_InvalidPath)
					activeStatus = "Invalid path";
				else
					activeStatus = "Problem " + int(info.activeStatus).ToString();
			}
			else if (!t.active)
			{
				int schedule = core.scheduler.getSchedule(t.hash);

				if (schedule <= 0)
					activeStatus = "Stopped";
				else
				{
					activeStatus = "Scheduled (";
					System::TimeSpan time = System::TimeSpan::FromSeconds((double)schedule);
					activeStatus += time.ToString("d\\d\\ hh\\hmm\\mss\\s")->TrimStart(' ', 'd', 'h', 'm', 's', '0');
					activeStatus += ")";
				}
			}
			else if (info.stopping)
				activeStatus = "Stopping";
			else
				activeStatus = "Active";

			//append speed chart with new values
			if (isSelected && t.active)
				speedChart.update((info.downloadSpeed / 1024.f) / 1024.f, (info.uploadSpeed / 1024.f) / 1024.f);

			System::String^ speedInfo = "";
			if (t.active && info.downloadSpeed > 0 && info.selectionProgress && info.selectionProgress < 1.0f && info.downloaded > 0)
			{
				speedInfo = formatBytesSpeed(info.downloadSpeed);

				uint64_t leftBytes = ((uint64_t)(info.downloaded / info.selectionProgress)) - info.downloaded;
				uint64_t leftSeconds = leftBytes / info.downloadSpeed;

				//estimate time to finish
				if (leftSeconds > 0 && leftSeconds < long::MaxValue)
				{
					System::TimeSpan time = System::TimeSpan::FromSeconds((double)leftSeconds);
					speedInfo += " (";
					speedInfo += time.ToString("d\\d\\ hh\\hmm\\mss\\s")->TrimStart(' ', 'd', 'h', 'm', 's', '0');
					speedInfo += ")";
				}
			}

			System::String^ name = gcnew System::String(info.name.data, 0, (int)info.name.length, System::Text::Encoding::UTF8);
			if (info.utmActive)
			{
				mtBI::MagnetLinkProgress magnetProgress;
				if (core.IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, t.hash, &magnetProgress) == mtt::Status::Success)
				{
					if (magnetProgress.progress > 1.0f)
						magnetProgress.progress = 1.0f;

					name = "Metadata download " + float(magnetProgress.progress).ToString("P");
				}
			}

			System::String^ progress;
			if (info.checking)
				progress = "Checking " + float(info.checkingProgress).ToString("P");
			else
			{
				progress = float(info.selectionProgress).ToString("P");

				if (info.progress != info.selectionProgress)
					progress += " (" + float(info.progress).ToString("P") + ")";
			}

			//torrent row - visual/logic data columns
			auto row = gcnew cli::array< System::String^  >(12) {
				gcnew System::String(hashStr.data()),
					name, progress, activeStatus,
					speedInfo, int(info.downloadSpeed).ToString(),
					info.uploadSpeed ? formatBytesSpeed(info.uploadSpeed) : "", int(info.uploadSpeed).ToString(),
					(t.active || info.connectedPeers) ? int(info.connectedPeers).ToString() : "",
					(t.active || info.foundPeers) ? int(info.foundPeers).ToString() : "",
					formatBytes(info.downloaded), formatBytes(info.uploaded)
			};

			torrentGrid->Rows[rowId]->SetValues(row);

			if (isSelected)
			{
				if (lastInfoIncomplete && !info.utmActive)
					core.selectionChanged = true;

				lastInfoIncomplete = info.utmActive;
			}
		}
	}

	GuiLite::MainForm::instance->buttonStart->Enabled = selectionStopped;
	GuiLite::MainForm::instance->buttonStop->Enabled = selectionActive;
	GuiLite::MainForm::instance->buttonRemove->Enabled = core.selected;
}

void TorrentsView::refreshPeers()
{
	mtBI::TorrentPeersInfo peersInfo;
	core.IoctlFunc(mtBI::MessageId::GetPeersInfo, core.firstSelectedHash, &peersInfo);

	auto peersGrid = GuiLite::MainForm::instance->getPeersGrid();
	System::String^ selectedPeer;
	if (peersGrid->SelectedRows->Count > 0)
		selectedPeer = (System::String^)peersGrid->SelectedRows[0]->Cells[0]->Value;
	adjustGridRowsCount(peersGrid, (int)peersInfo.peers.size());

	for (uint32_t i = 0; i < peersInfo.peers.size(); i++)
	{
		auto& peerInfo = peersInfo.peers[i];
		auto peerRow = gcnew cli::array< System::String^  >(8) {
			gcnew System::String(peerInfo.addr.data),
				!peerInfo.connected ? "Connecting" : ((peerInfo.dlSpeed == 0 && peerInfo.choking) ? "Requesting" : formatBytesSpeed(peerInfo.dlSpeed)), int(peerInfo.dlSpeed).ToString(),
				formatBytesSpeed(peerInfo.upSpeed), int(peerInfo.upSpeed).ToString(),
				float(peerInfo.progress).ToString("P"), gcnew System::String(peerInfo.client.data, 0, (int)peerInfo.client.length, System::Text::Encoding::UTF8),
				gcnew System::String(peerInfo.country.data)
		};

		peersGrid->Rows[i]->SetValues(peerRow);
	}

	if (peersGrid->SortedColumn)
		peersGrid->Sort(peersGrid->SortedColumn, peersGrid->SortOrder == System::Windows::Forms::SortOrder::Ascending ? System::ComponentModel::ListSortDirection::Ascending : System::ComponentModel::ListSortDirection::Descending);

	if (peersGrid->RowCount > 0 && selectedPeer)
	{
		peersGrid->ClearSelection();
		for (uint32_t i = 0; i < peersInfo.peers.size(); i++)
		{
			if (selectedPeer == (System::String^)peersGrid->Rows[i]->Cells[0]->Value)
				peersGrid->Rows[i]->Selected = true;
		}
	}
}

void TorrentsView::refreshSources()
{
	mtBI::SourcesInfo sourcesInfo;
	core.IoctlFunc(mtBI::MessageId::GetSourcesInfo, core.firstSelectedHash, &sourcesInfo);

	auto sourcesGrid = GuiLite::MainForm::instance->sourcesGrid;
	adjustGridRowsCount(sourcesGrid, (int)sourcesInfo.sources.size());

	for (size_t i = 0; i < sourcesInfo.sources.size(); i++)
	{
		auto& source = sourcesInfo.sources[i];
		auto nextCheck = (source.nextCheck == 0) ? gcnew System::String("") : System::TimeSpan::FromSeconds(source.nextCheck).ToString("T");
		char status[11];

		if (source.status == mtBI::SourceInfo::Ready)
			memcpy(status, "Ready", 6);
		else if (source.status == mtBI::SourceInfo::Connecting)
			memcpy(status, "Connecting", 11);
		else if (source.status == mtBI::SourceInfo::Announcing)
			memcpy(status, "Announcing", 11);
		else if (source.status == mtBI::SourceInfo::Announced)
			memcpy(status, "Announced", 10);
		else if (source.status == mtBI::SourceInfo::Offline)
			memcpy(status, "Offline", 8);
		else
			memcpy(status, "Stopped", 8);

		auto sourceRow = gcnew cli::array< System::String^  >(7) {
			gcnew System::String(source.name.data), gcnew System::String(status),
				int(source.peers).ToString(), int(source.seeds).ToString(), int(source.leechers).ToString(), nextCheck, int(source.interval).ToString()
		};

		//clear unnecessary 0 values based on state
		if (source.status == mtBI::SourceInfo::Stopped)
		{
			sourceRow[2] = "";
			sourceRow[3] = "";
			sourceRow[4] = "";
			sourceRow[6] = "";
		}
		else if (source.status != mtBI::SourceInfo::Announced)
		{
			if (source.peers == 0)
				sourceRow[2] = "";

			sourceRow[3] = "";
			sourceRow[4] = "";
		}

		sourcesGrid->Rows[i]->SetValues(sourceRow);
	}

	if (sourcesGrid->SortedColumn)
		sourcesGrid->Sort(sourcesGrid->SortedColumn, sourcesGrid->SortOrder == System::Windows::Forms::SortOrder::Ascending ? System::ComponentModel::ListSortDirection::Ascending : System::ComponentModel::ListSortDirection::Descending);
}
