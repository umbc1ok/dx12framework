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
    handleInput();
    updateFrustum();
}

void Camera::create()
{
    m_main_camera = new Camera();
    m_main_camera->set_can_tick(true);
}

hlsl::float4x4 Camera::getViewMatrix() const
{
    hlsl::float3 const position = entity->transform->get_position();
    hlsl::float3 const up = entity->transform->get_up();
    hlsl::float3 const forward = entity->transform->get_forward();
    return hlsl::lookAt(position, position + forward, up);
}

hlsl::float4x4 Camera::getProjectionMatrix()
{
    updateInternals();

    return m_projection;
}

hlsl::float3 Camera::getCullingPosition()
{
    if (!m_frozen)
        m_cachedPosition = entity->transform->get_position();

    return m_cachedPosition;
}

void Camera::updateInternals()
{
    if (m_dirty)
    {
        // HACK: Farplane and nearplane are switched, otherwise depth test was behaving oppositely
        m_projection = hlsl::perspective(m_fov, m_width / m_height, m_farPlane, m_nearPlane);
    }
}

void Camera::handleInput()
{
    auto kb = Input::getInstance()->m_keyboard->GetState();
    auto mouse = Input::getInstance()->m_mouse;
    auto mouse_state = Input::getInstance()->m_mouse->GetState();
    hlsl::float2 mouse_delta = Input::getInstance()->getMouseDelta();
    auto position = entity->transform->get_position();
    if(m_RMBPressed != mouse_state.rightButton)
    {
        m_RMBPressed = mouse_state.rightButton;
        Window::get_instance()->change_mouse_mode(!mouse_state.rightButton);
    }

    if(m_RMBPressed)
    {
        if(mouse_state.scrollWheelValue > 0)
            m_movementSpeed += 0.01f;
        else if (mouse_state.scrollWheelValue < 0)
            m_movementSpeed -= 0.01f;

        if (m_movementSpeed < 0.01f)
            m_movementSpeed = 0.01f;
        mouse->ResetScrollWheelValue();

        if (kb.W)
        {
            entity->transform->set_local_position(position + entity->transform->get_forward() * m_movementSpeed);
        }
        if (kb.S)
        {
            entity->transform->set_local_position(position - entity->transform->get_forward() * m_movementSpeed);
        }
        if (kb.A)
        {
            entity->transform->set_local_position(position - entity->transform->get_right() * m_movementSpeed);
        }
        if (kb.D)
        {
            entity->transform->set_local_position(position + entity->transform->get_right() * m_movementSpeed);
        }
        if (kb.E)
        {
            entity->transform->set_local_position(position + entity->transform->get_up() * m_movementSpeed);
        }
        if (kb.Q)
        {
            entity->transform->set_local_position(position - entity->transform->get_up() * m_movementSpeed);
        }

        {
            mouse_delta.y = -mouse_delta.y;

            mouse_delta *= 0.3f;

            m_yaw += mouse_delta.x;
            m_pitch = std::clamp(m_pitch + mouse_delta.y, -89.0f, 89.0f);

            entity->transform->set_local_euler_angles(hlsl::float3(m_pitch, -m_yaw, 0.0f));

        }
        updateInternals();
    }
}

void Camera::updateFrustum()
{

    if(!m_frozen)
    {
        hlsl::float4x4 comboMatrix = m_projection * getViewMatrix();
        //comboMatrix = hlsl::transpose(comboMatrix);

        // Left clipping plane
        m_frustum.left_plane = comboMatrix.row(3) + comboMatrix.row(0);
        // Right clipping plane
        m_frustum.right_plane = comboMatrix.row(3) - comboMatrix.row(0);
        // Bottom clipping plane
        m_frustum.bottom_plane = comboMatrix.row(3) + comboMatrix.row(1);
        // Top clipping plane
        m_frustum.top_plane = comboMatrix.row(3) - comboMatrix.row(1);
        // Near clipping plane
        m_frustum.near_plane = comboMatrix.row(2);
        // Far clipping plane
        m_frustum.far_plane = comboMatrix.row(3) - comboMatrix.row(2);
    }
}

