// MeshLoader.h
// @author octopoulos
// @version 2025-08-17

#pragma once

#include "objects/Mesh.h"

class MeshLoader
{
public:
	MeshLoader()   = default;
	~MeshLoader() = default;

	/// Loads mesh from file, creates Object3D with that mesh, and returns it
	static sMesh LoadModel(std::string_view name, std::string_view modelName, bool ramcopy = false);

	/// Loads mesh from file, creates Object3D with that mesh + add mesh shader + guess texture, and returns it
	static sMesh LoadModelFull(std::string_view name, std::string_view modelName, std::string_view textureName = "");
};
