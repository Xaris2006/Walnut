#include "ApplicationGUI.h"

#include "Walnut/UI/UI.h"
#include "Walnut/Core/Log.h"

#include "glad/glad.h"

//
// Adapted from Dear ImGui OpenGl3-GLFW example
//

#include "imgui_internal.h"

#include "backends/imgui_impl_glfw.h"
#include "Glad_imgui_impl_opengl3.h"

#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "ImGui/ImGuiTheme.h"

#include "stb_image.h"
//#include "../SDL2/include/SDL.h"
//#include "../SDL2/include/SDL_image.h"


#include <iostream>

// Emedded font
#include "ImGui/Roboto-Regular.embed"
#include "ImGui/Roboto-Bold.embed"
#include "ImGui/Roboto-Italic.embed"

extern bool g_ApplicationRunning;

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

//#define IMGUI_UNLIMITED_FRAME_RATE

// Unlike g_MainWindowData.FrameIndex, this is not the the swapchain image index
// and is always guaranteed to increase (eg. 0, 1, 2, 0, 1, 2)
static uint32_t s_CurrentFrameIndex = 0;

static std::unordered_map<std::string, ImFont*> s_Fonts;

static Walnut::Application* s_Instance = nullptr;

static const int windowWidth = 1872 / 3, static const windowHeight = 1498 / 3;

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

namespace Walnut {

#include "Walnut/Embed/Walnut-Icon.embed"
#include "Walnut/Embed/WindowImages.embed"

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		s_Instance = this;

