cmake_minimum_required (VERSION 3.8)

add_executable (mtTorrentSdkExample
	main
)

target_compile_features (mtTorrentSdkExample
	PUBLIC
		cxx_std_17
)

target_link_libraries (mtTorrentSdkExample mtTorrent)

target_include_directories (mtTorrentSdkExample PUBLIC ../mtTorrent/)

target_compile_features (mtTorrentSdkExample
	PUBLIC
		cxx_std_17
)
