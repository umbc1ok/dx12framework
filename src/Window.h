#pragma once
#include <d3d12.h>
#include <string>


class Window
{
public:
    static void create();

    static Window* get_instance()
    {
        return m_instance;
    }

    Window() = default;
    ~Window() = default;

    static HWND get_hwnd() { return hwnd; }
    static void update_window_name(std::string const& name);

private:
    static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static WNDCLASSEXW wc;
    static HWND hwnd;

    static Window* m_instance;

};
