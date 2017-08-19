#pragma once
#include <sstream>

extern void WriteLogImplementation(std::stringstream&);

#define WRITE_LOG(x) {std::stringstream ss; ss << x; WriteLogImplementation(ss);}
