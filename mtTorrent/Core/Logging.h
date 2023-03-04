#pragma once

#include "utils/Network.h"
#include <vector>
#include <mutex>

enum class LogType
{
	FIRST = 1,
	Peer = FIRST,
	ExtensionProtocol = FIRST << 1,
	MetadataDownload = FIRST << 2,
	Downloader = FIRST << 3,
	Transfer = FIRST << 4,
	Peers = FIRST << 5,
	HttpTracker = FIRST << 6,
	UdpTracker = FIRST << 7,
	Dht = FIRST << 8,
	TcpListener = FIRST << 9,
	SslTcp = FIRST << 10,
	UdpWriter = FIRST << 11,
	UdpListener = FIRST << 12,
	UdpMgr = FIRST << 13,
	UtpStream = FIRST << 14,
	BencodeParser = FIRST << 15,
	Bandwidth = FIRST << 16,
	PeersListener = FIRST << 17,
	TcpStream = FIRST << 18,
	PeerStream = FIRST << 19,
	General = FIRST << 20,
	Upnp = FIRST << 21,
	Uploader = FIRST << 22,

	Test = FIRST << 30
};

void EnableLog(LogType t, bool enable);
bool LogEnabled(LogType t);

struct LogWriter;
using FileLog = std::unique_ptr<LogWriter>;

struct LogWriter
{
	LogWriter(LogType t);
	~LogWriter();

	static LogWriter* GetGlobalLog(LogType,const char*);
	static FileLog Create(LogType);

	bool Enabled() const;

	uint8_t CreateLogLineId();
	void StartLogLine(uint8_t);

	enum DataType { START, PARAM, STR, FLOAT, UINT16, UINT, INT64, ADDR, ENDL };

	LogWriter& operator<<(float);
	LogWriter& operator<<(uint16_t);
	LogWriter& operator<<(uint32_t);
	LogWriter& operator<<(int);
	LogWriter& operator<<(int64_t);
	LogWriter& operator<<(uint64_t);
	LogWriter& operator<<(LogWriter::DataType);
	LogWriter& operator<<(const char*&);
	LogWriter& operator<<(const std::string&);
	LogWriter& operator<<(const Addr&);

	//save literals as list of pointers
	template <int N>
	LogWriter& operator<<(const char(&ptr)[N])
	{
		if (logLineIdFirstTime != NoParamsLineId)
		{
			lineParams[logLineIdFirstTime].push_back(ptr);
		}

		data.push_back((char)LogWriter::PARAM);

		return *this;
	}

	static const char NoParamsLineId = 0;

	std::vector<char> data;
	std::vector<std::vector<const char*>> lineParams;
	std::string name;
	LogType type{};

	std::mutex mtx;

	void assignIndex();
	int idx = 0;

private:

	void flush();
	uint8_t logLineIdFirstTime = NoParamsLineId;
};

//----------
// log per object instance

#define CREATE_LOG(type) log = LogEnabled(LogType::type) ? LogWriter::Create(LogType::type) : nullptr; if (log) log->name = #type "_";
#define CREATE_NAMED_LOG(type, n) CREATE_LOG(type) if (log) log->name += n;
#define NAME_LOG(n)if (log && log->name.back() == '_') log->name += n;
#define INDEX_LOG()if (log) log->assignIndex();

#define WRITE_LOG(logdata)  if (log && log->Enabled()) { static auto LogId = log->CreateLogLineId(); std::lock_guard<std::mutex> guard(log->mtx); log->StartLogLine(LogId); *log << logdata << LogWriter::ENDL;}

//----------
// log per object type

#define WRITE_GLOBAL_LOG(type, logdata) if (LogEnabled(LogType::type)) { static auto log = LogWriter::GetGlobalLog(LogType::type, #type); std::lock_guard<std::mutex> guard(log->mtx); static auto LogId = log->CreateLogLineId(); log->StartLogLine(LogId); *log << logdata << LogWriter::ENDL;}
