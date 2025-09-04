// MeshLoader.h
// @author octopoulos
// @version 2025-08-30

#pragma once

#include "objects/Mesh.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MeshLoader
/////////////

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

/// Load an FBX file
sMesh LoadFbx(const std::filesystem::path& path);

/// Load a GLB/GLTF file
sMesh LoadGltf(const std::filesystem::path& path);
