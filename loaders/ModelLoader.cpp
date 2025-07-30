// ModelLoader.cpp
// @author octopoulos
// @version 2025-07-26

#include "stdafx.h"
#include "loaders/ModelLoader.h"

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
