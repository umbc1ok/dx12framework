#pragma once

#include "Keyboard.h"
// DO NOT DELETE THIS INCLUDE
// It contains a #define that is needed for m_mouse->SetWindow();
#include "Windows.h"
#include "Mouse.h"
#include "utils/maths.h"

class Input
{
public:
    Input();
    ~Input() = default;
    static void create();
    static Input* get_instance() { return m_instance; }
    hlsl::float2 get_mouse_delta() const { return m_mouse_delta; }
    bool get_key_pressed();
    void update();
    DirectX::Keyboard* m_keyboard;
    DirectX::Mouse* m_mouse;
private:
    hlsl::float2 m_mouse_position;
    hlsl::float2 m_mouse_delta;
    static Input* m_instance;
};

