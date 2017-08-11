#pragma once

#include "json\picojson.h"

namespace js = picojson;

void printChampion(js::value& value, std::ostringstream& output);

void fillChampionInfo(js::object& map);

void printJson(js::value& value, std::ostringstream& output);

void riotApiRequest();