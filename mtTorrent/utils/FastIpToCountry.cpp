#include "FastIpToCountry.h"

void FastIpToCountry::toFile()
{
	std::ofstream file(filename, std::ios::binary);

	for (auto& c : countries)
	{
		file << c << '\n';
	}

	file << '\n';

	for (auto& b : buckets)
	{
		for (auto& range : b)
		{
			file.write((char*)& range.first, sizeof(range.first));
			file.write((char*)& range.second, sizeof(range.second));
		}

		file.write("EEEEEE", 6);
	}
}

void FastIpToCountry::fromFile()
{
	size_t bucket = 0;
	bool countriesLoaded = false;

	std::ifstream file(filename, std::ios::binary);
	while (file.good() && !file.eof())
	{
		if (countriesLoaded)
		{
			uint32_t ip = 0;
			uint16_t country = 17733;
			file.read((char*)& ip, sizeof(ip));
			file.read((char*)& country, sizeof(country));

			if (country == 17733) //"EE"
			{
				bucket++;
			}
			else
			{
				buckets[bucket].push_back({ ip, country });
			}
		}
		else
		{
			std::string line;
			std::getline(file, line);

			if (line.empty())
			{
				countriesLoaded = true;
			}
			else
			{
				countries.push_back(line);
			}
		}
	}
}

std::string FastIpToCountry::GetCountry(const uint32_t integerIp) const
{
	return GetCountryFromIndex(integerIp, GetIndexFromAddress(integerIp));
}

std::string FastIpToCountry::GetCountryFromIndex(const uint32_t address, unsigned char index) const
{
	const auto& list = buckets[index];
	auto it = std::lower_bound(list.begin(), list.end(), address,
		[](const auto & lhs, uint32_t rhs) -> bool {
			return lhs.first <= rhs;
		});

	if (it == list.end() || it == list.begin())
		return "";

	it--;

	return countries[it->second];
}
