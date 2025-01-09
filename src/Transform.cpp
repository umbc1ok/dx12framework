#include "Transform.h"

#include <ranges>
#include <cassert>

Transform::Transform(Entity* const& t_entity)
    : entity(t_entity)
{
}


void Transform::set_parent(Transform* const& new_parent)
{
    if (new_parent == nullptr)
    {
        if (parent == nullptr)
            return;

        parent->remove_child(this);
        m_local_dirty = true;
        return;
    }

    if (parent != nullptr)
    {
        parent->remove_child(this);
    }

    new_parent->add_child(this);
    m_local_dirty = true;
}

void Transform::set_position(hlsl::float3 const& position)
{
    if (parent == nullptr)
    {
        m_local_position = position;
    }
    else
    {
        hlsl::float3 const parent_global_position = parent->get_position();
        hlsl::float3 new_local_position = position - parent_global_position;

        auto inversed_rotation_matrix = hlsl::inverse(hlsl::rotation(parent->get_rotation()));
        new_local_position = (hlsl::float4(position, 1.0f) * inversed_rotation_matrix).xyz;

        m_local_position = new_local_position;
    }

    set_dirty();
}

void Transform::set_dirty()
{
    if (!m_local_dirty)
    {
        for (auto const& child : children)
        {
            child->set_parent_dirty();
        }
    }

    m_local_dirty = true;

}

void Transform::set_parent_dirty()
{
    if (!m_parent_dirty)
    {
        for (auto const& child : children)
        {
            child->set_parent_dirty();
        }
    }

    m_parent_dirty = true;
}

hlsl::float3 Transform::get_position()
{
    recompute_model_matrix_if_needed();

    return m_position;
}

// Set rotation - euler angles are in degrees
void Transform::set_rotation(hlsl::float3 const& euler_angles)
{
    if (parent == nullptr)
    {
        m_local_quat_rotation = hlsl::float4(hlsl::EulerToQuaternion(euler_angles * hlsl::DEG2RAD));
        m_local_euler_angles = euler_angles;
        m_quat_rotation = m_local_quat_rotation;
    }
    else
    {
        // Calculate the global rotation quaternion from the given euler angles
        hlsl::float4 const global_rotation = hlsl::float4(hlsl::EulerToQuaternion(euler_angles * hlsl::DEG2RAD));

        // Calculate the new local rotation by inverse of parent's rotation
        auto inversed_rotation_matrix = hlsl::inverse(hlsl::rotation(parent->get_rotation()));
        m_local_quat_rotation = inversed_rotation_matrix * global_rotation;

        // Convert the local rotation quaternion back to Euler angles for storage
        m_local_euler_angles = hlsl::QuaternionToEuler(m_local_quat_rotation) * hlsl::RAD2DEG;
    }

    set_dirty();
}

hlsl::float4 Transform::get_rotation()
{
    recompute_model_matrix_if_needed();

    return m_quat_rotation;
}

void Transform::set_scale(hlsl::float3 const& scale)
{
    if (parent == nullptr) // If there's no parent, global scale is the same as local scale
    {
        m_local_scale = scale;
    }
    else
    {
        hlsl::float3 const parent_global_scale = parent->get_scale();
        hlsl::float3 const new_local_scale = scale / parent_global_scale;

        m_local_scale = new_local_scale;
    }

    set_dirty();
}

hlsl::float3 Transform::get_scale()
{
    recompute_model_matrix_if_needed();

    return m_scale;
}

void Transform::set_local_position(hlsl::float3 const& position)
{
    m_local_position = position;

    set_dirty();
}

hlsl::float3 Transform::get_local_position() const
{
    return m_local_position;
}

void Transform::set_local_scale(hlsl::float3 const& scale)
{
    m_local_scale = scale;

    set_dirty();
}

hlsl::float3 Transform::get_local_scale() const
{
    return m_local_scale;
}

void Transform::set_local_euler_angles(hlsl::float3 const& euler_angles)
{
    
    auto const is_rotation_modified = hlsl::epsilonNotEqual(euler_angles, m_local_euler_angles, 0.0001f);
    if (!is_rotation_modified[0] && !is_rotation_modified[1] && !is_rotation_modified[2])
    {
        return;
    }
    
    m_local_euler_angles = euler_angles;
    m_local_quat_rotation = hlsl::float4(hlsl::EulerToQuaternion(euler_angles * hlsl::DEG2RAD));

    set_dirty();
}