		Init();
	}

	Application::~Application()
	{
		Shutdown();

		s_Instance = nullptr;
	}

	Application& Application::Get()
	{
		return *s_Instance;
	}

	void Application::Init()
	{
		// Intialize logging
		Log::Init();

		// Setup GLFW window
		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
		{
			std::cerr << "Could not initalize GLFW!\n";
			return;
		}


		// GL 3.0 + GLSL 130
		const char* glsl_version = "#version 430";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only


		if (m_Specification.CustomTitlebar)
		{
			glfwWindowHint(GLFW_TITLEBAR, false);

			// NOTE(Yan): Undecorated windows are probably
			//            also desired, so make this an option
			// glfwWindowHint(GLFW_DECORATED, false);
		}

		GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

		int monitorX, monitorY;
		glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);

		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		m_WindowHandle = glfwCreateWindow(m_Specification.Width, m_Specification.Height, m_Specification.Name.c_str(), NULL, NULL);

		glfwMakeContextCurrent(m_WindowHandle);
		glfwSwapInterval(1); // Enable vsync
		//glfwShowWindow(m_WindowHandle);

		if (m_Specification.CenterWindow)
		{
			glfwSetWindowPos(m_WindowHandle,
				monitorX + (videoMode->width - m_Specification.Width) / 2,
				monitorY + (videoMode->height - m_Specification.Height) / 2);

			glfwSetWindowAttrib(m_WindowHandle, GLFW_RESIZABLE, m_Specification.WindowResizeable ? GLFW_TRUE : GLFW_FALSE);
		}
		
		// Set icon
		GLFWimage icon;
		int channels;
		if (!m_Specification.IconPath.empty())
		{
			std::string iconPathStr = m_Specification.IconPath.string();
			icon.pixels = stbi_load(iconPathStr.c_str(), &icon.width, &icon.height, &channels, 4);
			glfwSetWindowIcon(m_WindowHandle, 1, &icon);
			stbi_image_free(icon.pixels);
		}

		glfwSetWindowUserPointer(m_WindowHandle, this);
		glfwSetTitlebarHitTestCallback(m_WindowHandle, [](GLFWwindow* window, int x, int y, int* hit)
		{
			Application* app = (Application*)glfwGetWindowUserPointer(window);
			*hit = app->IsTitleBarHovered();
		});		

		// Create Framebuffers
		int w, h;
		glfwGetFramebufferSize(m_WindowHandle, &w, &h);
		

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		// Theme colors
		UI::SetHazelTheme();

		// Style
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(10.0f, 10.0f);
		style.FramePadding = ImVec2(8.0f, 6.0f);
		style.ItemSpacing = ImVec2(6.0f, 6.0f);
		style.ChildRounding = 6.0f;
		style.PopupRounding = 6.0f;
		style.FrameRounding = 6.0f;
		style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}


		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(m_WindowHandle, true);
		ImGui_ImplOpenGL3_Init(glsl_version);
		

		// Load default font
		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;
		ImFont* robotoFont = io.Fonts->AddFontFromMemoryTTF((void*)g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &fontConfig);
		s_Fonts["Default"] = robotoFont;
		s_Fonts["Bold"] = io.Fonts->AddFontFromMemoryTTF((void*)g_RobotoBold, sizeof(g_RobotoBold), 20.0f, &fontConfig);
		s_Fonts["Italic"] = io.Fonts->AddFontFromMemoryTTF((void*)g_RobotoItalic, sizeof(g_RobotoItalic), 20.0f, &fontConfig);
		io.FontDefault = robotoFont;


		// Load images
		{
			uint32_t w, h;
			void* data = Image::Decode(g_WalnutIcon, sizeof(g_WalnutIcon), w, h);
			m_AppHeaderIcon = std::make_shared<Walnut::Image>(m_Specification.IconPath.string());
			m_AppHeaderIconHov = std::make_shared<Walnut::Image>(m_Specification.HoveredIconPath.string());
			free(data);
		}
		{
			uint32_t w, h;
			void* data = Image::Decode(g_WindowMinimizeIcon, sizeof(g_WindowMinimizeIcon), w, h);
			m_IconMinimize = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
			free(data);
		}
		{
			uint32_t w, h;
			void* data = Image::Decode(g_WindowMaximizeIcon, sizeof(g_WindowMaximizeIcon), w, h);
			m_IconMaximize = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
			free(data);
		}
		{
			uint32_t w, h;
			void* data = Image::Decode(g_WindowRestoreIcon, sizeof(g_WindowRestoreIcon), w, h);
			m_IconRestore = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
			free(data);
		}
		{
			uint32_t w, h;
			void* data = Image::Decode(g_WindowCloseIcon, sizeof(g_WindowCloseIcon), w, h);
			m_IconClose = std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
			free(data);
		}

		//GLFWimage image[1];
		//image->pixels = stbi_load("ls.png", &image->width, &image->height, 0, 4);
		//glfwSetWindowIcon(m_WindowHandle, 1, image);
		//stbi_image_free(image->pixels);
	}

	void Application::Shutdown()
	{
		for (auto& layer : m_LayerStack)
			layer->OnDetach();

		m_LayerStack.clear();

		// Release resources
		// NOTE(Yan): to avoid doing this manually, we shouldn't
		//            store resources in this Application class
		m_AppHeaderIcon.reset();
		m_AppHeaderIconHov.reset();
		m_IconClose.reset();
		m_IconMinimize.reset();
		m_IconMaximize.reset();
		m_IconRestore.reset();

		// Cleanup
		

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(m_WindowHandle);
		glfwTerminate();

		g_ApplicationRunning = false;

		Log::Shutdown();
	}

	void Application::UI_DrawTitlebar(float& outTitlebarHeight)
	{
		const float titlebarHeight = 58.0f;
		const bool isMaximized = IsMaximized();
		float titlebarVerticalOffset = isMaximized ? -6.0f : 0.0f;
		const ImVec2 windowPadding = ImGui::GetCurrentWindow()->WindowPadding;

		ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));
		const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
		const ImVec2 titlebarMax = { ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f,
									 ImGui::GetCursorScreenPos().y + titlebarHeight };
		auto* bgDrawList = ImGui::GetBackgroundDrawList();
		auto* fgDrawList = ImGui::GetForegroundDrawList();
		
		float logoRectSpace;

		// Logo
		{
			const int logoWidth = 48;// m_LogoTex->GetWidth();
			const int logoHeight = 48;// m_LogoTex->GetHeight();
			const ImVec2 logoOffset(16.0f + windowPadding.x, 5.0f + windowPadding.y + titlebarVerticalOffset);
			const ImVec2 logoRectStart = { ImGui::GetItemRectMin().x + logoOffset.x, ImGui::GetItemRectMin().y + logoOffset.y };
			const ImVec2 logoRectMax = { logoRectStart.x + logoWidth, logoRectStart.y + logoHeight };
			logoRectSpace = logoRectMax.x - logoRectStart.x;

			static auto icon = m_AppHeaderIcon;
			fgDrawList->AddImage((ImTextureID)icon->GetRendererID(), logoRectStart, logoRectMax);
			if (ImGui::ItemHoverable({ logoRectStart , logoRectMax }, ImGui::GetActiveID()))
			{
				icon = m_AppHeaderIconHov;
				
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					m_Specification.FuncIconPressed();
				}
			}
			else
				icon = m_AppHeaderIcon;
		}
		
		bgDrawList->AddRectFilled(titlebarMin, titlebarMax, UI::Colors::Theme::titlebar);
		// DEBUG TITLEBAR BOUNDS
		// fgDrawList->AddRect(titlebarMin, titlebarMax, UI::Colors::Theme::invalidPrefab);


		ImGui::BeginHorizontal("Titlebar", { ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetFrameHeightWithSpacing() });

		static float moveOffsetX;
		static float moveOffsetY;
		const float w = ImGui::GetContentRegionAvail().x;
		const float buttonsAreaWidth = 94;

		// Title bar drag area
		// On Windows we hook into the GLFW win32 window internals
		ImGui::SetCursorPos(ImVec2(windowPadding.x + 2 * logoRectSpace, windowPadding.y + titlebarVerticalOffset)); // Reset cursor pos
		// DEBUG DRAG BOUNDS
		// fgDrawList->AddRect(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + w - buttonsAreaWidth, ImGui::GetCursorScreenPos().y + titlebarHeight), UI::Colors::Theme::invalidPrefab);
		ImGui::InvisibleButton("##titleBarDragZone", ImVec2(w - 2 * logoRectSpace- buttonsAreaWidth, titlebarHeight));

		m_TitleBarHovered = ImGui::IsItemHovered();

		if (isMaximized)
		{
			float windowMousePosY = ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y;
			if (windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
				m_TitleBarHovered = true; // Account for the top-most pixels which don't register
		}

		// Draw Menubar
		if (m_MenubarCallback)
		{
			ImGui::SuspendLayout();
			{
				ImGui::SetItemAllowOverlap();
				const float logoHorizontalOffset = 16.0f * 2.0f + 48.0f + windowPadding.x;
				ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, 6.0f + titlebarVerticalOffset));
				UI_DrawMenubar();

				if (ImGui::IsItemHovered())
					m_TitleBarHovered = false;
			}

			ImGui::ResumeLayout();
		}

		{
			// Centered Window title
			ImVec2 currentCursorPos = ImGui::GetCursorPos();
			ImVec2 textSize = ImGui::CalcTextSize(m_Specification.Name.c_str());
			ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - textSize.x * 0.5f, 2.0f + windowPadding.y + 6.0f));
			ImGui::Text("%s", m_Specification.Name.c_str()); // Draw title
			ImGui::SetCursorPos(currentCursorPos);
		}

		// Window buttons
		const ImU32 buttonColN = UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::text, 0.9f);
		const ImU32 buttonColH = UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::text, 1.2f);
		const ImU32 buttonColP = UI::Colors::Theme::textDarker;
		const float buttonWidth = 14.0f;
		const float buttonHeight = 14.0f;

		// Minimize Button

		ImGui::Spring();
		UI::ShiftCursorY(8.0f);
		{
			const int iconWidth = m_IconMinimize->GetWidth();
			const int iconHeight = m_IconMinimize->GetHeight();
			const float padY = (buttonHeight - (float)iconHeight) / 2.0f;
			if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight)))
			{
				// TODO: move this stuff to a better place, like Window class
				if (m_WindowHandle)
				{
					glfwIconifyWindow(m_WindowHandle);
				}
			}

			UI::DrawButtonImage(m_IconMinimize, buttonColN, buttonColH, buttonColP, UI::RectExpanded(UI::GetItemRect(), 0.0f, -padY));
		}


		// Maximize Button
		ImGui::Spring(-1.0f, 17.0f);
		UI::ShiftCursorY(8.0f);
		{
			const int iconWidth = m_IconMaximize->GetWidth();
			const int iconHeight = m_IconMaximize->GetHeight();

			const bool isMaximized = IsMaximized();

			if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight)))
			{
				if (isMaximized)
					glfwRestoreWindow(m_WindowHandle);
				else
					glfwMaximizeWindow(m_WindowHandle);
			}

			UI::DrawButtonImage(isMaximized ? m_IconRestore : m_IconMaximize, buttonColN, buttonColH, buttonColP);
		}

		// Close Button
		ImGui::Spring(-1.0f, 15.0f);
		UI::ShiftCursorY(8.0f);
		{
			const int iconWidth = m_IconClose->GetWidth();
			const int iconHeight = m_IconClose->GetHeight();
			if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight)))
				Application::Get().Close();
			UI::DrawButtonImage(m_IconClose, UI::Colors::Theme::text, UI::Colors::ColorWithMultipliedValue({255, 0, 0, 255}, 1), buttonColP);

		}

		ImGui::Spring(-1.0f, 18.0f);
		ImGui::EndHorizontal();

		outTitlebarHeight = titlebarHeight;
	}

	void Application::UI_DrawMenubar()
	{
		if (!m_MenubarCallback)
			return;

		if (m_Specification.CustomTitlebar)
		{
			const ImRect menuBarRect = { ImGui::GetCursorPos(), { ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing() } };

			ImGui::BeginGroup();
			if (UI::BeginMenubar(menuBarRect))
			{
				m_MenubarCallback();
			}

			UI::EndMenubar();
			ImGui::EndGroup();

		}
		else
		{
			if (ImGui::BeginMenuBar())
			{
				m_MenubarCallback();
				ImGui::EndMenuBar();
			}
		}
	}

	void Application::Run()
	{
		m_Running = true;

		EndCustomWindow();
		
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImGuiIO& io = ImGui::GetIO();

		glfwMakeContextCurrent(m_WindowHandle);

		glfwShowWindow(m_WindowHandle);

		// Main loop
		while (!glfwWindowShouldClose(m_WindowHandle) && m_Running)
		{
			// Poll and handle events (inputs, window resize, etc.)
			// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
			// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
			glfwPollEvents();

			for (auto& layer : m_LayerStack)
				layer->OnUpdate(m_TimeStep);

			// Resize swap chain?
			

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			{
				// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
				// because it would be confusing to have two docking targets within each others.
				ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

				ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->Pos);
				ImGui::SetNextWindowSize(viewport->Size);
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
				if (!m_Specification.CustomTitlebar && m_MenubarCallback)
					window_flags |= ImGuiWindowFlags_MenuBar;

				const bool isMaximized = IsMaximized();

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, isMaximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);

				ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
				ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
				ImGui::PopStyleColor(); // MenuBarBg
				ImGui::PopStyleVar(2);

				ImGui::PopStyleVar(2);

				{
					ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(50, 50, 50, 255));
					// Draw window border if the window is not maximized
					if (!isMaximized)
						UI::RenderWindowOuterBorders(ImGui::GetCurrentWindow());

					ImGui::PopStyleColor(); // ImGuiCol_Border
				}
				
				if (m_Specification.CustomTitlebar)
				{
					float titleBarHeight;
					UI_DrawTitlebar(titleBarHeight);
					ImGui::SetCursorPosY(titleBarHeight);

				}

				// Dockspace
				ImGuiIO& io = ImGui::GetIO();
				ImGuiStyle& style = ImGui::GetStyle();
				float minWinSizeX = style.WindowMinSize.x;
				style.WindowMinSize.x = m_minSize;
				ImGui::DockSpace(ImGui::GetID("MyDockspace"), ImVec2(0, 0), m_DockNodeFlag);
				style.WindowMinSize.x = minWinSizeX;

				if (!m_Specification.CustomTitlebar)
					UI_DrawMenubar();

				for (auto& layer : m_LayerStack)
					layer->OnUIRender();

				ImGui::End();
			}

			// Rendering
			ImGui::Render();
			ImDrawData* main_draw_data = ImGui::GetDrawData();
			const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
			
			int display_w, display_h;
			glfwGetFramebufferSize(m_WindowHandle, &display_w, &display_h);
			//glViewport(0, 0, display_w, display_h);
			//glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
			//glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(main_draw_data);

			// Update and Render additional Platform Windows
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

			// Present Main Platform Window
			//if (!main_is_minimized)
			//	FramePresent(wd);
			//else
			//	std::this_thread::sleep_for(std::chrono::milliseconds(5));

			if(main_is_minimized)
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			
			glfwSwapBuffers(m_WindowHandle);

			float time = GetTime();
			m_FrameTime = time - m_LastFrameTime;
			m_TimeStep = glm::min<float>(m_FrameTime, 0.0333f);
			m_LastFrameTime = time;
		}

	}

	void Application::Close()
	{
		m_Running = false;
	}

	bool Application::IsMaximized() const
	{
		return (bool)glfwGetWindowAttrib(m_WindowHandle, GLFW_MAXIMIZED);
	}

	float Application::GetTime()
	{
		return (float)glfwGetTime();
	}

	ImFont* Application::GetFont(const std::string& name)
	{
		if (!s_Fonts.contains(name))
			return nullptr;

		return s_Fonts.at(name);
	}

	void Application::SetMinImGuiWindowSize(float minSize)
	{
		m_minSize = minSize;
	}

	void Application::SetDockNodeFlags(ImGuiDockNodeFlags flags)
	{
		m_DockNodeFlag = flags;
	}

	void Application::StartCustomWindow()
	{
#if 0
		m_customThread = new std::thread(
			[this]()
			{
				if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
					//printf("error initializing SDL: %s\n", SDL_GetError());
				}
				SDL_Window* win = SDL_CreateWindow("Start",
					SDL_WINDOWPOS_CENTERED,
					SDL_WINDOWPOS_CENTERED,
					1000, 1000, 0);

				// triggers the program that controls
	// your graphics hardware and sets flags
				Uint32 render_flags = SDL_RENDERER_ACCELERATED;

				// creates a renderer to render our images
				SDL_Renderer* rend = SDL_CreateRenderer(win, -1, render_flags);

				// creates a surface to load an image into the main memory
				SDL_Surface* surface;

				// please provide a path for your image
				surface = IMG_Load("ls.png");

				// loads image to our graphics hardware memory.
				SDL_Texture* tex = SDL_CreateTextureFromSurface(rend, surface);

				// clears main-memory
				SDL_FreeSurface(surface);

				// let us control our image position
				// so that we can move it with our keyboard.
				SDL_Rect dest;

				// connects our texture with dest to control position
				SDL_QueryTexture(tex, NULL, NULL, &dest.w, &dest.h);


				// animation loop
				while (!close) {

					// clears the screen
					SDL_RenderClear(rend);
					SDL_RenderCopy(rend, tex, NULL, &dest);

					// triggers the double buffers
					// for multiple rendering
					SDL_RenderPresent(rend);

					// calculates to 60 fps
					SDL_Delay(1000 / 60);
				}

				// destroy texture
				SDL_DestroyTexture(tex);

				// destroy renderer
				SDL_DestroyRenderer(rend);

				// destroy window
				SDL_DestroyWindow(win);

				// close SDL
				SDL_Quit();
			}
		);
#endif
	}

	void Application::EndCustomWindow()
	{
		m_customStartUpEnd = true;
		if (m_customThread)
			m_customThread->join();
		m_customThread = nullptr;
	}
}
