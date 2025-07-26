// Camera.h
// @author octopoulos
// @version 2025-07-22

#pragma once

#include "Object3d.h"

class Camera2 : public Object3d
{
public:
	float     aspect           = 1.0f;
	float     farPlane         = 100.0f;
	float     fovY             = 60.0f;
	float     nearPlane        = 0.1f;
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix       = glm::mat4(1.0f);

	Camera2()
	{
		type = ObjectType_Camera;
	}

	~Camera2() = default;

	void UpdateViewProjection(uint8_t viewId)
	{
		UpdateWorldMatrix();
		projectionMatrix = glm::perspective(glm::radians(fovY), aspect, nearPlane, farPlane);
		viewMatrix       = glm::inverse(worldMatrix);

		if (!bgfx::getCaps()->originBottomLeft)
			projectionMatrix[1][1] *= -1.0f;

		if (bgfx::getCaps()->homogeneousDepth)
		{
			glm::mat4 clipZRemap = glm::mat4(
			    1, 0, 0, 0,
			    0, 1, 0, 0,
			    0, 0, 0.5f, 0.5f,
			    0, 0, 0, 1);
			projectionMatrix = clipZRemap * projectionMatrix;
		}

		bgfx::setViewTransform(viewId, glm::value_ptr(viewMatrix), glm::value_ptr(projectionMatrix));
	}
};
