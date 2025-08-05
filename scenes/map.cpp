// map.cpp
// @author octopoulos
// @version 2025-07-31

#include "stdafx.h"
#include "app/App.h"
//
#include "core/ShaderManager.h"
#include "loaders/MeshLoader.h"
#include "textures/TextureManager.h"

#include "imgui-include.h"

void App::AddGeometry(uGeometry geometry)
{
	const auto& coord = cursor->position;

	auto object      = std::make_shared<Mesh>();
	object->geometry = geometry;
	object->material = std::make_shared<Material>(GetShaderManager().LoadProgram("vs_model_texture", "fs_model_texture"));
	object->LoadTextures("colors.png");

	object->ScaleRotationPosition(
	    { 1.0f, 1.0f, 1.0f },
	    { 0.0f, 0.0f, 0.0f },
	    { coord.x, coord.y, coord.z }
	);
	object->CreateShapeBody(physics.get(), GeometryShape(geometry->type, false));

	mapNode->AddNamedChild(object, fmt::format("{}:{}:{}:{} => {}", GeometryName(geometry->type), coord.x, coord.y, coord.z, GeometryShape(geometry->type, false)));
}

void App::AddObject(const std::string& name)
{
	const auto& coord = cursor->position;

	ui::Log("AddObject: {} @ {} {} {}", name, coord.x, coord.y, coord.z);
	if (auto object = MeshLoader::LoadModelFull(name))
	{
		object->ScaleRotationPosition(
		    { 1.0f, 1.0f, 1.0f },
		    { 0.0f, 0.0f, 0.0f },
		    { coord.x, coord.y, coord.z }
		);
		object->CreateShapeBody(physics.get(), ShapeType_TriangleMesh);

		mapNode->AddNamedChild(object, fmt::format("{}:{}:{}:{}", name, coord.x, coord.y, coord.z));
	}
}

