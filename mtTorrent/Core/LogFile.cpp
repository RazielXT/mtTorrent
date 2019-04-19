#include "LogFile.h"
#include <fstream>

LogFile::LogFile(std::string name) : logName(name)
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

void LogFile::clear()
{
	logMutex.lock();

	std::ofstream outfile;
	outfile.open(logName);

	if (outfile.good())
		outfile << GetLogTime() << ": start\n";

	logMutex.unlock();
}

