#include "VkApp.h"
#include "Window.h"
#include <iostream>

INT WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, INT)
{
    // Initialize the instance
    VkApp app;

    // Handle window messages to the end
    MSG msg;
    auto window = app.GetWindow();
    while (GetMessageW(&msg, window->GetHandle(), 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        if (window->Closed())
            break;
    }

    return 0;
}

