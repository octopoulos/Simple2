// RubikCube.h
// @author octopoulos
// @version 2025-08-03

#pragma once

#include "objects/Mesh.h"

using sRubikCube = std::shared_ptr<class RubikCube>;

class RubikCube : public Mesh
{
private:
	int cubeSize = 3; ///< size of the cube (e.g., 3 for 3x3x3)

public:
	RubikCube(std::string_view name, int cubeSize)
	    : Mesh(name, ObjectType_Group | ObjectType_RubikCube)
	    , cubeSize(cubeSize)
	{
		CreateCubies();
	}

	~RubikCube() = default;

	/// Create all cubies and position them in a 3D grid
	void CreateCubies();

	/// Serialize for JSON output
	virtual int Serialize(fmt::memory_buffer& outString, int depth, int bounds = 3, bool addChildren = true) const override;

	/// Get an Object3d as a Mesh
	static sRubikCube SharedPtr(const sObject3d& object)
	{
		return (object && (object->type & ObjectType_RubikCube)) ? std::static_pointer_cast<RubikCube>(object) : nullptr;
	}

	/// Show info table in ImGui
	virtual void ShowTable() const override;
};
