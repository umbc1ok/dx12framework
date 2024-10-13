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
    hlsl::float2 mouse_delta = Input::get_instance()->get_mouse_delta();
    auto position = entity->transform->get_position();
    auto rotation = entity->transform->get_euler_angles();
    float movement_speed = 0.2f;
    if (kb.W)
    {
        entity->transform->set_local_position(position + entity->transform->get_forward() * movement_speed);
    }
    if (kb.S)
    {
        entity->transform->set_local_position(position - entity->transform->get_forward() * movement_speed);
    }
    if (kb.A)
    {
        entity->transform->set_local_position(position - entity->transform->get_right() * movement_speed);
    }
    if (kb.D)
    {
        entity->transform->set_local_position(position + entity->transform->get_right() * movement_speed);
    }
    if (kb.E)
    {
        entity->transform->set_local_position(position + entity->transform->get_up() * movement_speed);
    }
    if (kb.Q)
    {
        entity->transform->set_local_position(position - entity->transform->get_up() * movement_speed);
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

