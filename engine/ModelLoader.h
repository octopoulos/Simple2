// ModelLoader.h
// @author octopoulos
// @version 2025-07-05

#pragma once

#include "bgfx_utils.h"

class ModelLoader
{
public:
	ModelLoader() = default;
	~ModelLoader() = default;

	/// Loads mesh from file, creates Object3D with that mesh, and returns it
	sMesh LoadModel(std::string_view name);
	Mesh2* LoadModel2(std::string_view name);
};
