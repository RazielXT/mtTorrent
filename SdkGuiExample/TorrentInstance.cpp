#include "TorrentInstance.h"
#include "imgui.h"
#include "mtTorrent/Api/Configuration.h"
#include <sstream>
#include <chrono>
#include <tuple>

TorrentInstance::TorrentInstance()
{
	auto settings = mtt::config::getExternal();
	settings.files.defaultDirectory = "./";
	mtt::config::setValues(settings.files);
	settings.dht.enabled = true;
	mtt::config::setValues(settings.dht);

	core = mttApi::Core::create();

	auto torrents = core->getTorrents();

	if (!torrents.empty())
		torrentPtr = torrents.front();
}

void TorrentInstance::draw()
{
	drawInfoWindow();

	if (torrentPtr)
	{
		drawSourcesWindow();
		drawPeersWindow();
	}
}

static std::string formatBytes(uint64_t bytes)
{
	const char* type = " MB";

	auto sz = ((bytes / 1024.f) / 1024.f);
	if (sz < 1)
	{
		sz = (float)(bytes / (1024ll));
		type = " KB";

		if (sz == 0)
		{
			sz = (float)bytes;
			type = " B";
		}
	}
	else if (sz > 1024)
	{
		sz /= 1024.f;
		type = " GB";
	}

	std::string str(50, '\0');
	snprintf(&str[0], 50, "%.2f", sz);
	str.resize(strlen(str.data()));

	while (str.back() == '0')
		str.pop_back();

	if (str.back() == '.')
		str.pop_back();

	return str + type;
}

static std::string formatBytesSpeed(uint64_t bytes)
{
	return formatBytes(bytes) + "/s";
}

