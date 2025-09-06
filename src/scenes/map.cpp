// map.cpp
// @author octopoulos
// @version 2025-09-01

#include "stdafx.h"
#include "app/App.h"
//
#include "common/config.h"      // DEV_models
#include "loaders/MeshLoader.h" // MeshLoader::

void App::AddGeometry(uGeometry geometry)
{
	const auto& coord = cursor->position;
	const auto& irot  = cursor->irot;

	if (auto object = std::make_shared<Mesh>(fmt::format("{}:{}", mapNode->children.size(), GeometryName(geometry->type), GeometryShape(geometry->type, false))))
	{
		object->geometry = geometry;
		object->material = std::make_shared<Material>("vs_model_texture", "fs_model_texture");
		object->material->LoadTextures({ "colors.png" });

		object->ScaleRotationPosition(
			{ 1.0f, 1.0f, 1.0f },
			{ irot[0], irot[1], irot[2] },
			{ coord.x, coord.y, coord.z }
		);
		object->CreateShapeBody(physics.get(), GeometryShape(geometry->type, false));

		mapNode->AddChild(object);
		AutoSave();
		SelectObject(object);
	}

	FocusScreen();
}

void App::AddObject(std::string_view modelName)
{
	const auto  coord = glm::vec3(cursor->position.x, cursor->position.y - 1.0f, cursor->position.z);
	const auto& irot  = cursor->irot;

	ui::Log("AddObject: {} @ {} {} {}", modelName, coord.x, coord.y, coord.z);
	if (auto object = MeshLoader::LoadModelFull(fmt::format("{}:{}", mapNode->children.size(), NodeName(modelName)), modelName))
	{
		object->ScaleIrotPosition(
		    { 1.0f, 1.0f, 1.0f },
		    { irot[0], irot[1], irot[2] },
		    { coord.x, coord.y, coord.z }
		);
		object->CreateShapeBody(physics.get(), ShapeType_TriangleMesh);

		mapNode->AddChild(object);
		AutoSave();
		SelectObject(object);
	}

	FocusScreen();
}

void App::AutoSave(const sObject3d& target)
{
	if (target)
	{
		// don't save if parent is not "Map"
		const auto* parent = target->parent;
		if (!parent || !(parent->type & ObjectType_Map)) return;
	}
	SaveScene();
}

std::string App::NodeName(std::string_view modelName)
{
	if (modelName.empty()) return "";

	const auto path     = std::filesystem::path(NormalizeFilename(modelName));
	const auto filename = path.filename().string();
	const auto parent   = path.parent_path().string();

	return parent.empty() ? filename : fmt::format("{} ({})", filename, parent);
}

TEST_CASE("NodeName")
{
	// clang-format off
	const std::vector<std::tuple<std::string, std::string>> vectors = {
		{ ""                    , ""                      },
		{ "kenney/car.bin"      , "car.bin (kenney)"      },
		{ "kenney/cars/car.bin" , "car.bin (kenney/cars)" },
		{ "kenney\\car.bin"     , "car.bin (kenney)"      },
		{ "kenney\\cars/car.bin", "car.bin (kenney/cars)" },
	};
	// clang-format on
	for (int i = -1; const auto& [modelName, answer] : vectors)
	{
		SUBCASE_FMT("{}_{}", ++i, modelName)
		CHECK(App::NodeName(modelName) == answer);
	}
}

