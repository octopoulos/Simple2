// Camera.h
// @author octopoulos
// @version 2025-10-15

#pragma once

#include "objects/Object3d.h"

enum CameraActions_ : int
{
	CameraAction_None  = 0,
	CameraAction_Orbit = 1,
	CameraAction_Pan   = 2
};

enum CameraDirs : int
{
	CameraDir_Forward = 0,
	CameraDir_Right   = 1,
	CameraDir_Up      = 2,
};

enum CameraFollows : int
{
	CameraFollow_None        = 0, ///
	CameraFollow_Active      = 1, ///< follow cursor/selObj, otherwise free look
	CameraFollow_Cursor      = 2, ///< follow the cursor
	CameraFollow_SelectedObj = 4, ///< follow the selected object
};

using sCamera = std::shared_ptr<class Camera>;

class Camera : public Object3d
{
private:
	int      action      = CameraAction_None;    ///< CameraActions_
	float    angleDecay  = 0.997f;               ///< angular decay
	float    angleVel[2] = { 0.0f, 0.0f };       ///< angular velocity
	float    aspect      = 1.0f;                 ///< aspect ratio (width / height)
	float    farPlane    = 100.0f;               ///< far clipping plane (perspective)
	float    fovY        = 60.0f;                ///< vertical field of view
	float    nearPlane   = 0.1f;                 ///< near clipping plane (perspective)
	float    orbit[2]    = { 0.0f, 0.0f };       ///< [azimuth, elevation]
	float    orbit0[2]   = { 0.0f, 0.0f };       ///< previous orbit values for orbiting speed
	bx::Vec3 panVel      = { 0.0f, 0.0f, 0.0f }; ///< panning velocity (right, up)
	bx::Vec3 pos0        = { 0.0f, 0.0f, 0.0f }; ///< previous position for panning speed

public:
	float    distance = 0.1f;                  ///< current distance to use between pos and target
	int      follow   = CameraFollow_Cursor;   ///< CameraFollows
	bx::Vec3 forward  = { 0.0f, 0.0f, 1.0f };  ///< forward dir (current)
	bx::Vec3 forward2 = { 0.0f, 0.0f, 1.0f };  ///< forward dir (wanted)
	bx::Vec3 pos      = { 0.0f, 0.0f, -3.0f }; ///< position: current
	bx::Vec3 pos2     = { 0.0f, 0.0f, -3.0f }; ///< position: destination
	bx::Vec3 right    = { 1.0f, 0.0f, 0.0f };  ///< right dir
	bx::Vec3 target   = bx::InitZero;          ///< target: current (look at)
	bx::Vec3 target2  = bx::InitZero;          ///< target: destination
	bx::Vec3 up       = { 0.0f, 1.0f, 0.0f };  ///< up dir
	bx::Vec3 worldUp  = { 0.0f, 1.0f, 0.0f };  ///< world up dir

	Camera(std::string_view name)
	    : Object3d(name, ObjectType_Camera)
	{
		Initialize();
	}

	~Camera() = default;

	/// Reset position and orientation
	void Initialize();

	/// Apply orbit deltas over time
	void ConsumeOrbit(float amount);

	/// Compute view + projection matrices
	void GetViewProjection(float fscreenX, float fscreenY, float* outView, float* outProj) const;

	/// Compute look-at view matrix
	void GetViewMatrix(float* viewMtx) const;

	/// Move the camera to a direction
	void Move(int cameraDir, float speed, bool resetActive = true);

	/// Accumulate orbit deltas
	void Orbit(float dx, float dy);

	/// Rotate around given axis
	void RotateAroundAxis(const bx::Vec3& axis, float angle);

	/// Serialize for JSON output
	virtual int Serialize(std::string& outString, int depth, int bounds = 3, bool addChildren = true) const override;

	/// Enable orthographic projection
	void SetOrthographic(const bx::Vec3& axis);

	/// Get an Object3d as a Camera
	static sCamera SharedPtr(const sObject3d& object)
	{
		return (object && (object->type & ObjectType_Camera)) ? std::static_pointer_cast<Camera>(object) : nullptr;
	}

	/// Show info table in ImGui
	void ShowInfoTable(bool showTitle = true) const override;

	/// Update smoothed camera state
	void Update(float delta);

	/// Update bgfx projection after computing view + projection matrices
	void UpdateViewProjection(uint8_t viewId, float fscreenX, float fscreenY);

	/// Adjust zoom + distance
	void Zoom(float ratio = 1.0f);

	/// Zoom by a signed amount
	void ZoomSigned(float amount);
};
