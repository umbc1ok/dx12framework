#pragma once

#include <vector>

#include "Component.h"
#include "utils/maths.h"

class Entity;

class Transform
{
public:
    explicit Transform(Entity* const& t_entity);
    ~Transform();

    void set_parent(Transform* const& new_parent);


    void set_position(hlsl::float3 const& position);
    void set_dirty();
    void set_parent_dirty();
    hlsl::float3 get_position();

    void set_rotation(hlsl::float3 const& euler_angles);
    hlsl::float4 get_rotation();

    void set_scale(hlsl::float3 const& scale);
    hlsl::float3 get_scale();

    void set_local_position(hlsl::float3 const& position);
    hlsl::float3 get_local_position() const;

    void set_local_scale(hlsl::float3 const& scale);
    hlsl::float3 get_local_scale() const;

    void set_local_euler_angles(hlsl::float3 const& euler_angles);
    hlsl::float3 get_euler_angles() const;

    // TODO: Implement
    hlsl::float3 get_euler_angles_restricted() const;

    void orient_towards(hlsl::float3 const& target, hlsl::float3 const& up = hlsl::float3(0.0f, 1.0f, 0.0f));

    hlsl::float3 get_forward();
    void recompute_forward_right_up_if_needed();
    hlsl::float3 get_right();
    hlsl::float3 get_up();

    hlsl::float4x4 const& get_model_matrix();

    hlsl::float4x4 get_local_model_matrix();
    void compute_local_model_matrix();
    void compute_model_matrix(hlsl::float4x4 const& parent_global_model_matrix);

    std::vector<Transform*> children;
    Transform* parent = {};
    Entity* entity = {};
private:
    void add_child(Transform* const& transform);
    void recompute_model_matrix_if_needed();
    void compute_model_matrix();
    void remove_child(Transform* const& transform);

    hlsl::float3 m_local_position = hlsl::float3(0.0f);
    hlsl::float3 m_local_euler_angles = hlsl::float3(0.0f);
    hlsl::float4 m_local_quat_rotation = hlsl::float4(1.0f, 0.0, 0.0f, 0.0f);
    hlsl::float3 m_local_scale = hlsl::float3(1.0f);

    hlsl::float3 m_position = {};
    hlsl::float4 m_quat_rotation = {};
    hlsl::float3 m_scale = {};

    hlsl::float3 m_forward = {};
    hlsl::float3 m_right = {};
    hlsl::float3 m_up = {};

    hlsl::float3 m_skew = {};
    hlsl::float4 m_perspective = {};

    hlsl::float4x4 m_model_matrix = hlsl::float4x4(1.0f);
    hlsl::float4x4 m_local_model_matrix = hlsl::float4x4(1.0f);


    hlsl::float3 m_world_up = hlsl::float3(0.0f, 1.0f, 0.0f);


    bool m_local_dirty = false;
    bool m_parent_dirty = false;
};
