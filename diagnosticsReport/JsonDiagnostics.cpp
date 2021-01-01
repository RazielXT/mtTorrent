#include "JsonDiagnostics.h"
#include "Json.hpp"

std::string JsonDiagnostics::generateJson()
{
	auto peers = loadPeers();

	auto jsPeers = json::Array();

	for (auto& peer : peers)
	{
		json::JSON peerJs;
		peerJs["name"] = peer.name;

		auto jsEvents = json::Array();

		for (auto& event : peer.events)
		{
			json::JSON obj;
			obj["t"] = (int)event.type;

			if (event.type == mtt::Diagnostics::PeerEventType::ReceiveMessage)
			{
				obj["i"] = event.idx;

				if (event.idx == 4)
					obj["h"] = event.subIdx;
			}
			else if (event.type == mtt::Diagnostics::PeerEventType::Piece)
			{
				obj["i"] = event.idx;
				obj["b"] = event.subIdx;
				//obj["s"] = event.sz;
			}
			else if (event.type == mtt::Diagnostics::PeerEventType::RequestPiece)
			{
				obj["i"] = event.idx;
				obj["b"] = event.subIdx;
				//obj["s"] = event.sz;
			}

			obj["tm"] = (uint32_t)event.time;
			jsEvents.append(std::move(obj));
		}
		peerJs["events"] = std::move(jsEvents);

		jsPeers.append(std::move(peerJs));
	}
	
	json::JSON obj;
	obj["peers"] = std::move(jsPeers);
	obj["timeStart"] = timestart;

	return obj.dump(1,"","");
}
