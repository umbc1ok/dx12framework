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
    static Input* getInstance() { return m_instance; }
    hlsl::float2 getMouseDelta() const { return m_mouseDelta; }
    bool getKeyPressed();
    void update();
    DirectX::Keyboard* m_keyboard;
    DirectX::Mouse* m_mouse;
private:
    hlsl::float2 m_mousePosition;
    hlsl::float2 m_mouseDelta;
    static Input* m_instance;
};

