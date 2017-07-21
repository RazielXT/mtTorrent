#pragma once
#include <iostream>

extern void LockLog();
extern void UnlockLog();
#define WRITE_LOG(x) {LockLog(); std::cout << x << "\n"; UnlockLog();}

#define DHT_LOG(x) WRITE_LOG("DHT: " << x)
#define PEER_LOG(x) {}//WRITE_LOG("PEER: " << x)
#define UDP_LOG(x) WRITE_LOG("UDP: " << x)
#define TCP_LOG(x) WRITE_LOG("TCP: " << x)
#define PARSER_LOG(x) {}//WRITE_LOG("PARSER: " << x)
#define GENERAL_INFO_LOG(x) {}//WRITE_LOG("INFO: " << x)
#define TRACKER_LOG(x) {}//WRITE_LOG("TRACKER: " << x)
