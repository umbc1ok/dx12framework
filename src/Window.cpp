#include "Window.h"
#include "imgui.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <iostream>
#include <tchar.h>
#include "imgui_internal.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Renderer.h"

// Forward declaration, for reason look into imgui_impl_win32.h
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
Window* Window::m_instance;
HWND Window::hwnd;
WNDCLASSEXW Window::wc;

void Window::update_window_name(std::string const& name)
{
    SetWindowText(hwnd, _T(name.c_str()));
}

LRESULT Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Renderer* renderer = Renderer::get_instance();
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (renderer != nullptr && renderer->get_device() != nullptr && wParam != SIZE_MINIMIZED)
        {
            renderer->on_window_resize();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        DirectX::Keyboard::ProcessMessage(msg, wParam, lParam);
        break;
    case WM_KEYUP:
        DirectX::Keyboard::ProcessMessage(msg, wParam, lParam);
        break;
    case WM_MOUSEMOVE:
        DirectX::Mouse::ProcessMessage(msg, wParam, lParam);
        break;
    case WM_MBUTTONDOWN:
        cursor_visible = !cursor_visible;
        change_mouse_mode();
        break;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void Window::change_mouse_mode()
{
    if (cursor_visible)
    {
        ::ShowCursor(true);
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }
    else
    {
        ::ShowCursor(false);
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
    }
}

void Window::create()
{
    m_instance = new Window();

    wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"DX12FRAMEWORK by Hubert Olejnik", nullptr };
    ::RegisterClassExW(&wc);

    hwnd = ::CreateWindowW(wc.lpszClassName, L"DX12FRAMEWORK by Hubert Olejnik", WS_OVERLAPPEDWINDOW, 0, 0, 1920, 1080, nullptr, nullptr, wc.hInstance, nullptr);


    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

}


