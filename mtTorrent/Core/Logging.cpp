#include "Logging.h"
#include <iostream>
#include <mutex>
#include <vector>
#include <fstream>

std::mutex logMutex;

void LockLog()
{
	logMutex.lock();
}

void UnlockLog()
{
	logMutex.unlock();
}

clock_t startLogTime = 0;

void InitLogTime()
{
	startLogTime = clock();
}

std::string FormatLogTime(long time)
{
	clock_t t = time - startLogTime;
	char buffer[10] = { 0 };
	sprintf_s(buffer, 10, "%.3f ", ((float)t) / CLOCKS_PER_SEC);

	return buffer;
}

std::string GetLogTime()
{
	static clock_t startTime = clock();

	clock_t t = clock() - startTime;
	char buffer[10] = { 0 };
	sprintf_s(buffer, 10, "%.3f ", ((float)t) / CLOCKS_PER_SEC);

	return buffer;
}

bool isUdpType(const char* t)
{
	return strncmp(t, "Udp", 3) == 0;
}

bool isBtType(const char* t)
{
	return strncmp(t, "Bt", 2) == 0;
}

void WriteLogImplementation(const char * const type, std::stringstream& ss)
{
	if (type != LogTypeBtUtm && type != LogTypeTest)
		return;

	LockLog();

	std::cout << GetLogTime() << type << ": " << ss.str() << "\n";

	UnlockLog();
}

class FileLogger
{
public:
	std::vector<std::string> logStrings;
	std::vector<std::string> prevLogStrings;

	void serialize(bool append)
	{
		if (logStrings.empty())
			return;

		std::ofstream file("fileLogger", append ? std::ios_base::app : std::ios_base::out);

		for (auto& s : logStrings)
		{
			file << s << "\n";
		}

		if(append)
			logStrings.clear();
	}

	~FileLogger()
	{
		serialize(false);
	}
};

FileLogger fileLogger;

void serializeFileLog()
{
	LockLog();
	fileLogger.serialize(true);
	UnlockLog();
}

void WriteLogFileImplementation(const char* const type, std::stringstream& ss)
{
	LockLog();

	fileLogger.logStrings.push_back(GetLogTime() + " (" + type + "): " + ss.str());
	if (fileLogger.logStrings.size() > 50000)
	{
		std::swap(fileLogger.logStrings, fileLogger.prevLogStrings);
		fileLogger.logStrings.clear();
	}

	UnlockLog();
}
