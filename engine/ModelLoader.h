// ModelLoader.h
// @author octopoulos
// @version 2025-07-20

#pragma once

#include "engine/Mesh.h"

class ModelLoader
{
public:
	ModelLoader()  = default;
	~ModelLoader() = default;

	/// Loads mesh from file, creates Object3D with that mesh, and returns it
	sMesh LoadModel(std::string_view name, bool ramcopy = false);
};
