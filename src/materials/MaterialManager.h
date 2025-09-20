// MaterialManager.h
// @author octopoulos
// @version 2025-09-16

#pragma once

#include "materials/Material.h"
#include "textures/TextureManager.h"

class MaterialManager
{
private:
	bool                destroyed = false; ///< Destroy already called?
	UMAP_STR<sMaterial> materials = {};    ///< cached materials

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
	sMaterial LoadMaterial(std::string_view name, std::string_view vsName, std::string_view fsName, const VEC_STR& texFiles = {}, const VEC<TextureData>& texDatas = {});

	/// Print all materials
	void PrintMaterials();

	/// Show materials in ImGui
	void ShowInfoTable(bool showTitle = true) const;
};

MaterialManager& GetMaterialManager();
