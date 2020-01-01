#pragma once
#include "Interface.h"

namespace mtt
{
	class AlertsManager
	{
	public:

		AlertsManager();

		static AlertsManager& Get();
		void torrentAlert(AlertId id, const uint8_t* hash);
		void metadataAlert(AlertId id, TorrentPtr t);

		void registerAlerts(uint32_t alertMask);
		std::vector<std::unique_ptr<mtt::AlertMessage>> popAlerts();

	private:

		bool isAlertRegistered(AlertId);

		std::mutex alertsMutex;
		std::vector<std::unique_ptr<mtt::AlertMessage>> alerts;

		uint32_t alertsMask = 0;
	};
}