hlsl::float3 Transform::get_euler_angles() const
{
    return m_local_euler_angles;
}

void Transform::orient_towards(hlsl::float3 const& target, hlsl::float3 const& up)
{
    hlsl::float4x4 transformation = hlsl::lookAt(get_position(), target, hlsl::float3(0.0f, 1.0f, 0.0f));
    transformation = hlsl::inverse(transformation);
    auto decomposed_rotation = hlsl::float4(1.0f);
    auto decomposed_translation = hlsl::float3(1.0f);
    auto decomposed_scale = hlsl::float3(1.0f);
    hlsl::DecomposeMatrix(transformation, decomposed_translation, decomposed_rotation, decomposed_scale);
    m_local_quat_rotation = decomposed_rotation;
    m_local_euler_angles = hlsl::QuaternionToEuler(m_local_quat_rotation) * hlsl::RAD2DEG;

    set_dirty();
}

hlsl::float3 Transform::get_forward()
{
    recompute_forward_right_up_if_needed();
    return m_forward;
}

void Transform::recompute_forward_right_up_if_needed()
{

    auto epsilon_bool_vec = hlsl::epsilonNotEqual(m_local_euler_angles_cached, get_euler_angles(), 0.0001f);
    if (!epsilon_bool_vec[0] && !epsilon_bool_vec[1] && !epsilon_bool_vec[2])
    {
        return;
    }

    auto const euler_angles = get_euler_angles();
    m_local_euler_angles_cached = euler_angles;

    auto direction_forward = hlsl::float3(0.0f, 0.0f, -1.0f);

    direction_forward = (hlsl::float4(direction_forward, 0.0f) * hlsl::transpose(hlsl::rotation(get_rotation()))).xyz;
    m_forward = hlsl::normalizeSafe(direction_forward);
    m_right = hlsl::normalizeSafe(hlsl::cross(m_forward, m_world_up));
    m_up = hlsl::normalizeSafe(hlsl::cross(m_right, m_forward));
}

hlsl::float3 Transform::get_right()
{
    recompute_forward_right_up_if_needed();
    return m_right;
}

hlsl::float3 Transform::get_up()
{
    recompute_forward_right_up_if_needed();
    return m_up;
}

hlsl::float4x4 Transform::get_model_matrix()
{
    recompute_model_matrix_if_needed();

    return m_model_matrix;
}

void Transform::add_child(Transform* const& transform)
{
    children.emplace_back(transform);
    transform->parent = this;
}

void Transform::recompute_model_matrix_if_needed()
{
    if (m_local_dirty || m_parent_dirty)
    {
        if (parent == nullptr)
        {
            compute_model_matrix();
        }
        else
            compute_model_matrix(parent->get_model_matrix());

        hlsl::DecomposeMatrix(m_model_matrix, m_position, m_quat_rotation, m_scale);
    }
}

void Transform::compute_model_matrix()
{
    assert(parent == nullptr);

    assert(m_local_dirty || m_parent_dirty);

    m_model_matrix = get_local_model_matrix();

    m_parent_dirty = false;
}

void Transform::compute_model_matrix(hlsl::float4x4 const& parent_global_model_matrix)
{
    assert(m_local_dirty || m_parent_dirty);

    // TODO: It's the order I took from Engine, which is column major. Check if it works here too.
    // I believe it should be the other way round (just as with PVM matrix)
    m_model_matrix = parent_global_model_matrix * get_local_model_matrix();

    m_parent_dirty = false;
}

hlsl::float4x4 Transform::get_local_model_matrix()
{
    if (!m_local_dirty)
        return m_local_model_matrix;

    compute_local_model_matrix();
    return m_local_model_matrix;
}

void Transform::compute_local_model_matrix()
{
    m_local_model_matrix = hlsl::ComposeMatrix(m_local_position, m_local_quat_rotation, m_local_scale);
    m_local_dirty = false;
}

void Transform::remove_child(Transform* const& transform)
{
    assert(transform->parent == this);

    auto const it = std::ranges::find(children, transform);

    if (it == children.end())
        return;

    children.erase(it);

    transform->parent = nullptr;
}
