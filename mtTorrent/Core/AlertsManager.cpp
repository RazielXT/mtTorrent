#include "AlertsManager.h"
#include "Torrent.h"

mtt::AlertsManager instance;

void mtt::AlertsManager::torrentAlert(Alerts::Id id, Torrent* torrent)
{
	if (!isAlertRegistered(id))
		return;

	auto alert = std::make_unique<TorrentAlert>();
	alert->id = id;
	alert->torrent = torrent->shared_from_this();

	std::lock_guard<std::mutex> guard(alertsMutex);
	alerts.push_back(std::move(alert));
}

void mtt::AlertsManager::metadataAlert(Alerts::Id id, Torrent* torrent)
{
	if (!isAlertRegistered(id))
		return;

	auto alert = std::make_unique<MetadataAlert>();
	alert->id = id;
	alert->torrent = torrent->shared_from_this();

	std::lock_guard<std::mutex> guard(alertsMutex);
	alerts.push_back(std::move(alert));
}

void mtt::AlertsManager::configAlert(Alerts::Id id, config::ValueType type)
{
	if (!isAlertRegistered(id))
		return;

	auto alert = std::make_unique<ConfigAlert>();
	alert->id = id;
	alert->configType = type;

	std::lock_guard<std::mutex> guard(alertsMutex);
	alerts.push_back(std::move(alert));
}

mtt::AlertsManager& mtt::AlertsManager::Get()
{
	return instance;
}

void mtt::AlertsManager::registerAlerts(uint64_t m)
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

bool mtt::AlertsManager::isAlertRegistered(Alerts::Id id)
{
	return (alertsMask & id);
}

