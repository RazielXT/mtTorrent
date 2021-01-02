#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <array>

class FastIpToCountry
{
public:

	void toFile(const std::string& folder);
	void fromFile(const std::string& folder);

	std::string GetCountry(uint32_t integerIp) const;

protected:

	std::vector<std::string> countries;
	std::array<std::vector<std::pair<uint32_t, uint16_t>>, 256> buckets;

	const char* filename = "ipclist";

	static unsigned char GetIndexFromAddress(uint32_t address)
	{
		return address >> (3 * 8);
	}

	std::string GetCountryFromIndex(uint32_t address, unsigned char index) const;
};
