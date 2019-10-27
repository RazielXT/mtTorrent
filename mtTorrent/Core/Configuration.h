#pragma once
#include <Api/Configuration.h>
#include <functional>

namespace mtt
{
	namespace config
	{
		int registerOnChangeCallback(ValueType, std::function<void()>);
		void unregisterOnChangeCallback(int);

		void load();
		void save();
	}
}