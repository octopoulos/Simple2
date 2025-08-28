// MeshLoader.cpp
// @author octopoulos
// @version 2025-08-24

#include "stdafx.h"
#include "loaders/MeshLoader.h"
//
#include "core/ShaderManager.h"
#include "textures/TextureManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MeshLoader
/////////////

sMesh MeshLoader::LoadModel(std::string_view name, std::string_view modelName, bool ramcopy)
{
	std::filesystem::path path = "runtime/models";
	path /= fmt::format("{}.bin", modelName);

	bx::FilePath filePath(path.string().c_str());

	auto mesh = MeshLoad(name, filePath, ramcopy);
	if (!mesh)
	{
		ui::LogError("LoadModel: Cannot load: {} @{}", modelName, path);
		return nullptr;
	}

	mesh->load      = MeshLoad_Basic;
	mesh->modelName = NormalizeFilename(modelName);
	return mesh;
}

sMesh MeshLoader::LoadModelFull(std::string_view name, std::string_view modelName, std::string_view textureName)
{
	auto mesh = LoadModel(name, modelName, true);
	if (!mesh) return nullptr;

	// 1) default shader
	mesh->material = std::make_shared<Material>("vs_model_texture", "fs_model_texture");

	// 2) find a texture
	if (textureName.size())
	{
		const auto& texture = GetTextureManager().LoadTexture(textureName);
		if (bgfx::isValid(texture)) mesh->textures.push_back(texture);
	}
	else
	{
		const auto parent      = std::filesystem::path(modelName).parent_path();
		const auto texturePath = std::filesystem::path("runtime/textures") / parent;

		if (!parent.empty() && IsDirectory(texturePath))
		{
			VEC_STR texNames;
			for (const auto& dirEntry : std::filesystem::directory_iterator { texturePath })
			{
				const auto& path     = dirEntry.path();
				const auto  filename = path.filename();
				const auto  texName  = fmt::format("{}/{}", parent.string(), filename.string());
				texNames.push_back(texName);
			}

			// load all texture variations
			if (const int size = TO_INT(texNames.size()))
			{
				for (int id = -1; const auto& texName : texNames)
				{
					const auto& texture = GetTextureManager().LoadTexture(texName);
					if (bgfx::isValid(texture)) mesh->textures.push_back(texture);
					ui::Log("=> {} {} {}", id, texName, bgfx::isValid(texture));
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

	mesh->load      = MeshLoad_Full;
	mesh->modelName = NormalizeFilename(modelName);
	return mesh;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

std::string NormalizeFilename(std::string_view filename)
{
	return ReplaceAll(filename, "\\", "/");
}

TEST_CASE("NormalizeFilename")
{
	// clang-format off
	const std::vector<std::tuple<std::string, std::string>> vectors = {
		{ ""               , ""               },
		{ "kenney/car.bin" , "kenney/car.bin" },
		{ "kenney\\car.bin", "kenney/car.bin" },
	};
	// clang-format on
	for (int i = -1; const auto& [filename, answer] : vectors)
	{
		SUBCASE_FMT("{}_{}", ++i, filename)
		CHECK(NormalizeFilename(filename) == answer);
	}
}
