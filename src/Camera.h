#pragma once
#include "Component.h"
#include <DirectXMath.h>

#include "Entity.h"

class Camera final : public Component
{
public:
	Camera() = default;
	~Camera() = default;

    void update() override;
	static void create();
	static Camera* get_main_camera() { return m_main_camera; };
	hlsl::float4x4 get_view_matrix() const;
	hlsl::float4x4 get_projection_matrix();
	void update_internals();
	void handle_input();

private:
	inline static Camera* m_main_camera;

	float m_movement_speed = 0.2f;
	hlsl::float4x4 m_projection;
	float width = 1920.0f;
	float height = 1080.0f;

	float fov = 90.0f;
	float m_yaw = 0.0f;
	float m_pitch = 10.0f;

	float near_plane = 0.1f;
	float far_plane = 1000.0f;

	bool m_dirty = true;
};

