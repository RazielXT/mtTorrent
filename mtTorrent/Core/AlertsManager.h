#pragma once
#include "Interface.h"

namespace mtt
{
	class AlertsManager
	{
	public:

		static AlertsManager& Get();
		void torrentAlert(AlertId id, Torrent* torrent);
		void metadataAlert(AlertId id, Torrent* torrent);
		void configAlert(AlertId id, config::ValueType type);

		void registerAlerts(uint32_t alertMask);
		std::vector<std::unique_ptr<mtt::AlertMessage>> popAlerts();

	private:

		bool isAlertRegistered(AlertId);

		std::mutex alertsMutex;
		std::vector<std::unique_ptr<mtt::AlertMessage>> alerts;

		uint32_t alertsMask = 0;
	};
}