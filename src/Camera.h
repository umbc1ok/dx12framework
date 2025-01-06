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
	static Camera* getMainCamera() { return m_main_camera; };
	hlsl::float4x4 getViewMatrix() const;
	hlsl::float4x4 getProjectionMatrix();
	void updateInternals();
	void handleInput();

private:
	inline static Camera* m_main_camera;

	float m_movementSpeed = 0.05f;
	hlsl::float4x4 m_projection;
	float m_width = 1920.0f;
	float m_height = 1080.0f;

	float m_fov = 90.0f;
	float m_yaw = 0.0f;
	float m_pitch = 10.0f;

	float m_nearPlane = 0.1f;
	float m_farPlane = 1000.0f;

	bool m_dirty = true;
    bool m_RMBPressed = false;
};

