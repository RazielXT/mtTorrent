#include "State.h"
#include <fstream>
#include "json\picojson.h"

void mtt::TorrentsState::saveState()
{
	std::ofstream file("mtt.state");

	if (!file)
		return;

	picojson::object jsState;

	picojson::array jsTorrentsState;
	for (auto t : torrents)
	{
		picojson::object tstate;
		tstate["torrentFile"] = picojson::value(t.torrentFilePath);
		tstate["downloadPath"] = picojson::value(t.downloadPath);

		jsTorrentsState.push_back(picojson::value(tstate));
	}
	jsState["torrents"] = picojson::value(jsTorrentsState);

	auto out = picojson::value(jsState).serialize();

	file << out;
}

void mtt::TorrentsState::loadState()
{
	torrents.clear();

	std::ifstream file("mtt.state");

	if (!file)
		return;

	std::string data((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	picojson::value jsState;
	auto res = picojson::parse(jsState, data);

	if (res.empty() && jsState.is<picojson::object>())
	{
		auto jsTorrents = jsState.get("torrents");

		if (jsTorrents.is<picojson::array>())
		{
			auto torrentsArray = jsTorrents.get<picojson::array>();
			for (auto& t : torrentsArray)
			{
				TorrentState state;
				auto props = t.get<picojson::object>();

				state.torrentFilePath = props["torrentFile"].to_str();
				state.downloadPath = props["downloadPath"].to_str();

				torrents.push_back(state);
			}
		}
	}
}

