cmake_minimum_required (VERSION 3.18)

option(static_runtime "Build with static runtime" OFF)
option(BUILD_SHARED_LIBS "Build as a shared library" ON)
option(USE_SSL "Include OpenSSL for HTTPS" OFF)

set (core_sources
	AlertsManager.cpp
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
	FilesImpl.cpp
	InterfaceImpl.cpp
	ListenerImpl.cpp
	MetadataDownloadImpl.cpp
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

set (utils_sources
	Bandwidth.cpp
	Base32.cpp
	BencodeParser.cpp
	BencodeWriter.cpp
	ByteSwap.cpp
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
	Xml.cpp
	BigNumber.cpp
	DataBuffer.cpp
	DiffieHellman.cpp
	RC4.cpp
)

list(TRANSFORM utils_sources PREPEND "utils/")

add_library (mtTorrent
	${core_sources}
	${core_api_sources}
	${core_dht_sources}
	${core_utp_sources}
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

#OPENSSL
if (USE_SSL)
	add_compile_definitions (MTT_WITH_SSL)

	find_package(OpenSSL REQUIRED)
	include_directories(${OPENSSL_INCLUDE_DIR})
	target_link_libraries( mtTorrent PUBLIC OpenSSL::SSL)

	if (WIN32 AND static_runtime)
		target_link_libraries( mtTorrent PRIVATE crypt32)
		set(OPENSSL_MSVC_STATIC_RT ON)
	endif()

	if (WIN32)
		target_link_libraries( mtTorrent PRIVATE OpenSSL::applink)
	endif()

	if (NOT BUILD_SHARED_LIBS)
		set(OPENSSL_USE_STATIC_LIBS ON)
	endif()
endif()

#runtime
if (WIN32 AND static_runtime)
	ucm_set_runtime(STATIC)
endif()

#DLL
if (WIN32 AND BUILD_SHARED_LIBS)
	add_compile_definitions (MTT_IS_DLL)
endif()
