// Object3d.h
// @author octopoulos
// @version 2025-08-04

#pragma once

class Object3d;
using sObject3d = std::shared_ptr<Object3d>;

/// An object can have multiple types (like a flag)
enum ObjectTypes_ : int
{
	ObjectType_Basic     = 1 << 0, ///< basic object
	ObjectType_Camera    = 1 << 1, ///< camera object
	ObjectType_Clone     = 1 << 2, ///< clone of another object (doesn't own groups)
	ObjectType_Group     = 1 << 3, ///< group (no render)
	ObjectType_Instance  = 1 << 4, ///< instance
	ObjectType_Mesh      = 1 << 5, ///< mesh
	ObjectType_RubikCube = 1 << 6, ///< Rubic cube
	ObjectType_Scene     = 1 << 7, ///< scene
};

enum RenderFlags_ : int
{
	RenderFlag_Instancing = 1 << 0,
};

class Object3d
{
public:
	std::vector<sObject3d> children    = {};                         ///< sub-objects
	int                    id          = 0;                          ///< unique id
	glm::mat4              matrix      = glm::mat4(1.0f);            ///< full local transform (S * R * T)
	glm::mat4              matrixWorld = glm::mat4(1.0f);            ///< parent->matrixWorld * matrix
	std::string            name        = "";                         ///< object name (used to find in scene)
	Object3d*              parent      = nullptr;                    ///< parent object
	glm::vec3              position    = glm::vec3(0.0f);            ///< x, y, z
	glm::quat              quaternion  = glm::identity<glm::quat>(); ///< quaternion
	glm::quat              rotation    = glm::vec3(0.0f);            ///< rotation: Euler angles
	glm::vec3              scale       = glm::vec3(1.0f);            ///< sx, sy, sz
	glm::mat4              scaleMatrix = glm::mat4(1.0f);            ///< uses scale
	glm::mat4              transform   = {};                         ///< computed from S * R * T
	int                    type        = ObjectType_Basic;           ///< ObjectTypes_
	bool                   visible     = true;                       ///< object is rendered if true

	Object3d()          = default;
	virtual ~Object3d() = default;

	/// Add a child to the object
	virtual void AddChild(sObject3d child);

	/// Add a child and specify its name
	void AddNamedChild(sObject3d child, std::string&& name)
	{
		child->name = std::move(name);
		AddChild(child);
	}

	/// Remove a child from the object
	virtual void RemoveChild(const sObject3d& child);

	/// Render the object
	virtual void Render(uint8_t viewId, int renderFlags);

	/// Apply scale then rotation then translation
	void ScaleRotationPosition(const glm::vec3& _scale, const glm::vec3& _rotation, const glm::vec3& _position);

	/// Apply scale then quaternion then translation
	void ScaleQuaternionPosition(const glm::vec3& _scale, const glm::quat& _quaternion, const glm::vec3& _position);

	/// Synchronize physics transform
	virtual void SynchronizePhysics();

	/// Render the object + recursively for all children
	void TraverseAndRender(uint8_t viewId, int renderFlags);

	/// Update local matrix from scale * quaternion * position
	void UpdateLocalMatrix();

	/// Calculate world matrix + recursively for all children
	void UpdateWorldMatrix();
};
