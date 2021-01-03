// Dear ImGui: standalone example application for GLFW + OpenGL2, using legacy fixed pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_glfw_opengl2/ folder**
// See imgui_impl_glfw.cpp for details.

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <stdio.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>

#include "Api/Core.h"
#include "Api/Configuration.h"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

std::string formatBytes(uint64_t bytes)
{
	const char* type = " MB";

	auto sz = ((bytes / 1024.f) / 1024.f);
	if (sz < 1)
	{
		sz = (float)(bytes / (1024ll));
		type = " KB";

		if (sz == 0)
		{
			sz = (float)bytes;
			type = " B";
		}
	}
	else if (sz > 1024)
	{
		sz /= 1024.f;
		type = " GB";
	}

	std::string str(50, '\0');
	snprintf(&str[0], 50, "%.2f", sz);
	str.resize(strlen(str.data()));

	while (str.back() == '0')
		str.pop_back();

	if (str.back() == '.')
		str.pop_back();

	return str + type;
}

std::string formatBytesSpeed(uint64_t bytes)
{
	return formatBytes(bytes) + "/s";
}

int main(int, char**)
{
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;
	GLFWwindow* window = glfwCreateWindow(1280, 720, "mtTorrent SdkGuiExample", NULL, NULL);
	if (window == NULL)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL2_Init();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	auto core = mttApi::Core::create();

	auto settings = mtt::config::getExternal();
	settings.files.defaultDirectory = "./";
	mtt::config::setValues(settings.files);
	settings.dht.enabled = true;
	mtt::config::setValues(settings.dht);

	core->registerAlerts((int)mtt::AlertId::MetadataFinished);

	//magnet test
	auto addResult = core->addMagnet("magnet:?xt=urn:btih:9b8d456ba5e2ce92023b069743e0d1051f199034&dn=%5BDameDesuYo%5D%20Shingeki%20no%20Kyojin%20%28The%20Final%20Season%29%20-%2063v0%20%281920x1080%2010bit%20AAC%29%20%5B098588E9%5D.mkv&tr=http%3A%2F%2Fnyaa.tracker.wf%3A7777%2Fannounce&tr=udp%3A%2F%2Fopen.stealth.si%3A80%2Fannounce&tr=udp%3A%2F%2Ftracker.opentrackr.org%3A1337%2Fannounce&tr=udp%3A%2F%2Ftracker.coppersurfer.tk%3A6969%2Fannounce&tr=udp%3A%2F%2Fexodus.desync.com%3A6969%2Fannounce");
	if (!addResult.second)
	{
		return 0;
	}

	auto torrentPtr = addResult.second;
	torrentPtr->start();

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		static ImGuiTableFlags flags = ImGuiTableFlags_SizingPolicyFixed | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

		{
			ImGui::Begin("Torrent info");

			ImGui::Text(torrentPtr->name().data());
			ImGui::Text("Total size %s", formatBytes(torrentPtr->getFileInfo().info.fullSize).data());

			auto state = torrentPtr->getState();

			switch (state)
			{
			case mttApi::Torrent::State::CheckingFiles:
				ImGui::Text("Name: %s, checking files... %f%%\n", torrentPtr->checkingProgress() * 100);
				break;
			case mttApi::Torrent::State::DownloadingMetadata:
			{
				if (auto magnet = torrentPtr->getMagnetDownload())
				{
					mtt::MetadataDownloadState utmState = magnet->getState();
					if (utmState.partsCount == 0)
						ImGui::Text("Metadata download getting peers... Connected peers: %u, Received peers: %u\n", torrentPtr->getPeers()->connectedCount(), torrentPtr->getPeers()->receivedCount());
					else
						ImGui::Text("Metadata download progress: %u / %u, Connected peers: %u, Received peers: %u\n", utmState.receivedParts, utmState.partsCount, torrentPtr->getPeers()->connectedCount(), torrentPtr->getPeers()->receivedCount());

					std::vector<std::string> logs;
					if (auto count = magnet->getDownloadLog(logs, 0))
					{
						for (auto& l : logs)
							ImGui::Text("Metadata log: %s\n", l.data());
					}
				}
				break;
			}
			case mttApi::Torrent::State::Downloading:
				ImGui::Text("Downloading, Progress %f%%, Speed: %s, Connected peers: %u, Found peers: %u\n", torrentPtr->currentProgress() * 100, formatBytesSpeed(torrentPtr->getFileTransfer()->getDownloadSpeed()).data(), torrentPtr->getPeers()->connectedCount(), torrentPtr->getPeers()->receivedCount());
				break;
			case mttApi::Torrent::State::Seeding:
				ImGui::Text("Finished, upload speed: %s\n", formatBytesSpeed(torrentPtr->getFileTransfer()->getUploadSpeed()).data());
				break;
			case mttApi::Torrent::State::Interrupted:
				ImGui::Text("Interrupted, problem code: %d\n", (int)torrentPtr->getLastError());
				break;
			case mttApi::Torrent::State::Inactive:
			default:
				break;
			}

			ImGui::End();
		}

		{
			ImGui::Begin("Torrent peers");

			auto peerStateToString = [](mtt::ActivePeerInfo state) -> std::string
			{
				if (!state.connected)
					return "Connecting";
				else if (state.choking)
					return "Requesting";
				else
					return "Connected";
			};

			if (ImGui::BeginTable("##PeersTable", 4, flags))
			{
				ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Dl speed", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Client Id", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				auto peers = torrentPtr->getFileTransfer()->getPeersInfo();
				for (size_t row = 0; row < peers.size(); row++)
				{
					const auto& peer = peers[row];

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text(peer.address.data());

					ImGui::TableSetColumnIndex(1);
					ImGui::Text(peerStateToString(peer).data());

					ImGui::TableSetColumnIndex(2);
					ImGui::Text(formatBytesSpeed(peer.downloadSpeed).data());

					ImGui::TableSetColumnIndex(3);
					ImGui::Text(peer.client.data());
				}
				ImGui::EndTable();
			}

			ImGui::End();
		}

		{
			ImGui::Begin("Torrent sources");

			auto sourceStateToString = [](mtt::TrackerState state) -> std::string
			{
				if (state == mtt::TrackerState::Connected || state == mtt::TrackerState::Alive)
					return "Ready";
				else if (state == mtt::TrackerState::Connecting)
					return "Connecting";
				else if (state == mtt::TrackerState::Announcing || state == mtt::TrackerState::Reannouncing)
					return "Announcing";
				else if (state == mtt::TrackerState::Announced)
					return "Announced";
				else if (state == mtt::TrackerState::Offline)
					return "Offline";
				else
					return "Stopped";
			};

			if (ImGui::BeginTable("##SourcesTable", 4, flags))
			{
				ImGui::TableSetupColumn("Hostname", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Received", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Found", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				auto sources = torrentPtr->getPeers()->getSourcesInfo();
				for (size_t row = 0; row < sources.size(); row++)
				{
					const auto& source = sources[row];

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text(source.hostname.data());

					ImGui::TableSetColumnIndex(1);
					ImGui::Text(sourceStateToString(source.state).data());

					ImGui::TableSetColumnIndex(2);
					ImGui::Text(std::to_string(source.peers).data());

					ImGui::TableSetColumnIndex(3);
					ImGui::Text(std::to_string(source.seeds).data());
				}
				ImGui::EndTable();
			}

			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		// If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
		// you may need to backup/reset/restore current shader using the commented lines below.
		//GLint last_program;
		//glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
		//glUseProgram(0);
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		//glUseProgram(last_program);

		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
