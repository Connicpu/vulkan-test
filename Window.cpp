#include "Window.h"

#define WINDOW_CLASS (L"CnnrsVulkanRendererWindow")

Window::Window()
    : closed(false)
{
    hinst = (HINSTANCE)GetModuleHandleW(nullptr);

    WNDCLASSEXW wndc = { sizeof(wndc) };
    wndc.lpszClassName = WINDOW_CLASS;
    wndc.cbWndExtra = sizeof(Window *);
    wndc.lpfnWndProc = WinProc;
    RegisterClassExW(&wndc);

    hwnd = CreateWindowExW(
        0U, WINDOW_CLASS, L"Cnnr's Vulkan Renderer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hinst, this
    );

    ShowWindow(hwnd, SW_SHOW);
}

Window::~Window()
{
    UnregisterClassW(WINDOW_CLASS, hinst);
}

HWND Window::GetHandle()
{
    return hwnd;
}

HINSTANCE Window::GetHInst()
{
    return hinst;
}

bool Window::Closed()
{
    return closed;
}

void Window::SetHandler(UINT msg, std::function<WndCallback> &&callback)
{
    callbacks.emplace(msg, std::move(callback));
}

LRESULT WINAPI Window::WinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            auto createInfo = (CREATESTRUCTW *)lp;
            auto window = (LONG_PTR)createInfo->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, window);
            return 0;
        }

        case WM_CLOSE:
        {
            auto window = (Window *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            window->closed = true;
            return 0;
        }

        default:
        {
            auto window = (Window *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            if (window)
            {
                auto cb = window->callbacks.find(msg);
                if (cb != window->callbacks.end())
                {
                    return cb->second(hwnd, msg, wp, lp);
                }
            }

            return DefWindowProcW(hwnd, msg, wp, lp);
        }
    }
}
