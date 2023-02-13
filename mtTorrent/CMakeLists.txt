cmake_minimum_required (VERSION 3.8)

set (core_sources
	AlertsManager.cpp
	BinaryInterfaceHandler.cpp
	Configuration.cpp
	Core.cpp
	Downloader.cpp
	DownloadStorage.cpp
	ExtensionProtocol.cpp
	Files.cpp
	FileTransfer.cpp
	HolepunchExtension.cpp
	HttpsTrackerComm.cpp
	HttpTrackerComm.cpp
	IncomingPeersListener.cpp
	Interface.cpp
	Logging.cpp
	main.cpp
	MetadataDownload.cpp
	MetadataExtension.cpp
	MetadataReconstruction.cpp
	PeerCommunication.cpp
	PeerMessage.cpp
	Peers.cpp
	PexExtension.cpp
	PiecesProgress.cpp
	State.cpp
	Storage.cpp
	Torrent.cpp
	TrackerManager.cpp
	UdpTrackerComm.cpp
	Uploader.cpp
	PeerStream.cpp
	ProtocolEncryption.cpp
)

list(TRANSFORM core_sources PREPEND "Core/")

set (core_api_sources
	CoreImpl.cpp
	FileTransferImpl.cpp
	InterfaceImpl.cpp
	ListenerImpl.cpp
	MagnetDownloadImpl.cpp
	PeersImpl.cpp
	TorrentImpl.cpp
	DhtImpl.cpp
)

list(TRANSFORM core_api_sources PREPEND "Core/Api/")

set (core_dht_sources
	Communication.cpp
	Node.cpp
	Query.cpp
	Responder.cpp
	Table.cpp
)

list(TRANSFORM core_dht_sources PREPEND "Core/Dht/")

set (core_utp_sources
	UtpManager.cpp
	UtpStream.cpp
)

list(TRANSFORM core_utp_sources PREPEND "Core/Utp/")

set (public_sources
	ModuleString.cpp
)

list(TRANSFORM public_sources PREPEND "Public/")

set (utils_sources
	Bandwidth.cpp
	Base32.cpp
	BencodeParser.cpp
	BencodeWriter.cpp
	BitUtils.cpp
	FastIpToCountry.cpp
	HexEncoding.cpp
	HttpHeader.cpp
	NetAdaptersList.cpp
	Network.cpp
	ScheduledTimer.cpp
	ServiceThreadpool.cpp
	SHA.cpp
	SslAsyncStream.cpp
	TcpAsyncServer.cpp
	TcpAsyncStream.cpp
	TorrentFileParser.cpp
	UdpAsyncComm.cpp
	UdpAsyncReceiver.cpp
	UdpAsyncWriter.cpp
	UpnpDiscovery.cpp
	UpnpPortMapping.cpp
	Uri.cpp
	UrlEncoding.cpp
	XmlParser.cpp
	BigNumber.cpp
	DataBuffer.cpp
	DiffieHellman.cpp
	RC4.cpp
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
