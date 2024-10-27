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
    static bool get_cursor_visible() { return m_cursor_visible; }
    static void change_mouse_mode(bool cursor_visible);
private:
    static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static WNDCLASSEXW wc;
    static HWND hwnd;
    inline static bool m_cursor_visible = true;
    static Window* m_instance;

};
