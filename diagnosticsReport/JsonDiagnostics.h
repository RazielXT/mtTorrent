#pragma once

#include "../mtTorrent/Core/Diagnostics/Diagnostics.h"

class JsonDiagnostics : public mtt::Diagnostics::Storage
{
public:

	std::string generateJson();
};