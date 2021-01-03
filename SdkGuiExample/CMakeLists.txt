cmake_minimum_required (VERSION 3.8)

set (imgui_sources
	imgui
	imgui_demo
	imgui_draw
	imgui_tables
	imgui_widgets
)

list (TRANSFORM imgui_sources PREPEND "imgui/")

set (imgui_backends
	imgui_impl_opengl2
	imgui_impl_glfw
)

list (TRANSFORM imgui_backends PREPEND "imgui/backends/")

add_executable (mtTorrentSdkGuiExample
	main
	TorrentInstance
	${imgui_sources}
	${imgui_backends}
)

target_include_directories (mtTorrentSdkGuiExample PUBLIC imgui imgui/backends)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
message("CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")

set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
message(STATUS "Found OpenGL in ${OPENGL_INCLUDE_DIR}")

find_package(glfw3 3.3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")

find_package(GLEW REQUIRED)
message(STATUS "Found GLEW in ${GLEW_INCLUDE_DIRS}")

target_include_directories(mtTorrentSdkGuiExample PRIVATE ${OPENGL_INCLUDE_DIR})

target_link_libraries(mtTorrentSdkGuiExample OpenGL::OpenGL glfw GLEW ${CMAKE_DL_LIBS})

target_compile_features (mtTorrentSdkGuiExample
	PUBLIC
		cxx_std_17
)

target_link_libraries (mtTorrentSdkGuiExample mtTorrent)

target_include_directories (mtTorrentSdkGuiExample PUBLIC ../mtTorrent/)
