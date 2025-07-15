// ModelLoader.h
// @author octopoulos
// @version 2025-07-11

#pragma once

class ModelLoader
{
public:
	ModelLoader() = default;
	~ModelLoader() = default;

	/// Loads mesh from file, creates Object3D with that mesh, and returns it
	sMesh LoadModel(std::string_view name);
};
