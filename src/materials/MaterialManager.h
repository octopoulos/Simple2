// MaterialManager.h
// @author octopoulos
// @version 2025-08-25

#pragma once

#include "materials/Material.h"

class MaterialManager
{
private:
	UMAP_STR<sMaterial> materials = {}; /// cached materials

public:
	MaterialManager() = default;

	~MaterialManager() { Destroy(); }

	/// Release all materials
	void Destroy();

	/// Retrieve a loaded material by key
	/// @returns nullptr if not found
	sMaterial GetMaterial(std::string_view name) const;

	/// Load or retrieve a material
	/// - caches internally
	sMaterial LoadMaterial(std::string_view name, std::string_view vsName, std::string_view fsName, std::string_view colorName = "", std::string_view normalName = "");

	/// Print all materials
	void PrintMaterials();

	/// Show materials in ImGui
	void ShowMaterials();
};

MaterialManager& GetMaterialManager();
