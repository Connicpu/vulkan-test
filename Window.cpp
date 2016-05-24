#include "Window.h"

#define WINDOW_CLASS (L"CnnrsVulkanRenderer")

static LRESULT WINAPI WinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

Window::Window(HINSTANCE hinst)
    : hinst(hinst), closed(false)
{
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

bool Window::Closed()
{
    return closed;
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
            return DefWindowProcW(hwnd, msg, wp, lp);
    }
}