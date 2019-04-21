#include "LogFile.h"
#include <fstream>

LogFile::LogFile()
{
}

extern std::string GetLogTime();

void LogFile::append(std::stringstream& ss)
{
	logMutex.lock();

	std::ofstream outfile;
	outfile.open(logName, std::ios_base::app);

	if(outfile.good())
		outfile << GetLogTime() << ": " << ss.str() << "\n";

	logMutex.unlock();
}

void LogFile::init(std::string name)
{
	logName = name;

#ifdef BT_LOGFILE
	logMutex.lock();

	std::ofstream outfile;
	outfile.open(logName);

	if (outfile.good())
		outfile << GetLogTime() << ": start\n";

	logMutex.unlock();
#endif
}
