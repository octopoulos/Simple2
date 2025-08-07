// MeshLoader.cpp
// @author octopoulos
// @version 2025-07-31

#include "stdafx.h"
#include "loaders/MeshLoader.h"
//
#include "core/ShaderManager.h"
#include "textures/TextureManager.h"

sMesh MeshLoader::LoadModel(std::string_view name, bool ramcopy)
{
	std::filesystem::path path = "runtime/models";
	path /= fmt::format("{}.bin", name);

	bx::FilePath filePath(path.string().c_str());

	auto mesh = MeshLoad(filePath, ramcopy);
	if (!mesh)
	{
		ui::LogError("LoadModel: Cannot load: {} @{}", name, path);
		return nullptr;
	}
	return mesh;
}

sMesh MeshLoader::LoadModelFull(std::string_view name, std::string_view textureName)
{
	auto mesh = LoadModel(name, true);
	if (!mesh) return nullptr;

	// 1) default shader
	mesh->program = GetShaderManager().LoadProgram("vs_model_texture", "fs_model_texture");

	// 2) find a texture
	if (textureName.size())
	{
		const auto& texture = GetTextureManager().LoadTexture(textureName);
		if (bgfx::isValid(texture)) mesh->textures.push_back(texture);
	}
	else
	{
		const auto parent      = std::filesystem::path(name).parent_path();
		const auto texturePath = std::filesystem::path("runtime/textures") / parent;

		if (!parent.empty() && IsDirectory(texturePath))
		{
			VEC_STR names;
			for (const auto& dirEntry : std::filesystem::directory_iterator { texturePath })
			{
				const auto& path     = dirEntry.path();
				const auto  filename = path.filename();
				const auto  name     = fmt::format("{}/{}", parent.string(), filename.string());
				names.push_back(name);
			}

			// load all texture variations
			if (const int size = TO_INT(names.size()))
			{
				for (int id = -1; const auto& name : names)
				{
					const auto& texture = GetTextureManager().LoadTexture(name);
					if (bgfx::isValid(texture)) mesh->textures.push_back(texture);
					ui::Log("=> {} {} {}", id, name, bgfx::isValid(texture));
				}
			}
		}
	}

	// 3) assign current texture
	if (mesh->textures.size())
	{
		mesh->texColor = mesh->textures[0];
		mesh->Initialize();
	}
	return mesh;
}
