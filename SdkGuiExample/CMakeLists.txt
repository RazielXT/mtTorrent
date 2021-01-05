cmake_minimum_required (VERSION 3.8)

if (WIN32 AND MSVC)
	set (GLFW_LOCAL TRUE)
endif ()

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

if (GLFW_LOCAL)
	set (gl3w_sources
		gl3w
	)

	list (TRANSFORM gl3w_sources PREPEND "imgui/examples/libs/gl3w/GL/")
endif ()

add_executable (mtTorrentSdkGuiExample
	main
	TorrentInstance
	${imgui_sources}
	${imgui_backends}
	${gl3w_sources}
)

target_include_directories (mtTorrentSdkGuiExample PUBLIC imgui imgui/backends imgui/examples/libs/gl3w imgui/examples/libs/glfw/include)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
message ("CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")

set (OpenGL_GL_PREFERENCE GLVND)

find_package (OpenGL REQUIRED)
message (STATUS "Found OpenGL ${OPENGL_FOUND} in ${OPENGL_INCLUDE_DIR}")

target_link_libraries (mtTorrentSdkGuiExample OpenGL::GL ${CMAKE_DL_LIBS})

if (GLFW_LOCAL)
	target_include_directories (mtTorrentSdkGuiExample PUBLIC imgui/examples/libs/gl3w imgui/examples/libs/glfw/include)
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

target_include_directories (mtTorrentSdkGuiExample PUBLIC ../)
