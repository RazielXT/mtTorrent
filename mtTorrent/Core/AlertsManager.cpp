#include "AlertsManager.h"
#include "Torrent.h"

mtt::AlertsManager* ptr = nullptr;

void mtt::AlertsManager::torrentAlert(AlertId id, const uint8_t* hash)
{
	if (!isAlertRegistered(id))
		return;

	auto alert = std::make_unique<TorrentAlert>();
	alert->id = id;
	memcpy(alert->hash, hash, 20);

	std::lock_guard<std::mutex> guard(alertsMutex);
	alerts.push_back(std::move(alert));
}

void mtt::AlertsManager::metadataAlert(AlertId id, const uint8_t* hash)
{
	if (!isAlertRegistered(id))
		return;

	auto alert = std::make_unique<MetadataAlert>();
	alert->id = id;
	memcpy(alert->hash, hash, 20);

	std::lock_guard<std::mutex> guard(alertsMutex);
	alerts.push_back(std::move(alert));
}

void mtt::AlertsManager::configAlert(AlertId id, config::ValueType type)
{
	if (!isAlertRegistered(id))
		return;

	auto alert = std::make_unique<ConfigAlert>();
	alert->id = id;
	alert->configType = type;

	std::lock_guard<std::mutex> guard(alertsMutex);
	alerts.push_back(std::move(alert));
}


mtt::AlertsManager::AlertsManager()
{
	ptr = this;
}

mtt::AlertsManager& mtt::AlertsManager::Get()
{
	return *ptr;
}

void mtt::AlertsManager::registerAlerts(uint32_t m)
{
	alertsMask = m;
}

std::vector<std::unique_ptr<mtt::AlertMessage>> mtt::AlertsManager::popAlerts()
{
	std::lock_guard<std::mutex> guard(alertsMutex);
	auto v = std::move(alerts);
	alerts.clear();

	return v;
}

bool mtt::AlertsManager::isAlertRegistered(AlertId id)
{
	return (alertsMask & (int)id);
}

