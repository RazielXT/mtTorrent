cmake_minimum_required (VERSION 3.8)

set (core_sources
	AlertsManager
	BinaryInterfaceHandler
	Configuration
	Core
	Downloader
	ExtensionProtocol
	Files
	FileTransfer
	HttpsTrackerComm
	HttpTrackerComm
	IncomingPeersListener
	Interface
	Logging
	main
	MetadataDownload
	MetadataReconstruction
	PeerCommunication
	PeerMessage
	Peers
	PiecesProgress
	State
	Storage
	Torrent
	TrackerManager
	UdpTrackerComm
	Uploader
	PeerStream
	ProtocolEncryption
)

list(TRANSFORM core_sources PREPEND "Core/")

set (core_api_sources
	CoreImpl
	FileTransferImpl
	InterfaceImpl
	ListenerImpl
	MagnetDownloadImpl
	PeersImpl
	TorrentImpl
)

list(TRANSFORM core_api_sources PREPEND "Core/Api/")

set (core_dht_sources
	Communication
	Node
	Query
	Responder
	Table
)

list(TRANSFORM core_dht_sources PREPEND "Core/Dht/")

set (core_utp_sources
	UtpManager
	UtpStream
)

list(TRANSFORM core_utp_sources PREPEND "Core/Utp/")

set (public_sources
	ModuleString
)

list(TRANSFORM public_sources PREPEND "Public/")

set (utils_sources
	Bandwidth
	Base32
	BencodeParser
	BencodeWriter
	ByteSwap
	FastIpToCountry
	HexEncoding
	HttpHeader
	NetAdaptersList
	Network
	ScheduledTimer
	ServiceThreadpool
	SHA
	SslAsyncStream
	TcpAsyncServer
	TcpAsyncStream
	TorrentFileParser
	UdpAsyncComm
	UdpAsyncReceiver
	UdpAsyncWriter
	UpnpDiscovery
	UpnpPortMapping
	Uri
	UrlEncoding
	XmlParser
	BigNumber
	DataBuffer
	DiffieHellman
	RC4
)

list(TRANSFORM utils_sources PREPEND "utils/")

option (BUILD_SHARED_LIBS "build mtTorrent as a shared library" ON)

add_library (mtTorrent
	${core_sources}
	${core_api_sources}
	${core_dht_sources}
	${core_utp_sources}
	${public_sources}
	${utils_sources}
)

target_include_directories (mtTorrent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} Core)

target_compile_features (mtTorrent
	PUBLIC
		cxx_std_17
)

#ASIO

add_compile_definitions (ASIO_STANDALONE)

target_include_directories (mtTorrent PUBLIC asio/asio/include)

if (WIN32)
	add_compile_definitions (_WIN32_WINNT=0x0601)
elseif (UNIX)
	find_package ( Threads )
	target_link_libraries ( mtTorrent ${CMAKE_THREAD_LIBS_INIT} )
endif ()
