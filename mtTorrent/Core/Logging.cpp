#include "Logging.h"
#include <iostream>
#include <mutex>
#include <vector>
#include <fstream>
#include <cstring>
#include <map>
#include <filesystem>

clock_t startTime = clock();

float GetLogTimeT()
{
	clock_t t = clock() - startTime;

	return ((float)t) / CLOCKS_PER_SEC;
}

uint32_t EnabledLogsFlag = 0;// (uint32_t)LogType::TcpStream | (uint32_t)LogType::Peer | (uint32_t)LogType::Downloader;// (uint32_t)LogType::Tcp | (uint32_t)LogType::Bandwidth | (uint32_t)LogType::Peer;// (uint32_t)LogType::UtpStream | (uint32_t)LogType::Downloader; //(uint32_t)LogType::PeerCommunicationDiagnostics | (uint32_t)LogType::UtpStream | (uint32_t)LogType::Downloader;

void EnableLog(LogType t, bool enable)
{
	if (enable)
		EnabledLogsFlag |= (uint32_t)t;
	else
		EnabledLogsFlag &= ~(uint32_t)t;
}

bool LogEnabled(LogType t)
{
	return EnabledLogsFlag & (uint32_t)t;
}

LogWriter& LogWriter::operator<<(float f)
{
	data.push_back((char)LogWriter::FLOAT);
	data.insert(data.end(), (char*)&f, ((char*)&f) + sizeof(float));
	return *this;
}

LogWriter& LogWriter::operator<<(uint16_t i)
{
	data.push_back((char)LogWriter::UINT16);
	data.insert(data.end(), (char*)&i, ((char*)&i) + sizeof(uint16_t));
	return *this;
}

LogWriter& LogWriter::operator<<(uint32_t i)
{
	data.push_back((char)LogWriter::UINT);
	data.insert(data.end(), (char*)&i, ((char*)&i) + sizeof(uint32_t));
	return *this;
}

LogWriter& LogWriter::operator<<(const Addr& a)
{
	data.push_back((char)LogWriter::ADDR);
	data.insert(data.end(), (char*)a.addrBytes, ((char*)&a.addrBytes) + 4);
	data.insert(data.end(), (char*)&a.port, ((char*)&a.port) + 2);
	return *this;
}

LogWriter& LogWriter::operator<<(int64_t i)
{
	data.push_back((char)LogWriter::INT64);
	data.insert(data.end(), (char*)&i, ((char*)&i) + sizeof(int64_t));
	return *this;
}

uint8_t LogWriter::CreateLogLineId()
{
	static std::mutex logMutex;

	std::lock_guard<std::mutex> guard(logMutex);

	static std::map<LogType, uint8_t> logLineCounters;

	return ++logLineCounters[type];
}

void LogWriter::StartLogLine(uint8_t id)
{
	*this << LogWriter::START;
	data.push_back(id);
	*this << GetLogTimeT();

	if (id >= lineParams.size())
	{
		lineParams.resize(id + 1);
	}

	logLineIdFirstTime = (id == NoParamsLineId) ? NoParamsLineId : (lineParams[id].empty() ? id : NoParamsLineId);
}

LogWriter& LogWriter::operator<<(const char*& ptr)
{
	data.push_back((char)LogWriter::STR);
	data.insert(data.end(), ptr, ptr + strlen(ptr) + 1);

	return *this;
}

LogWriter& LogWriter::operator<<(const std::string& str)
{
	data.push_back((char)LogWriter::STR);
	data.insert(data.end(), str.c_str(), str.c_str() + str.length() + 1);
	return *this;
}

LogWriter& LogWriter::operator<<(LogWriter::DataType t)
{
	data.push_back((char)t);

	return *this;
}

LogWriter& LogWriter::operator<<(int i) { return *this << (uint32_t)i; };
LogWriter& LogWriter::operator<<(uint64_t i) { return *this << (uint32_t)i; };

LogWriter::LogWriter(LogType t) : type(t)
{
	//NoParamsLineId 0
	lineParams.emplace_back();
}

LogWriter::~LogWriter()
{
	if (data.empty() || name.empty() || name.back() == '_')
		return;

	std::replace(name.begin(), name.end(), ':', '.');

	std::string folder = "./logs/";
	std::error_code ec;
	if (!std::filesystem::create_directories(folder, ec) && ec)
	{
		auto msg = ec.message();
		return;
	}

	bool append = false;

	std::ofstream file(folder + name + ".txt", append ? std::ios_base::app : std::ios_base::out);

	size_t pos = 0;
	uint8_t currentLineId = 0;
	size_t currentLineParamPos = 0;

	while (pos < data.size())
	{
		auto type = (LogWriter::DataType)data[pos];
		pos++;

		if (type == ENDL)
		{
			file << "\n";
		}
		else if (type == START)
		{
			currentLineId = data[pos++];
			currentLineParamPos = 0;
		}
		else if (type == UINT16)
		{
			file << *(uint16_t*)&data[pos] << " ";
			pos += sizeof(uint16_t);
		}
		else if (type == UINT)
		{
			file << *(uint32_t*)&data[pos] << " ";
			pos += sizeof(uint32_t);
		}
		else if (type == INT64)
		{
			file << *(int64_t*)&data[pos] << " ";
			pos += sizeof(int64_t);
		}
		else if (type == FLOAT)
		{
			file << *(float*)&data[pos] << " ";
			pos += sizeof(float);
		}
		else if (type == ADDR)
		{
			file << Addr((uint8_t*)&data[pos], (uint16_t&)data[pos+4], false).toString() << " ";
			pos += 6;
		}
		else if (type == STR)
		{
			auto stringPtr = (const char*)&data[pos];
			file << stringPtr << " ";
			pos += strlen(stringPtr) + 1;
		}
		else if (type == PARAM && currentLineId != NoParamsLineId)
		{
			auto stringPtr = lineParams[currentLineId][currentLineParamPos++];
			file << stringPtr;
		}
	}
}

LogWriter* LogWriter::GetGlobalLog(LogType t, const char* name)
{
	static std::mutex logMutex;
	static std::map<const char*, FileLog> writers;

	std::lock_guard<std::mutex> guard(logMutex);

	if (writers.find(name) == writers.end())
	{
		auto ptr = Create(t);
		ptr->name = name;
		writers[name] = std::move(ptr);
	}

	return writers[name].get();
}

FileLog LogWriter::Create(LogType type)
{
	return std::move(std::make_unique<LogWriter>(type));
}

bool LogWriter::Enabled() const
{
	return LogEnabled(type);
}
