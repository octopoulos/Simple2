// Object3d.h
// @author octopoulos
// @version 2025-09-16

#pragma once

enum Deads_ : int
{
	Dead_Alive  = 0, ///< alive object
	Dead_Dead   = 1, ///< dead object, not moving
	Dead_Remove = 2, ///< object must be removed
};

enum ShowObjects_ : int
{
	ShowObject_Basic            = 1 << 1,
	ShowObject_Geometry         = 1 << 2,
	ShowObject_Material         = 1 << 3, // main
	ShowObject_MaterialShaders  = 1 << 4,
	ShowObject_MaterialTextures = 1 << 5,
	ShowObject_Physics          = 1 << 6,
	ShowObject_Transform        = 1 << 7,
};

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
	ObjectType_RubikNode = 1 << 10, ///< Rubic node (single cube child)
	ObjectType_Scene     = 1 << 11, ///< scene
};

enum RenderFlags_ : int
{
	RenderFlag_Instancing = 1 << 0,
};

class Object3d
{
public:
	glm::quat                         axis1       = glm::identity<glm::quat>(); ///< quaternion: axis origin
	glm::quat                         axis2       = glm::identity<glm::quat>(); ///< quaternion: axis target
	double                            axisTs      = 0.0;                        ///< stamp when rotating face
	int                               childId     = 0;                          ///< selected sub-object
	int                               childInc    = 0;                          ///< incremental id
	std::vector<sObject3d>            children    = {};                         ///< sub-objects
	int                               dead        = Dead_Alive;                 ///< dead status: Deads_
	int                               id          = 0;                          ///< unique id
	MAP<int, std::weak_ptr<Object3d>> ids         = {};                         ///< id'd children
	double                            interval    = 0.0;                        ///< current interval
	int                               irot[3]     = {};                         ///< number of 45 deg rotations
	glm::mat4                         matrix      = glm::mat4(1.0f);            ///< full local transform (S * R * T)
	glm::mat4                         matrixWorld = glm::mat4(1.0f);            ///< parent->matrixWorld * matrix
	std::string                       name        = "";                         ///< object name (used to find in scene)
	UMAP_STR<std::weak_ptr<Object3d>> names       = {};                         ///< named children
	Object3d*                         parent      = nullptr;                    ///< parent object
	bool                              parentSync  = true;                       ///< sync'd to the parent or physically independent?
	bool                              placing     = false;                      ///< object is being placed on the map?
	glm::vec3                         position    = glm::vec3(0.0f);            ///< local position
	glm::vec3                         position1   = glm::vec3(0.0f);            ///< position: origin
	glm::vec3                         position2   = glm::vec3(0.0f);            ///< position: target
	double                            posTs       = 0.0;                        ///< stamp when moved
	glm::quat                         quaternion  = glm::identity<glm::quat>(); ///< quaternion
	glm::quat                         quaternion1 = glm::identity<glm::quat>(); ///< quaternion: origin
	glm::quat                         quaternion2 = glm::identity<glm::quat>(); ///< quaternion: target
	double                            quatTs      = 0.0;                        ///< stamp when rotated
	glm::vec3                         rotation    = glm::vec3(0.0f);            ///< rotation: Euler angles
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

	/// Remove dead children
	void ClearDeads(bool force);

	/// Handle incomplete interpolation
	/// @param warp: warp to next position/quaternion, otherwise only check
	/// @returns: &1: axis/pos, &2: quat, &4: child axis/pos, &8: child quat
	int CompleteInterpolation(bool warp, std::string_view origin);

	/// Convert matrix to position, rotation/quaternion, scale + irot
	void DecomposeMatrix();

	/// Apply the easing function
	float EaseFunction(double td);

	/// Get the easing function
	int GetEase();

	/// Get the transition interval
	virtual double GetInterval(bool recalculate = false);

	/// Find an direct child by id
	sObject3d GetObjectById(int id) const;

	/// Find an direct child by name
	sObject3d GetObjectByName(std::string_view name) const;

	/// Calculate Irot from Rotation
	void IrotFromRotation();

	/// Remove a child from the object
	bool RemoveChild(const sObject3d& child);

	/// Render the object
	virtual void Render(uint8_t viewId, int renderFlags);

	/// Calculate rotation and quaternion from irot
	/// @param instant: instant rotation, no smoothing
	void RotationFromIrot(bool instant);

	/// Calculate rotation and irot from quaternion
	void RotationFromQuaternion();

	/// Apply scale then rotation then translation
	void ScaleIrotPosition(const glm::vec3& _scale, const std::array<int, 3>& _irot, const glm::vec3& _position);

	/// Apply scale then rotation then translation
	void ScaleRotationPosition(const glm::vec3& _scale, const glm::vec3& _rotation, const glm::vec3& _position);

	/// Apply scale then quaternion then translation
	void ScaleQuaternionPosition(const glm::vec3& _scale, const glm::quat& _quaternion, const glm::vec3& _position);

	/// Serialize for JSON output
	virtual int Serialize(fmt::memory_buffer& outString, int depth, int bounds = 3, bool addChildren = true) const;

	/// Show info table in ImGui
	virtual void ShowInfoTable(bool showTitle = true) const;

	/// Show settings in ImGui
	/// @param show: ShowObjects_
	virtual void ShowSettings(bool isPopup, int show);

	/// Synchronize physics transform
	virtual int SynchronizePhysics();

	/// Transform a vector
	glm::mat4 TransformPosition(const glm::vec3& _position);

	/// Update local matrix from scale * quaternion * position
	void UpdateLocalMatrix(std::string_view origin = "");

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
