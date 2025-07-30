// Camera.h
// @author octopoulos
// @version 2025-07-26

#pragma once

#include "objects/Object3d.h"

class Camera : public Object3d
{
private:
	float    aspect    = 1.0f;                  ///
	bx::Vec3 at        = { 0.0f, 0.0f, 1.0f };  ///
	bx::Vec3 eye       = { 0.0f, 0.5f, 0.0f };  ///
	float    farPlane  = 100.0f;                ///
	float    fovY      = 60.0f;                 ///
	float    nearPlane = 0.1f;                  ///
	float    orbit[2]  = {};                    ///
	bx::Vec3 pos       = { 0.0f, 0.0f, -3.0f }; ///< position: current
	bx::Vec3 target    = bx::InitZero;          ///< target: current

public:
	bx::Vec3 forward = { 0.0f, 0.0f, 1.0f };  ///< forward dir
	bx::Vec3 pos2    = { 0.0f, 0.0f, -3.0f }; ///< position: destination
	bx::Vec3 right   = { 1.0f, 0.0f, 0.0f };  ///< right dir
	bx::Vec3 target2 = bx::InitZero;          ///< target: destination
	bx::Vec3 up      = { 0.0f, 1.0f, 0.0f };  ///< up dir

	Camera()
	{
		type = ObjectType_Camera;
		Initialize();
	}

	~Camera() = default;

	void Initialize();

	void ConsumeOrbit(float amount);

	void GetViewMatrix(float* viewMtx);

	void Orbit(float dx, float dy);

	void Update(float delta);

	void UpdateViewProjection(uint8_t viewId, float fscreenX, float fscreenY);
};

using sCamera = std::shared_ptr<Camera>;
