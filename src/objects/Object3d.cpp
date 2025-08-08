// Object3d.cpp
// @author octopoulos
// @version 2025-08-04

#include "stdafx.h"
#include "objects/Object3d.h"

void Object3d::AddChild(sObject3d child)
{
	child->parent = this;
	children.push_back(std::move(child));
}

void Object3d::RemoveChild(const sObject3d& child)
{
	if (const auto& it = std::find(children.begin(), children.end(), child); it != children.end())
	{
		(*it)->parent = nullptr;
		children.erase(it);
	}
}

void Object3d::Render(uint8_t viewId, int renderFlags)
{
	if (!visible) return;

	if (type & ObjectType_Group)
	{
		for (const auto& child : children)
			child->Render(viewId, renderFlags);
	}
}

void Object3d::ScaleRotationPosition(const glm::vec3& _scale, const glm::vec3& _rotation, const glm::vec3& _position)
{
	position    = _position;
	rotation    = _rotation;
	scale       = _scale;
	quaternion  = glm::quat(rotation);
	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	UpdateLocalMatrix();
}

void Object3d::ScaleQuaternionPosition(const glm::vec3& _scale, const glm::quat& _quaternion, const glm::vec3& _position)
{
	position    = _position;
	quaternion  = _quaternion;
	scale       = _scale;
	rotation    = glm::eulerAngles(quaternion);
	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	UpdateLocalMatrix();
}

void Object3d::SynchronizePhysics()
{
}

void Object3d::TraverseAndRender(uint8_t viewId, int renderFlags)
{
	if (!(type & ObjectType_Group))
		if (type & ObjectType_Instance) return;

	//UpdateWorldMatrix();
	Render(viewId, renderFlags);
	for (const auto& child : children)
		child->TraverseAndRender(viewId, renderFlags);
}

void Object3d::UpdateLocalMatrix()
{
	transform = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(quaternion) * scaleMatrix;
	matrix    = transform;
	UpdateWorldMatrix();
}

void Object3d::UpdateWorldMatrix()
{
	if (parent)
		matrixWorld = parent->matrixWorld * matrix;
	else
		matrixWorld = matrix;

	for (auto& child : children)
		child->UpdateWorldMatrix();
}
