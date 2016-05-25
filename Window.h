#pragma once

#include <Windows.h>

class Window
{
public:
    Window();
    ~Window();

    HWND GetHandle();
    HINSTANCE GetHInst();
    bool Closed();

private:
    static LRESULT WINAPI WinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HINSTANCE hinst;
    HWND hwnd;
    bool closed;
};