void App::MapUi()
{
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const ImVec2 firstSize   = { 250.0f, displaySize.y - 100.0f };

	ImGui::SetNextWindowPos(ImVec2(displaySize.x - firstSize.x - 10.0f, 50.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(firstSize, ImGuiCond_FirstUseEver);

	ImGui::Begin("Controls");
	{
		ImGui::SliderInt("Icon size", &iconSize, 32, 128, "%d px");

		const auto  style        = ImGui::GetStyle();
		const auto& wsize        = ImGui::GetWindowSize() - style.WindowPadding;
		const float imagePadding = style.FramePadding.x * 2;
		const float imageSize    = iconSize;
		const float spacing      = std::clamp(iconSize / 16.0f, 2.0f, 8.0f);
		const int   itemsPerRow  = std::max(1, int((wsize.x - style.IndentSpacing - style.ScrollbarSize) / (imageSize + imagePadding + spacing)));

		for (const auto& [kit, models] : kitModels)
		{
			if (ImGui::TreeNode(fmt::format("[{}] {}", models.size(), kit.size() ? kit : "*"s).c_str()))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

				int       count    = 0;
				const int numModel = TO_INT(models.size());

				for (const auto& [name, hasPreview] : models)
				{
					bool       hasImage = false;
					const auto kitName  = fmt::format("{}/{}", kit, name);
					//ImGui::TextUnformatted(name.c_str());

					// display previews here (png files)
					if (hasPreview)
					{
						const auto preview = fmt::format("runtime/models-prev/{}.png", kitName);
						const auto handle  = GetTextureManager().LoadTexture(preview);
						if (bgfx::isValid(handle))
						{
							// crop if texture is 512px
							auto uv0 = ImVec2(0.0f, 0.0f);
							auto uv1 = ImVec2(1.0f, 1.0f);
							if (const auto* info = GetTextureManager().GetTextureInfo(preview); info->width >= 512)
							{
								uv0 = ImVec2(0.3f, 0.3f);
								uv1 = ImVec2(0.7f, 0.7f);
							}

							ImTextureID texId = (ImTextureID)(uintptr_t)handle.idx;
							if (ImGui::ImageButton(fmt::format("##{}", kitName).c_str(), texId, ImVec2(imageSize, imageSize), uv0, uv1))
								AddObject(kitName);

							if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", name.c_str());
							hasImage = true;
						}
					}

					// no image => show text in boxes
					if (!hasImage)
					{
						const ImVec2 boxSize = { imageSize + imagePadding, imageSize + imagePadding };
						if (ImGui::Button(fmt::format("##{}", kitName).c_str(), boxSize))
							AddObject(kitName);

						// draw wrapped text manually inside the button
						const ImVec2 textMin   = ImGui::GetItemRectMin();
						const ImVec2 textPos   = textMin + ImVec2(4.0f, 4.0f);
						const float  wrapWidth = boxSize.x - 8.0f;

						ImGui::GetWindowDrawList()->AddText(
						    ImGui::GetFont(),
						    ImGui::GetFontSize(),
						    textPos,
						    ImGui::GetColorU32(ImGuiCol_Text),
						    name.c_str(),
						    nullptr,
						    wrapWidth
						);
					}

					// layout next item on same row
					++count;
					if (count < numModel && (count % itemsPerRow) != 0)
						ImGui::SameLine();
				}

				ImGui::PopStyleVar();
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
}

bool App::OpenMap(const std::filesystem::path& filename)
{
	mapNode.reset();
	tiles.clear();

	ReadLines(filename, [&](std::string_view line, int lineId) {
		if (line.empty() || line[0] == '#') return;

		const auto splits = SplitStringView(line, '\t');
		if (splits.size() == 4)
		{
			const auto  name = std::string(splits[0]);
			const float x    = FastAtof(splits[1]);
			const float y    = FastAtof(splits[2]);
			const float z    = FastAtof(splits[3]);

			auto mesh      = std::make_shared<Mesh>();
			mesh->id       = 0;
			mesh->name     = name;
			mesh->position = glm::vec3(x, y, z);
			mapNode->AddChild(mesh);

			Tile tile = {
				.mesh = mesh,
				.name = name,
				.x    = x,
				.y    = y,
				.z    = z,
			};
			tiles.push_back(tile);
			ui::Log("Loaded tile: {} at ({}, {}, {})", tile.name, tile.x, tile.y, tile.z);
		}
	});

	return true;
}

void App::RescanAssets()
{
	const std::filesystem::path& MODEL_SRC_DIR   = "assets/models";
	const std::filesystem::path& MODEL_OUT_DIR   = "runtime/models";
	const std::filesystem::path& PREVIEW_OUT_DIR = "runtime/models-prev";
	const std::filesystem::path& TEXTURE_OUT_DIR = "runtime/textures";

	static const USET_STR validModelExts   = { ".glb", ".obj" };
	static const USET_STR validTextureExts = { ".png", ".jpg", ".dds", ".tga" };

	std::vector<std::filesystem::path> modelSources;
	std::vector<std::filesystem::path> modelOutputs;
	std::vector<std::filesystem::path> textureOutputs;

	for (auto& entry : std::filesystem::recursive_directory_iterator(MODEL_SRC_DIR))
	{
		const std::filesystem::path& path = entry.path();
		if (!IsFile(path)) continue;

		const std::string ext = path.extension().string();
		if (!validModelExts.contains(ext)) continue;

		std::filesystem::path relPath     = std::filesystem::relative(path, MODEL_SRC_DIR);
		std::string           modelName   = path.stem().string();
		std::filesystem::path firstFolder = relPath.empty() ? "" : *relPath.begin();

		std::filesystem::path modelOut = firstFolder.empty()
		    ? (MODEL_OUT_DIR / (modelName + ".bin"))
		    : (MODEL_OUT_DIR / firstFolder / (modelName + ".bin"));

		std::filesystem::path previewOut = firstFolder.empty()
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

			std::string cmd = fmt::format(
			    "geometryc -f \"{}\" -o \"{}\"",
			    path.string(), modelOut.string());

			int result = std::system(cmd.c_str());

			if (result != 0)
				ui::Log("ERROR: geometryc failed with code {}", result);
		}

		modelSources.push_back(path);
		modelOutputs.push_back(modelOut);

		if (!firstFolder.empty())
		{
			const auto texBase = MODEL_SRC_DIR / firstFolder;
			for (auto& texEntry : std::filesystem::recursive_directory_iterator(texBase))
			{
				const auto& texPath = texEntry.path();
				if (!IsFile(texPath)) continue;

				if (texPath.parent_path().filename() != "Textures") continue;

				std::string ext = texPath.extension().string();
				if (!validTextureExts.contains(ext)) continue;

				const auto texName = texPath.filename();
				const auto texOut  = TEXTURE_OUT_DIR / firstFolder / texName;

				if (!IsFile(texOut))
				{
					CreateDirectories(texOut.parent_path());
					if (CopyFileX(texPath, texOut, false))
					{
						ui::Log("Copy: {} -> {}", texPath, texOut);
						textureOutputs.push_back(texOut);
					}
				}
			}
		}
	}

	ui::Log("Models: {}  Textures: {}", modelSources.size(), textureOutputs.size());
}


bool App::SaveMap(const std::filesystem::path& filename)
{
	std::ofstream ofs(filename, std::ios::binary);
	if (!ofs.is_open())
	{
		ui::Log("Failed to open file for writing: {}", filename);
		return false;
	}

	ofs << "# Map file generated by App\n\n";

	for (const auto& tile : tiles)
		ofs << fmt::format("{}\t{}\t{}\t{}\n", tile.name, tile.x, tile.y, tile.z);

	ui::Log("Saved {} tiles to {}", tiles.size(), filename);
	return true;
}

void App::ScanModels(const std::filesystem::path& folder, const std::filesystem::path& folderPrev, int depth, const std::string& relative)
{
	MAP_STR_INT&                       models     = kitModels[relative];
	std::vector<std::filesystem::path> subFolders = {};

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
				else if (path.has_extension() && path.extension() == ".bin")
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
	if (depth == 0) [[unlikely]]
	{
		for (const auto& [kit, models] : kitModels)
		{
			ui::Log("{}: {}", kit, models.size());
			for (const auto& [name, flag] : models)
				ui::Log("  - {} : {}", name, flag);
		}
	}
}
