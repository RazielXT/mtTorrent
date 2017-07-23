#pragma once
#include <iostream>

extern void LockLog();
extern void UnlockLog();
extern std::string GetLogTime();

#define WRITE_LOG(x) {LockLog(); std::cout << GetLogTime() << x << "\n"; UnlockLog();}
#define INFO_LOG(x) {}//WRITE_LOG("INFO: " << x)
