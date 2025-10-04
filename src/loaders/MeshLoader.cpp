// MeshLoader.cpp
// @author octopoulos
// @version 2025-09-29

#include "stdafx.h"
#include "loaders/MeshLoader.h"
//
#include "materials/MaterialManager.h" // GetMaterialManager
#include "textures/TextureManager.h"   // GetTextureManager

enum MeshFormats_ : int
{
	MeshFormat_Bgfx = 1,
	MeshFormat_Fbx  = 2,
	MeshFormat_Gltf = 3,
};

static const UMAP_STR_INT extFormats = {
	{ "bin" , MeshFormat_Bgfx },
	{ "fbx" , MeshFormat_Fbx  },
	{ "glb" , MeshFormat_Gltf },
	{ "gltf", MeshFormat_Gltf },
};

static const VEC_STR priorityExts = { "glb", "gltf", "fbx", "bin" };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sMesh MeshLoader::LoadModel(std::string_view name, std::string_view modelName, bool ramcopy, std::string_view texPath)
{
	const std::filesystem::path modelDir = "runtime/models";

	sMesh mesh;

	for (const auto& ext : priorityExts)
	{
		const auto path = modelDir / Format("%s.%s", Cstr(modelName), Cstr(ext));
		if (IsFile(path))
		{
			const int format = FindDefault(extFormats, ext, 0);
			// clang-format off
			switch (format)
			{
			case MeshFormat_Bgfx: mesh = LoadBgfx(path, ramcopy, texPath); break;
			case MeshFormat_Fbx : mesh = LoadFbx (path, ramcopy, texPath); break;
			case MeshFormat_Gltf: mesh = LoadGltf(path, ramcopy, texPath); break;
			}
			// clang-format on

			if (mesh) break;
		}
	}

	if (!mesh)
	{
		ui::LogError("LoadModel: Cannot load: %s @%s", Cstr(name), Cstr(modelName));
		return nullptr;
	}

	mesh->load      = MeshLoad_Basic;
	mesh->name      = name;
	mesh->modelName = NormalizeFilename(modelName);
	return mesh;
}

sMesh MeshLoader::LoadModelFull(std::string_view name, std::string_view modelName, const VEC_STR& texFiles, std::string_view texPath)
{
	// 1) load model
	auto mesh = LoadModel(name, modelName, true, texPath);
	if (!mesh) return nullptr;
	ui::Log("LoadModelFull: %s %s groups=%lld", Cstr(name), Cstr(modelName), mesh->groups.size());

	// 2) create material
	if (!mesh->material)
	{
		mesh->material = GetMaterialManager().LoadMaterial(modelName, "vs_model_texture", "fs_model_texture");
		mesh->material->FindModelTextures(modelName, texFiles);
	}

	// 3) done
	mesh->load      = MeshLoad_Full;
	mesh->modelName = NormalizeFilename(modelName);
	return mesh;
}
