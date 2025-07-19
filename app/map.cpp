// map.cpp
// @author octopoulos
// @version 2025-07-15

#include "stdafx.h"
#include "app.h"

#include "dear-imgui/imgui.h"

void App::MapUi()
{
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const ImVec2 firstSize   = { 300.0f, displaySize.y - 100.0f };

	ImGui::SetNextWindowPos(ImVec2(displaySize.x - firstSize.x - 10.0f, 50.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(firstSize, ImGuiCond_FirstUseEver);

	ImGui::Begin("Controls");
	{
		const auto& wsize        = ImGui::GetWindowSize();
		const float imagePadding = 4.0f;
		const float imageSize    = 64.0f;
		const int   itemsPerRow  = int((wsize.x - 50.0f - imagePadding) / (imageSize + imagePadding));

		for (const auto& [kit, models] : kitModels)
		{
			if (ImGui::TreeNode(fmt::format("[{}] {}", models.size(), kit.size() ? kit : "*"s).c_str()))
			{
				int count = 0;

				for (const auto& [name, hasPreview] : models)
				{
					bool hasImage = false;
					//ImGui::TextUnformatted(name.c_str());

					// display previews here (png files)
					if (hasPreview)
					{
						const auto preview = fmt::format("runtime/models-prev/{}/{}.png", kit, name);
						const auto handle  = textureManager.LoadTexture(preview);
						if (bgfx::isValid(handle))
						{
							ImTextureID texId = (ImTextureID)(uintptr_t)handle.idx;
							if (ImGui::ImageButton(fmt::format("##{}", preview).c_str(), texId, ImVec2(imageSize, imageSize)))
							{
								ui::Log("Clicked on model: {}", name);
							}
							if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", name.c_str());
							hasImage = true;
						}
					}
					// HERE, I WANT TO WRAP THE TEXT IN a 64x64 box
					if (!hasImage) ImGui::TextWrapped("%s", name.c_str());

					// layout next item on same row
					if (++count % itemsPerRow != 0) ImGui::SameLine();
				}
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
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
					const auto stem    = filename.stem().string();
					const auto preview = folderPrev / (stem + ".png");
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
