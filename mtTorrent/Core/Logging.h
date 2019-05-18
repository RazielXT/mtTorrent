#pragma once
#include <sstream>

extern void WriteLogImplementation(const char * const,std::stringstream&);

#define LOG_TYPE(x) const char * const LogType##x = #x

LOG_TYPE(Udp);
LOG_TYPE(Tcp);
LOG_TYPE(Test);
LOG_TYPE(Dht);
LOG_TYPE(HttpTracker);
LOG_TYPE(UdpTracker);
LOG_TYPE(Bt);
LOG_TYPE(BtUtm);
LOG_TYPE(BtExt);
LOG_TYPE(BencodeParser);
LOG_TYPE(FileParser);
LOG_TYPE(UdpListener);
LOG_TYPE(UdpMgr);
LOG_TYPE(Download);

#ifdef MTT_TEST_STANDALONE
#define WRITE_LOG(type, x) {std::stringstream ss; ss << x; WriteLogImplementation(type, ss);}
#else
#define WRITE_LOG(type, x) {}
#endif