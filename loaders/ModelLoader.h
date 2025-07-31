// ModelLoader.h
// @author octopoulos
// @version 2025-07-27

#pragma once

#include "objects/Mesh.h"

class ModelLoader
{
public:
	ModelLoader()  = default;
	~ModelLoader() = default;

	/// Loads mesh from file, creates Object3D with that mesh, and returns it
	static sMesh LoadModel(std::string_view name, bool ramcopy = false);

	/// Loads mesh from file, creates Object3D with that mesh + add mesh shader + guess texture, and returns it
	static sMesh LoadModelFull(std::string_view name);
};