void App::RescanAssets()
{
	const std::filesystem::path MODEL_OUT_DIR   = "runtime/models";
	const std::filesystem::path MODEL_SRC_DIR   = "assets/models";
	const std::filesystem::path PREVIEW_OUT_DIR = "runtime/models-prev";
	const std::filesystem::path TEXTURE_OUT_DIR = "runtime/textures";

	static const USET_STR modelExts   = { ".glb", ".obj" };
	static const USET_STR textureExts = { ".png", ".jpg", ".dds", ".tga" };

	std::vector<std::filesystem::path> modelOuts;
	std::vector<std::filesystem::path> modelSrcs;
	std::vector<std::filesystem::path> textureOuts;

	for (auto& entry : std::filesystem::recursive_directory_iterator(MODEL_SRC_DIR))
	{
		const std::filesystem::path& path = entry.path();
		if (!IsFile(path)) continue;

		const std::string ext = path.extension().string();
		if (!modelExts.contains(ext)) continue;

		const auto modelName   = path.stem().string();
		const auto relPath     = std::filesystem::relative(path, MODEL_SRC_DIR);
		const auto firstFolder = (relPath.empty() || relPath.parent_path().empty()) ? "" : *relPath.begin();

		ui::Log("       path={}\n    relPath={}\nfirstFolder={}", path, relPath, firstFolder);

		const auto modelOut = firstFolder.empty()
		    ? (MODEL_OUT_DIR / (modelName + ".bin"))
		    : (MODEL_OUT_DIR / firstFolder / (modelName + ".bin"));

		const auto previewOut = firstFolder.empty()
		    ? std::filesystem::path()
		    : (PREVIEW_OUT_DIR / firstFolder / (modelName + ".png"));

		std::filesystem::path previewSrc;
		if (!firstFolder.empty())
		{
			const auto previewBase = MODEL_SRC_DIR / firstFolder;
			const auto prev1       = previewBase / "Previews" / (modelName + ".png");
			const auto prev2       = previewBase / "Isometric" / (modelName + "_NE.png");

			if (IsFile(prev1))
				previewSrc = prev1;
			else if (IsFile(prev2))
				previewSrc = prev2;

			if (!previewSrc.empty() && !IsFile(previewOut))
			{
				CreateDirectories(previewOut.parent_path());
				CopyFileX(previewSrc, previewOut, false);
			}
		}

		if (!IsFile(modelOut))
		{
			ui::Log("Need to convert: {}", relPath.string());
			ui::Log("Convert: {} => {}", path.string(), modelOut.string());

			CreateDirectories(modelOut.parent_path());

			const auto cmd    = fmt::format(R"(geometryc -f "{}" -o "{}")", path.string(), modelOut.string());
			const int  result = std::system(cmd.c_str());
			if (result != 0) ui::Log("ERROR: geometryc failed with code {}", result);
		}

		modelOuts.push_back(modelOut);
		modelSrcs.push_back(path);

		if (!firstFolder.empty())
		{
			const auto texBase = MODEL_SRC_DIR / firstFolder;
			for (auto& texEntry : std::filesystem::recursive_directory_iterator(texBase))
			{
				const auto& texPath = texEntry.path();
				if (!IsFile(texPath)) continue;

				if (texPath.parent_path().filename() != "Textures") continue;

				std::string ext = texPath.extension().string();
				if (!textureExts.contains(ext)) continue;

				const auto texName = texPath.filename();
				const auto texOut  = TEXTURE_OUT_DIR / firstFolder / texName;

				if (!IsFile(texOut))
				{
					CreateDirectories(texOut.parent_path());
					if (CopyFileX(texPath, texOut, false))
					{
						ui::Log("Copy: {} -> {}", texPath, texOut);
						textureOuts.push_back(texOut);
					}
				}
			}
		}
	}

	ui::Log("Models: {}  Textures: {}", modelSrcs.size(), textureOuts.size());
}

void App::ScanModels(const std::filesystem::path& folder, const std::filesystem::path& folderPrev, int depth, const std::string& relative)
{
	MAP_STR_INT&                       models     = kitModels[relative];
	std::vector<std::filesystem::path> subFolders = {};

	auto GoodExtension = [](const std::filesystem::path& path) {
		static USET_STR goodExtensions = { ".bin", ".fbx", ".glb", ".gltf" };

		if (!path.has_extension()) return false;
		return goodExtensions.contains(path.extension().string());
	};

	// 1) check all files in the folder
	{
		int numFile = 0;

		for (const auto& dirEntry : std::filesystem::directory_iterator { folder })
		{
			++numFile;
			const auto& path     = dirEntry.path();
			const auto  filename = path.filename();

			if (IsDirectory(path)) [[unlikely]]
			{
				//ui::Log(" {} / [{}]", relative, path);
				subFolders.emplace_back(filename);
			}
			else if (IsFile(path))
			{
				if (filename == ".DS_Store")
					std::filesystem::remove(path);
				else if (GoodExtension(path))
				{
					const auto stem     = filename.stem().string();
					const auto preview  = folderPrev / (stem + ".png");
					const auto preview2 = folderPrev / (stem + "_NE.png");

					if (!IsFile(preview) && IsFile(preview2))
						std::filesystem::copy_file(preview2, preview);

					//ui::Log(" {} / - {} - {} - {}", relative, filename, preview, IsFile(preview));
					models.insert({ stem, IsFile(preview) ? 1 : 0 });
				}
			}
		}
	}

	// 2) check sub folders, combine 'shuffle' and 'older first'
	{
		for (const auto& subFolder : subFolders)
			ScanModels(folder / subFolder, folderPrev / subFolder, depth + 1, fmt::format("{}{}{}", relative, relative.size() ? "/" : "", subFolder.filename().string()));
	}

	// 3) summary
	if (DEV_models && depth == 0) [[unlikely]]
	{
		for (const auto& [kit, models] : kitModels)
		{
			ui::Log("{}: {}", kit, models.size());
			for (const auto& [name, flag] : models)
				ui::Log("  - {} : {}", name, flag);
		}
	}
}
