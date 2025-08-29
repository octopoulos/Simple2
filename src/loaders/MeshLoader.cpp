// MeshLoader.cpp
// @author octopoulos
// @version 2025-08-25

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
	// 1) load model
	auto mesh = LoadModel(name, modelName, true);
	if (!mesh) return nullptr;

	// 2) create material
	mesh->material = std::make_shared<Material>("vs_model_texture", "fs_model_texture");
	mesh->material->FindModelTextures(modelName, textureName);

	// 3) done
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
