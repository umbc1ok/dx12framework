#include "Camera.h"

#include <algorithm>
#include <iostream>

#include "Component.h"
#include "Input.h"
#include "Window.h"

class Entity;

void Camera::update()
{
    Component::update();
    if(!Window::get_cursor_visible())
    {
        handle_input();
    }
}

void Camera::create()
{
    m_main_camera = new Camera();
    m_main_camera->set_can_tick(true);
}

hlsl::float4x4 Camera::get_view_matrix() const
{
    hlsl::float3 const position = entity->transform->get_position();
    hlsl::float3 const up = entity->transform->get_up();
    hlsl::float3 const forward = entity->transform->get_forward();
    return hlsl::lookAt(position, position + forward, up);
}

hlsl::float4x4 Camera::get_projection_matrix()
{
    update_internals();

    return m_projection;
}

void Camera::update_internals()
{
    if (m_dirty)
    {
        // HACK: Farplane and nearplane are switched, otherwise depth test was behaving oppositely
        m_projection = hlsl::perspective(fov, width / height, far_plane, near_plane);
    }
}

void Camera::handle_input()
{
    auto kb = Input::get_instance()->m_keyboard->GetState();
    auto mouse = Input::get_instance()->m_mouse;
    auto mouse_state = Input::get_instance()->m_mouse->GetState();
    hlsl::float2 mouse_delta = Input::get_instance()->get_mouse_delta();
    auto position = entity->transform->get_position();
    auto rotation = entity->transform->get_euler_angles();

    if(mouse_state.scrollWheelValue > 0)
        m_movement_speed += 0.01f;
    else if (mouse_state.scrollWheelValue < 0)
        m_movement_speed -= 0.01f;

    if (m_movement_speed < 0.01f)
        m_movement_speed = 0.01f;
    mouse->ResetScrollWheelValue();

    if (kb.W)
    {
        entity->transform->set_local_position(position + entity->transform->get_forward() * m_movement_speed);
    }
    if (kb.S)
    {
        entity->transform->set_local_position(position - entity->transform->get_forward() * m_movement_speed);
    }
    if (kb.A)
    {
        entity->transform->set_local_position(position - entity->transform->get_right() * m_movement_speed);
    }
    if (kb.D)
    {
        entity->transform->set_local_position(position + entity->transform->get_right() * m_movement_speed);
    }
    if (kb.E)
    {
        entity->transform->set_local_position(position + entity->transform->get_up() * m_movement_speed);
    }
    if (kb.Q)
    {
        entity->transform->set_local_position(position - entity->transform->get_up() * m_movement_speed);
    }

    {
        mouse_delta.y = -mouse_delta.y;

        mouse_delta *= 0.3f;

        m_yaw += mouse_delta.x;
        m_pitch = std::clamp(m_pitch + mouse_delta.y, -89.0f, 89.0f);

        entity->transform->set_local_euler_angles(hlsl::float3(m_pitch, -m_yaw, 0.0f));

    }
    update_internals();
}

