#include <mutex>
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
