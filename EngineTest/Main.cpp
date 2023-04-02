#include "Test.h"
#pragma comment(lib, "engine.lib")

#if TEST_ENTITY_COMPONENTS
#include "TestEntityComponents.h"
#elif TEST_WINDOW
#include "TestWindowWin32.h"
#include "TestWindowLinux.h"
#elif TEST_RENDERER
#include "TestRendererWin32.h"
#include "TestRendererLinux.h"
#else
#error One of the tests must be enabled
#endif

#ifdef _WIN64
#include <Windows.h>
#include <filesystem>

// TODO: This is a duplicate
std::filesystem::path set_current_directory_to_executable_path()
{
    // set the working directory to the executable path
    wchar_t path[MAX_PATH];
    const uint32_t length{ GetModuleFileName(0, &path[0], MAX_PATH) };

    if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return {};

    std::filesystem::path p{ path };
    std::filesystem::current_path(p.parent_path());

    return std::filesystem::current_path();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#if _DEBUG
    // MSVC Debug flags that help check for memory leaks
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    set_current_directory_to_executable_path();
    engine_test test{};

    if (test.initialize())
    {
        MSG msg;
        bool is_running{ true };
        while (is_running)
        {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                is_running &= (msg.message != WM_QUIT);
            }
            test.run();
        }
        test.shutdown();
        return 0;
    }
}

#elif __linux__
#include "Platform/LinuxWindowManager.h"

int main(int argc, char* argv[])
{   
    using namespace primal;
    engine_test test{};
    
    if (!platform::display()) return 1;

	if (test.initialize())
	{
        platform::event ev;
        bool is_running{ true };
        
        while (is_running)
        {
            while(platform::pending_events())
            {
                platform::next_event(&ev);
                platform::dispatch_event(&ev);
                is_running = !platform::quit_msg_received(&ev);
            }
            test.run();
        }

        test.shutdown();
        platform::close_display();
        return 0;
	}
}
#endif // _WIN64