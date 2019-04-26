//source csv http://software77.net/geo-ip/

#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <array>
#include <map>

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	return split(s, delim, elems);
}

template <class T>
T stringTo(const std::string &str)
{
	T result;
	std::stringstream ss(str);
	ss >> result;
	return result;
}

template <class Ty, class Container>
std::vector<Ty> convertContainerTo(const Container &source)
{
	std::vector<Ty> result;
	std::for_each(source.begin(), source.end(),
		[&result](const std::string &it) { result.push_back(stringTo<Ty>(it)); });
	return result;
}

typedef unsigned int uint32_t;
typedef uint32_t IpAddress_t;

/** IP address mapping entry */
class IpAddressMapping {
public:
	std::string country;
	IpAddress_t startAddress;
};

/** Class for mapping IP addresses to countries using database
  * from http://software77.net/geo-ip/ */
class IpToCountry
{
public:
	std::array<std::vector<IpAddressMapping>, 256> m_countryIpList;

	/** Construct new IP-to-country mapper from the specified file. */
	IpToCountry(const std::string &FileName = "IpToCountry.csv")
	{
		std::ifstream file(FileName);
		while (file.good() && !file.eof())
		{
			std::string line;
			std::getline(file, line);
			if (line.find_first_of('#') == std::string::npos && line.length() > 0)
			{
				IpAddressMapping mapping = ParseSingleLine(line);
				m_countryIpList[GetIndexFromAddress(mapping.startAddress)].push_back(mapping);
			}
		}
	}

	/** Find the country for given IP address or throw std::exception. */
	IpAddressMapping GetCountry(const std::string &address) const
	{
		IpAddress_t integerIp = IntegerFromIp(address);
		return GetCountryFromIndex(integerIp, GetIndexFromAddress(integerIp));
	}

	/** Convert a human-readable ipv4 address to integer */
	static IpAddress_t IntegerFromIp(const std::string &ip)
	{
		auto tokens = split(ip, '.');
		auto integers = convertContainerTo<uint32_t>(tokens);
		return (integers[0] << (3 * 8)) +
			(integers[1] << (2 * 8)) +
			(integers[2] << (1 * 8)) +
			integers[3];
	}

private:

	static unsigned char GetIndexFromAddress(IpAddress_t address)
	{
		return address >> (3 * 8);
	}

	IpAddressMapping GetCountryFromIndex(const IpAddress_t address, unsigned char index) const
	{
		const auto &list = m_countryIpList[index];
		auto it = std::find_if(list.rbegin(), list.rend(),
			[address](IpAddressMapping it) { return it.startAddress <= address; });
		if (it == list.rend())
			return GetCountryFromIndex(address, index - 1);
		return *it;
	}

	// File format:
	// "1464729600","1464860671","ripencc","1117497600","DE","DEU","Germany"
	static IpAddressMapping ParseSingleLine(const std::string &line)
	{
		IpAddressMapping mapping;
		auto tokens = split(line, ',');
		mapping.country = tokens[6].substr(1, tokens[6].length() - 2);
		mapping.startAddress = stringTo<uint32_t>(tokens[0].substr(1, tokens[0].length() - 2));
		return mapping;
	}
};

class FastIpToCountry
{
protected:

	std::map<std::string, uint16_t> countryMap;
	std::array<std::vector<std::pair<IpAddress_t, uint16_t>>, 256> buckets;

	const char* filename = "ipclist";

public:

	void toFile()
	{
		std::ofstream file(filename);

		for (auto& c : countryMap)
		{
			file << c.second << c.first << '\n';
		}

		file << '\n';

		for (auto& b : buckets)
		{
			for (auto& range : b)
			{
				file.write((char*)&range.first, sizeof(range.first));
				file.write((char*)&range.second, sizeof(range.second));
			}

			file.write("\0\0\0\0\0\0", 6);
		}
	}

	void fromFile()
	{
		size_t bucket = 0;
		bool countriesLoaded = false;

		std::ifstream file(filename, std::ios::binary);
		while (file.good() && !file.eof())
		{
			if (!countriesLoaded)
			{
				std::string line;
				std::getline(file, line);
				line.pop_back();

				if (line.empty())
				{
					countriesLoaded = true;
					std::getline(file, line);
				}
				else
				{
					char* nameStart = nullptr;
					uint16_t id = (uint16_t)strtol(line.data(), &nameStart, 10);
					countryMap[nameStart] = id;
				}
			}
			else
			{
				uint32_t ip = 0;
				uint16_t country = 0;
				file.read((char*)&ip, sizeof(ip));
				file.read((char*)&country, sizeof(country));

				if (country == 0 && ip == 0)
				{
					bucket++;
				}
				else
				{
					buckets[bucket].push_back({ ip, country });
				}
			}
		}
	}

};

class FastIpToCountryFromParser : public FastIpToCountry
{
public:

	void fromParser(IpToCountry& parser)
	{
		int b = 0;
		for (auto& bucket : parser.m_countryIpList)
		{
			uint16_t lastCountry = 0;
			buckets[b].reserve(bucket.size());

			for (auto& range : bucket)
			{
				auto& countryId = countryMap[range.country];

				if (countryId && countryId == lastCountry)
					continue;

				if (countryId == 0)
					countryId = (uint16_t)countryMap.size();

				lastCountry = countryId;

				buckets[b].push_back({ range.startAddress, countryId });
			}

			b++;
		}
	}
};

int main()
{
	FastIpToCountryFromParser fastParser;
	fastParser.fromFile();

	IpToCountry parser("D:\\IpToCountry.csv");

	fastParser.fromParser(parser);
	fastParser.toFile();

	FastIpToCountryFromParser fastParser2;
	fastParser2.fromFile();

	return 0;
}