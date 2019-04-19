#pragma once
#include <sstream>
#include <mutex>

class LogFile
{
public:
	LogFile() {}
	LogFile(std::string name);

	void append(std::stringstream&);
	void clear();

	std::string logName;
	std::mutex logMutex;
};

#define LOG_APPEND_2(log, x) {std::stringstream ss; ss << x; log.append(ss);}
#define LOG_APPEND(x) {std::stringstream ss; ss << x; log.append(ss);}