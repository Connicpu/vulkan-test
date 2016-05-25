#pragma once

#include <Windows.h>
#include <functional>
#include <unordered_map>

typedef LRESULT (WndCallback)(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

class Window
{
public:
    Window();
    ~Window();

    HWND GetHandle();
    HINSTANCE GetHInst();
    bool Closed();

    void SetHandler(UINT msg, std::function<WndCallback> &&callback);

private:
    static LRESULT WINAPI WinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HINSTANCE hinst;
    HWND hwnd;
    bool closed;

    std::unordered_map<UINT, std::function<WndCallback>> callbacks;
};