void TorrentInstance::drawInfoWindow()
{
	ImGui::SetNextWindowPos({ 8, 8 }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({ 690, 200 }, ImGuiCond_FirstUseEver);
	ImGui::Begin("Torrent info");

	if (!torrentPtr)
	{
		static mtt::Status status = mtt::Status::Success;
		static char input[1024] = "";

		ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
		ImGui::InputText("Input", input, IM_ARRAYSIZE(input));

		if (ImGui::Button("Add Magnet"))
			std::tie(status, torrentPtr) = core->addMagnet(input);

		ImGui::SameLine();

		if (ImGui::Button("Add Torrent File"))
			std::tie(status, torrentPtr) = core->addFile(input);

		if (status != mtt::Status::Success)
			ImGui::Text("Adding error %d", (int)status);

		ImGui::End();

		return;
	}

	if (torrentPtr->getActiveState() == mttApi::Torrent::ActiveState::Started)
	{
		if (ImGui::Button("Stop"))
			torrentPtr->stop();
	}
	else
	{
		if (ImGui::Button("Start"))
			torrentPtr->start();
	}

	ImGui::SameLine();
	if (ImGui::Button("Remove"))
	{
		core->removeTorrent(torrentPtr, true);
		torrentPtr = nullptr;
		ImGui::End();
		return;
	}

	ImGui::Text(torrentPtr->name().data());
	ImGui::Text("Total size %s", formatBytes(torrentPtr->getFileInfo().info.fullSize).data());

	auto state = torrentPtr->getState();

	switch (state)
	{
	case mttApi::Torrent::State::CheckingFiles:
		ImGui::Text("Checking files... %f%%\n", torrentPtr->checkingProgress() * 100);
		break;
	case mttApi::Torrent::State::DownloadingMetadata:
	{
		if (auto magnet = torrentPtr->getMagnetDownload())
		{
			mtt::MetadataDownloadState utmState = magnet->getState();
			if (utmState.partsCount == 0)
				ImGui::Text("Metadata download getting peers... Connected peers: %u, Received peers: %u\n", torrentPtr->getPeers()->connectedCount(), torrentPtr->getPeers()->receivedCount());
			else
				ImGui::Text("Metadata download progress: %u / %u, Connected peers: %u, Received peers: %u\n", utmState.receivedParts, utmState.partsCount, torrentPtr->getPeers()->connectedCount(), torrentPtr->getPeers()->receivedCount());

			std::vector<std::string> logs;
			if (auto count = magnet->getDownloadLog(logs, 0))
			{
				for (auto& l : logs)
					ImGui::Text("Metadata log: %s\n", l.data());
			}
		}
		break;
	}
	case mttApi::Torrent::State::Downloading:
		ImGui::Text("Downloading, Progress %f%%, Speed: %s, Connected peers: %u, Found peers: %u\n", torrentPtr->progress() * 100, formatBytesSpeed(torrentPtr->getFileTransfer()->getDownloadSpeed()).data(), torrentPtr->getPeers()->connectedCount(), torrentPtr->getPeers()->receivedCount());
		break;
	case mttApi::Torrent::State::Seeding:
		ImGui::Text("Finished, upload speed: %s\n", formatBytesSpeed(torrentPtr->getFileTransfer()->getUploadSpeed()).data());
		break;
	case mttApi::Torrent::State::Interrupted:
		ImGui::Text("Interrupted, problem code: %d\n", (int)torrentPtr->getLastError());
		break;
	case mttApi::Torrent::State::Inactive:
	default:
		ImGui::Text("Stopped");
		break;
	}

	ImGui::End();
}

void TorrentInstance::drawPeersWindow()
{
	static ImGuiTableFlags flags = /*ImGuiTableFlags_SizingPolicyFixed | */ ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

	ImGui::SetNextWindowPos({ 110, 210 }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({ 1030, 470 }, ImGuiCond_FirstUseEver);
	ImGui::Begin("Torrent peers");

	auto peerStateToString = [](mtt::ActivePeerInfo state) -> std::string
	{
		if (!state.connected)
			return "Connecting";
		else if (state.choking)
			return "Requesting";
		else
			return "Connected";
	};

	if (ImGui::BeginTable("##PeersTable", 4, flags))
	{
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Download speed", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Client Id", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		auto peers = torrentPtr->getFileTransfer()->getPeersInfo();
		for (size_t row = 0; row < peers.size(); row++)
		{
			const auto& peer = peers[row];

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text(peer.address.data());

			ImGui::TableSetColumnIndex(1);
			ImGui::Text(peerStateToString(peer).data());

			ImGui::TableSetColumnIndex(2);
			ImGui::Text(formatBytesSpeed(peer.downloadSpeed).data());

			ImGui::TableSetColumnIndex(3);
			ImGui::Text(peer.client.data());
		}
		ImGui::EndTable();
	}

	ImGui::End();
}

std::string formatSeconds(uint32_t seconds)
{
	if (seconds == 0 || seconds > 60*60)
		return "";

	std::string time = std::to_string(seconds % 60);

	if (seconds >= 60)
	{
		if (time.size() == 1)
			time = "0" + time;

		seconds /= 60;
		std::string minutes = std::to_string(seconds % 60);
		if (minutes.size() == 1)
			minutes = "0" + minutes;

		time = minutes + ":" + time;
	}

	return time;
}

void TorrentInstance::drawSourcesWindow()
{
	static ImGuiTableFlags flags = /*ImGuiTableFlags_SizingPolicyFixed | */ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

	ImGui::SetNextWindowPos({ 695, 8 }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({ 515, 200 }, ImGuiCond_FirstUseEver);
	ImGui::Begin("Torrent sources");

	auto sourceStateToString = [](mtt::TrackerState state) -> std::string
	{
		if (state == mtt::TrackerState::Connected || state == mtt::TrackerState::Alive)
			return "Ready";
		else if (state == mtt::TrackerState::Connecting)
			return "Connecting";
		else if (state == mtt::TrackerState::Announcing || state == mtt::TrackerState::Reannouncing)
			return "Announcing";
		else if (state == mtt::TrackerState::Announced)
			return "Announced";
		else if (state == mtt::TrackerState::Offline)
			return "Offline";
		else
			return "Stopped";
	};

	if (ImGui::BeginTable("##SourcesTable", 5, flags))
	{
		ImGui::TableSetupColumn("Hostname", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Received", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Found", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Next check", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		uint32_t currentTime = (uint32_t)time(0);
		auto sources = torrentPtr->getPeers()->getSourcesInfo();
		for (size_t row = 0; row < sources.size(); row++)
		{
			const auto& source = sources[row];

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text(source.hostname.data());

			ImGui::TableSetColumnIndex(1);
			ImGui::Text(sourceStateToString(source.state).data());

			ImGui::TableSetColumnIndex(2);
			ImGui::Text(std::to_string(source.peers).data());

			ImGui::TableSetColumnIndex(3);
			ImGui::Text(std::to_string(source.seeds).data());

			auto nextCheck = source.nextAnnounce < currentTime ? 0 : source.nextAnnounce - currentTime;
			ImGui::TableSetColumnIndex(4);
			ImGui::Text(formatSeconds(nextCheck).data());
		}
		ImGui::EndTable();
	}

	ImGui::End();
}
