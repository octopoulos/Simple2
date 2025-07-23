// Object3d.h
// @author octopoulos
// @version 2025-07-19

#pragma once

class Object3d;
using sObject3d = std::shared_ptr<Object3d>;

enum ObjectTypes_
{
	ObjectType_Basic  = 0, ///< basic object
	ObjectType_Camera = 1, ///< camera object
	ObjectType_Mesh   = 2, ///< mesh
};

class Object3d
{
public:
	std::vector<sObject3d> children    = {};                         ///< sub-objects
	int                    id          = 0;                          ///< unique id
	std::string            name        = "";                         ///< object name (used to find in scene)
	Object3d*              parent      = nullptr;                    ///< parent object
	int                    type        = ObjectType_Basic;           ///< object type
	glm::vec3              position    = glm::vec3(0.0f);            ///< x, y, z
	glm::quat              quaternion  = glm::identity<glm::quat>(); ///< quaternion
	glm::quat              rotation    = glm::vec3(0.0f);            ///< rotation: Euler angles
	glm::vec3              scale       = glm::vec3(1.0f);            ///< sx, sy, sz
	glm::mat4              scaleMatrix = glm::mat4(1.0f);            ///< uses scale
	glm::mat4              transform   = {};                         ///< computed from S * R * T
	glm::mat4              localMatrix = glm::mat4(1.0f);            ///< full local transform (S * R * T)
	glm::mat4              worldMatrix = glm::mat4(1.0f);            ///< parent->worldMatrix * localMatrix

	Object3d()          = default;
	virtual ~Object3d() = default;

	virtual void AddChild(sObject3d child);
	virtual void RemoveChild(const sObject3d& child);
	virtual void Render(uint8_t viewId);
	void         ScaleRotationPosition(const glm::vec3& _scale, const glm::vec3& _rotation, const glm::vec3& _position);
	void         ScaleQuaternionPosition(const glm::vec3& _scale, const glm::quat& _quaternion, const glm::vec3& _position);
	void         TraverseAndRender(uint8_t viewId);
	void         UpdateMatrix();
};
