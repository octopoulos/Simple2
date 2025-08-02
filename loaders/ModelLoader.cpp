// ModelLoader.cpp
// @author octopoulos
// @version 2025-07-28

#include "stdafx.h"
#include "loaders/ModelLoader.h"
//
#include "core/ShaderManager.h"
#include "textures/TextureManager.h"

sMesh ModelLoader::LoadModel(std::string_view name, bool ramcopy)
{
	std::filesystem::path path = "runtime/models";
	path /= fmt::format("{}.bin", name);

	bx::FilePath filePath(path.string().c_str());

	auto mesh = MeshLoad(filePath, ramcopy);
	if (!mesh)
	{
		ui::LogError("Failed to load mesh: {} @{}", name, path);
		return nullptr;
	}
	return mesh;
}

sMesh ModelLoader::LoadModelFull(std::string_view name, std::string_view textureName)
{
	auto mesh = LoadModel(name, true);
	if (!mesh) return nullptr;

	// 1) default shader
	mesh->program = GetShaderManager().LoadProgram("vs_model_texture", "fs_model_texture");

	// 2) find a texture
	if (textureName.size())
	{
		mesh->texColor = GetTextureManager().LoadTexture(name);
		mesh->Initialize();
	}
	else
	{
		const auto parent      = std::filesystem::path(name).parent_path();
		const auto texturePath = std::filesystem::path("runtime/textures") / parent;

		if (IsDirectory(texturePath))
		{
			VEC_STR names;
			for (const auto& dirEntry : std::filesystem::directory_iterator { texturePath })
			{
				const auto& path     = dirEntry.path();
				const auto  filename = path.filename();
				const auto  name     = fmt::format("{}/{}", parent.string(), filename.string());
				names.push_back(name);
			}

			if (const int size = TO_INT(names.size()))
			{
				// TODO: for now, keep the first variant, but we should give the user the choice
				const int  index = 0; // MerseneInt32(0, size - 1);
				const auto name  = names[index];
				mesh->texColor   = GetTextureManager().LoadTexture(name);
				ui::Log("=> {} {} {}", index, name, bgfx::isValid(mesh->texColor));
				mesh->Initialize();
			}
		}
	}

	return mesh;
}
