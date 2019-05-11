#pragma once
#include <sstream>
#include <mutex>

#ifdef _DEBUG
#define BT_LOGFILE
#endif // DEBUG

class LogFile
{
public:
	LogFile();

	void append(std::stringstream&);
	void init(std::string name);

	std::string logName;
	std::mutex logMutex;
};

#ifdef BT_LOGFILE
#define LOG_APPEND(x) {std::stringstream ss; ss << x; log.append(ss);}
#else
#define LOG_APPEND(x) {}
#endif