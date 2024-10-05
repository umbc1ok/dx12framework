#include "Input.h"

#include "Camera.h"
#include "Window.h"


Input* Input::m_instance;
Input::Input()
{
    m_keyboard = new DirectX::Keyboard();
    m_mouse = new DirectX::Mouse();
    m_mouse->SetWindow(Window::get_hwnd());
    m_mouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);
}

void Input::create()
{
    m_instance = new Input();
}

void Input::update()
{
    auto mouse_state = m_mouse->GetState();
    m_mouse_delta = hlsl::float2(mouse_state.x, mouse_state.y) - m_mouse_position;
    m_mouse_position = hlsl::float2(mouse_state.x, mouse_state.y);
}
