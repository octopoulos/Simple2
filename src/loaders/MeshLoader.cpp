// MeshLoader.cpp
// @author octopoulos
// @version 2025-09-02

#include "stdafx.h"
#include "loaders/MeshLoader.h"
//
#include "materials/MaterialManager.h" // GetMaterialManager
#include "textures/TextureManager.h"   // GetTextureManager

static const VEC_STR priorityExts = { "glb", "gltf", "fbx", "bin" };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sMesh MeshLoader::LoadModel(std::string_view name, std::string_view modelName, bool ramcopy)
{
	const std::filesystem::path modelDir = "runtime/models";

	sMesh mesh;

	for (const auto ext : priorityExts)
	{
		const auto path = modelDir / fmt::format("{}.{}", modelName, ext);
		if (IsFile(path))
		{
			// bgfx format
			if (ext == "bin")
			{
				bx::FilePath filePath(path.string().c_str());
				mesh = MeshLoad(name, filePath, ramcopy);
			}
			// fbx
			else if (ext == "fbx")
				mesh = LoadFbx(path);
			// gltf
			else mesh = LoadGltf(path);

			if (mesh) break;
		}
	}

	if (!mesh)
	{
		ui::LogError("LoadModel: Cannot load: {} @{}", name, modelName);
		return nullptr;
	}

	mesh->load      = MeshLoad_Basic;
	mesh->name      = name;
	mesh->modelName = NormalizeFilename(modelName);
	return mesh;
}

sMesh MeshLoader::LoadModelFull(std::string_view name, std::string_view modelName, std::string_view textureName)
{
	// 1) load model
	auto mesh = LoadModel(name, modelName, true);
	if (!mesh) return nullptr;

	// 2) create material
	if (!mesh->material)
	{
		mesh->material = GetMaterialManager().LoadMaterial(modelName, "vs_model_texture", "fs_model_texture", { std::string(textureName) });
		mesh->material->FindModelTextures(modelName, textureName);
	}

	// 3) done
	mesh->load      = MeshLoad_Full;
	mesh->modelName = NormalizeFilename(modelName);
	return mesh;
}
