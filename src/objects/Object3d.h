// Object3d.h
// @author octopoulos
// @version 2025-08-19

#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OBJECT3D
///////////

class Object3d;
using sObject3d = std::shared_ptr<Object3d>;

/// An object can have multiple types (like a flag)
enum ObjectTypes_ : int
{
	ObjectType_Basic     = 1 << 0,  ///< basic object
	ObjectType_Camera    = 1 << 1,  ///< camera object
	ObjectType_Clone     = 1 << 2,  ///< clone of another object (doesn't own groups)
	ObjectType_Cursor    = 1 << 3,  ///< cursor for placing objects
	ObjectType_Group     = 1 << 4,  ///< group (no render)
	ObjectType_HasBody   = 1 << 5,  ///< has a body (Mesh only)
	ObjectType_Instance  = 1 << 6,  ///< instance
	ObjectType_Map       = 1 << 7,  ///< map
	ObjectType_Mesh      = 1 << 8,  ///< mesh
	ObjectType_RubikCube = 1 << 9,  ///< Rubic cube
	ObjectType_Scene     = 1 << 10, ///< scene
};

enum RenderFlags_ : int
{
	RenderFlag_Instancing = 1 << 0,
};

class Object3d
{
public:
	std::vector<sObject3d>            children    = {};                         ///< sub-objects
	int                               id          = 0;                          ///< unique id
	int                               irot[3]     = {};                         ///< number of 45 deg rotations
	glm::mat4                         matrix      = glm::mat4(1.0f);            ///< full local transform (S * R * T)
	glm::mat4                         matrixWorld = glm::mat4(1.0f);            ///< parent->matrixWorld * matrix
	std::string                       name        = "";                         ///< object name (used to find in scene)
	UMAP_STR<std::weak_ptr<Object3d>> names       = {};                         ///< named children
	Object3d*                         parent      = nullptr;                    ///< parent object
	glm::vec3                         position    = glm::vec3(0.0f);            ///< position
	glm::vec3                         position1   = glm::vec3(0.0f);            ///< position: origin
	glm::vec3                         position2   = glm::vec3(0.0f);            ///< position: target
	double                            posTs       = 0.0;                        ///< stamp when moved
	glm::quat                         quaternion  = glm::identity<glm::quat>(); ///< quaternion
	glm::quat                         quaternion1 = glm::identity<glm::quat>(); ///< quaternion: origin
	glm::quat                         quaternion2 = glm::identity<glm::quat>(); ///< quaternion: target
	double                            quatTs      = 0.0;                        ///< stamp when rotated
	glm::quat                         rotation    = glm::vec3(0.0f);            ///< rotation: Euler angles
	glm::vec3                         scale       = glm::vec3(1.0f);            ///< sx, sy, sz
	glm::mat4                         scaleMatrix = glm::mat4(1.0f);            ///< uses scale
	int                               type        = ObjectType_Basic;           ///< ObjectTypes_
	bool                              visible     = true;                       ///< object is rendered if true

	Object3d(std::string_view name, int type = ObjectType_Basic)
		: name(name)
	    , type(type)
	{
	}

	virtual ~Object3d() = default;

	/// Add a child to the object
	void AddChild(sObject3d child);

	/// Find an direct child by name
	sObject3d GetObjectByName(std::string_view name) const;

	/// Remove a child from the object
	void RemoveChild(const sObject3d& child);

	/// Render the object
	virtual void Render(uint8_t viewId, int renderFlags);

	/// Calculate rotation and quaternion from irot
	/// @param instant: instant rotation, no smoothing
	void RotationFromIrot(bool instant);

	/// Apply scale then rotation then translation
	void ScaleIrotPosition(const glm::vec3& _scale, const std::array<int, 3>& _irot, const glm::vec3& _position);

	/// Apply scale then rotation then translation
	void ScaleRotationPosition(const glm::vec3& _scale, const glm::vec3& _rotation, const glm::vec3& _position);

	/// Apply scale then quaternion then translation
	void ScaleQuaternionPosition(const glm::vec3& _scale, const glm::quat& _quaternion, const glm::vec3& _position);

	/// Serialize for JSON output
	virtual int Serialize(fmt::memory_buffer& outString, int bounds = 3) const;

	/// Synchronize physics transform
	virtual void SynchronizePhysics();

	/// Update local matrix from scale * quaternion * position
	/// @param force: recompute matrixWorld even if has a body
	void UpdateLocalMatrix(bool force = false);

	/// Calculate world matrix + recursively for all children
	/// - if HasBody => matrixWorld is left untouched because it comes from bullet3
	/// @param force: recompute even if has a body
	void UpdateWorldMatrix(bool force = false);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

/// Convert object type: int to string
std::string ObjectName(int type);

/// Convert object type: string to int
int ObjectType(std::string_view name);
