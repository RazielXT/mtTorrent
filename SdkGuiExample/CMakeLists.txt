cmake_minimum_required (VERSION 3.8)

if (WIN32 AND MSVC)
	set (GLFW_LOCAL TRUE)
endif ()

set (imgui_sources
	imgui.cpp
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_tables.cpp
	imgui_widgets.cpp
)

list (TRANSFORM imgui_sources PREPEND "imgui/")

set (imgui_backends
	imgui_impl_opengl2.cpp
	imgui_impl_glfw.cpp
)

list (TRANSFORM imgui_backends PREPEND "imgui/backends/")

add_executable (mtTorrentSdkGuiExample
	main.cpp
	TorrentInstance.cpp
	${imgui_sources}
	${imgui_backends}
)

target_include_directories (mtTorrentSdkGuiExample PUBLIC ../ imgui imgui/backends)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
message ("CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")

set (OpenGL_GL_PREFERENCE GLVND)

find_package (OpenGL REQUIRED)
message (STATUS "Found OpenGL ${OPENGL_FOUND} in ${OPENGL_INCLUDE_DIR}")

target_link_libraries (mtTorrentSdkGuiExample OpenGL::GL ${CMAKE_DL_LIBS})

if (GLFW_LOCAL)
	message (STATUS "Linking GLFW3 from ImGui")

	target_include_directories (mtTorrentSdkGuiExample PUBLIC imgui/examples/libs/glfw/include)
	target_link_libraries (mtTorrentSdkGuiExample ${CMAKE_SOURCE_DIR}/SdkGuiExample/imgui/examples/libs/glfw/lib-vc2010-64/glfw3.lib)
else ()
	find_package (glfw3 3.3 REQUIRED)
	message (STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")

	find_package (GLEW REQUIRED)
	message (STATUS "Found GLEW in ${GLEW_INCLUDE_DIRS}")

	target_link_libraries (mtTorrentSdkGuiExample glfw GLEW)
endif ()

target_compile_features (mtTorrentSdkGuiExample
	PUBLIC
		cxx_std_17
)

target_link_libraries (mtTorrentSdkGuiExample mtTorrent)

