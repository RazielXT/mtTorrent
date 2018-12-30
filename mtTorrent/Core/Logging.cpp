#include <iostream>
#include <sstream>
#include <mutex>
#include "Logging.h"

std::mutex logMutex;

void LockLog()
{
	logMutex.lock();
}

void UnlockLog()
{
	logMutex.unlock();
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
	if (type != LogTypeDownload && type != LogTypeTest)
		return;

	LockLog();

	std::cout << GetLogTime() << type << ": " << ss.str() << "\n";

	UnlockLog();
}
