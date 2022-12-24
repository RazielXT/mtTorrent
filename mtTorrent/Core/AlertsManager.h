#pragma once
#include "Interface.h"

namespace mtt
{
	class AlertsManager
	{
	public:

		static AlertsManager& Get();
		void torrentAlert(Alerts::Id id, Torrent* torrent);
		void metadataAlert(Alerts::Id id, Torrent* torrent);
		void configAlert(Alerts::Id id, config::ValueType type);

		void registerAlerts(uint64_t alertMask);
		std::vector<std::unique_ptr<mtt::AlertMessage>> popAlerts();

	private:

		bool isAlertRegistered(Alerts::Id);

		std::mutex alertsMutex;
		std::vector<std::unique_ptr<mtt::AlertMessage>> alerts;

		uint64_t alertsMask = 0;
	};
}