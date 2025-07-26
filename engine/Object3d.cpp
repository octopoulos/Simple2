// Object3d.cpp
// @author octopoulos
// @version 2025-07-21

#include "stdafx.h"
#include "Object3d.h"

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
}

void Object3d::ScaleRotationPosition(const glm::vec3& _scale, const glm::vec3& _rotation, const glm::vec3& _position)
{
	position   = _position;
	rotation   = _rotation;
	scale      = _scale;
	quaternion = glm::quat(rotation);

	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
	transform   = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(quaternion) * scaleMatrix;
	localMatrix = transform;
	worldMatrix = transform;
}

void Object3d::ScaleQuaternionPosition(const glm::vec3& _scale, const glm::quat& _quaternion, const glm::vec3& _position)
{
	position   = _position;
	quaternion = _quaternion;
	scale      = _scale;
	rotation   = glm::eulerAngles(quaternion);

	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
	transform   = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(quaternion) * scaleMatrix;
	localMatrix = transform;
	worldMatrix = transform;
}

void Object3d::SynchronizePhysics()
{
}

void Object3d::TraverseAndRender(uint8_t viewId, int renderFlags)
{
	if (!(type & ObjectType_Group))
		if (type & ObjectType_Instance) return;

	UpdateMatrix();
	Render(viewId, renderFlags);
	for (const auto& child : children)
		child->TraverseAndRender(viewId, renderFlags);
}

void Object3d::UpdateMatrix()
{
	if (parent)
		worldMatrix = parent->worldMatrix * localMatrix;
	else
		worldMatrix = localMatrix;

	for (auto& child : children)
		child->UpdateMatrix();
}
