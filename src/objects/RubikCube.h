// RubikCube.h
// @author octopoulos
// @version 2025-10-06

#pragma once

#include "objects/Mesh.h"

struct RubikFace
{
	std::string color       = "";              // #ff0000
	std::string name        = "";              // blue, green, orange, red, white, yellow
	glm::vec3   normal      = glm::vec3(0.0f); // normal
	// computed
	glm::vec3   normalWorld = glm::vec3(0.0f); // rotated normal (world)
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using sRubikCube = std::shared_ptr<class RubikCube>;

class RubikCube : public Mesh
{
private:
	int  cubeSize = 3;     ///< size of the cube (e.g., 3 for 3x3x3)
	bool isDirty  = false; ///< cube has rotated and the transforms must be updated

	/// Specific Rubik controls, can be used by AI
	void AiControls(const sCamera& camera, int modifier, const bool* downs, bool isQueue);

	/// Rotate the whole cube (XYZ)
	void RotateCube(const RubikFace* face, int angle, int key, bool isQueue);

	/// Rotate a layer (UD/RL/FB)
	void RotateLayer(const RubikFace* face, int angle, int key, bool isQueue);

	/// Scramble cube
	void Scramble(const sCamera& camera, int steps);

public:
	RubikCube(std::string_view name, int cubeSize)
	    : Mesh(name, ObjectType_Group | ObjectType_RubikCube)
	    , cubeSize(cubeSize)
	{
	}

	~RubikCube() = default;

	/// Specific Rubik controls
	virtual void Controls(const sCamera& camera, int modifier, const bool* downs, bool* ignores, const bool* keys) override;

	/// Create all cubies and position them in a 3D grid
	void Initialize();

	/// Pass the physics object for possible initialization
	virtual void SetPhysics() override;

	/// Serialize for JSON output
	virtual int Serialize(fmt::memory_buffer& outString, int depth, int bounds = 3, bool addChildren = true) const override;

	/// Get an Object3d as a Mesh
	static sRubikCube SharedPtr(const sObject3d& object)
	{
		return (object && (object->type & ObjectType_RubikCube)) ? std::static_pointer_cast<RubikCube>(object) : nullptr;
	}

	/// Show info table in ImGui
	virtual void ShowInfoTable(bool showTitle = true) const override;
};
