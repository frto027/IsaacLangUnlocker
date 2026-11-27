#include "imgui_driver.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"

#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>
#include <tchar.h>
#include <stdio.h>
#include <functional>
#include <string>
#include "Shlwapi.h"
#include "../lang.h"
// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC            g_hRC;          // Rendering context
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;


// Forward declarations of helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void TryAddFont(const char * fontPath) {
    if (PathFileExistsA(fontPath)) {
        ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath);
    }
}


bool imguiWindowCreated = false;
HWND hwnd;
WNDCLASSEXW wc;
void EnsureImGuiWindow() {
	if (imguiWindowCreated) {
		return;
	}
	imguiWindowCreated = true;
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"LanguageUnlocker", nullptr };
    ::RegisterClassExW(&wc);
    hwnd = ::CreateWindowW(wc.lpszClassName, L"Language Unlocker by Frto027", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize OpenGL
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        //TODO: message box error.
        imguiWindowCreated = false;
        return ;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls

    io.IniFilename = NULL;
    io.LogFilename = NULL;

    TryAddFont("C:\\Windows\\Fonts\\msyh.ttc");

    // Setup Dear ImGui style
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    //style.FontSizeBase = 16.0f;           // Default to 13.0f when unspecified, which allows crisp legacy font for non-4K screens.
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;

    // Setup scaling
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
}
void ExitImguiWindow() {
    if (imguiWindowCreated) {
        imguiWindowCreated = false;

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceWGL(hwnd, &g_MainWindow);
        wglDeleteContext(g_hRC);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }
}
bool window_closed = false;
void DrawImGuiWindowUntil(const char * windowName/* not displayed, don't translate */, std::function<bool(void)> callback) {
    EnsureImGuiWindow();
    if (!imguiWindowCreated)
        //TODO: log the error
        return;
    // Main loop
    bool done = false;
    bool displayOpenSourceLicense = false;
    while (!done)
    {
        if (window_closed)
            break;
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if (::IsIconic(hwnd))
        {
            ::Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        ImGui::Begin(windowName, NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
        if (callback()) {
            done = true;
        }

        ImGui::SeparatorText(T(u8"版权声明", u8"Copyright"));
        ImGui::TextWrapped(T(u8"中文资源维护：X.P.87\n二进制补丁制作：Frto027\n感谢您的使用"),u8"LangHackRep+ is created by Frto027.");
        ImGui::SameLine();
        if (ImGui::Button(T(u8"开源许可协议", u8"Open Source License"))) {
            displayOpenSourceLicense = true;
        }

        ImGui::End();

        if (displayOpenSourceLicense) {
            if (ImGui::Begin("open-source-license", &displayOpenSourceLicense)) {
                ImGui::SetWindowFocus();
                ImGui::SeparatorText(T(u8"中文补丁", u8"LangHackRep+"));
                ImGui::TextWrapped(T(u8"中文补丁为开源程序，我们未授权任何第三方转载。", u8"LangHackRep+ is open source program."));
                if (getLang() == LANG_CN) {
                    ImGui::TextWrapped("");
                }

                ImGui::SeparatorText("Zydis");
                ImGui::TextWrapped(T(u8"MIT许可协议 补丁使用Zydis开发了一个高智商但不太聪明的内存动态补丁", u8"MIT License"));

                ImGui::SeparatorText("ImGUI");
                ImGui::TextWrapped(T(u8"MIT许可协议 你正在看到的界面，就是ImGUI的杰出效果！", u8"MIT License"));
            }
            ImGui::End();

        }
        // Rendering
        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Present
        ::SwapBuffers(g_MainWindow.hDC);
    }

};

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (g_hRC)
        return true;

    // Create legacy context
    HGLRC tempRC = wglCreateContext(data->hDC);
    if (!tempRC) { ::ReleaseDC(hWnd, data->hDC); return false; }
    if (!wglMakeCurrent(data->hDC, tempRC))
    {
        wglDeleteContext(tempRC);
        ::ReleaseDC(hWnd, data->hDC);
        return false;
    }

    // Get context version
    GLint major = 0, minor = 0;
    glGetIntegerv(0x821B, &major); // GL_MAJOR_VERSION
    glGetIntegerv(0x821C, &minor); // GL_MINOR_VERSION
    const char* gl_version_str = (const char*)glGetString(GL_VERSION);
    if (major == 0 && minor == 0)
        sscanf_s(gl_version_str, "%d.%d", &major, &minor); // Query GL_VERSION in desktop GL 2.x, the string will start with "<major>.<minor>"
    const GLuint gl_version = (GLuint)(major * 100 + minor * 10);

    // Keep temporary context: already OpenGL 3.0+.
    g_hRC = tempRC;
    if (gl_version >= 300)
        return true;

    typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
    const PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

    // GL 3.0
    const int attribs[] =
    {
        0x2091, 3,      // WGL_CONTEXT_MAJOR_VERSION_ARB
        0x2092, 0,      // WGL_CONTEXT_MINOR_VERSION_ARB
        0x9126, 0x0001, // WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB
        0
    };
    HGLRC newRC = nullptr;
    if (wglCreateContextAttribsARB)
        newRC = wglCreateContextAttribsARB(data->hDC, 0, attribs);

    // If we managed to create 3.0+ context: use that one and destroy the temporary OpenGL 2.x compatibility context.
    if (newRC)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(tempRC);
        g_hRC = newRC;
        wglMakeCurrent(data->hDC, g_hRC);
    }

    return true;
}
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_Width = LOWORD(lParam);
            g_Height = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        window_closed = true;
        exit(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

