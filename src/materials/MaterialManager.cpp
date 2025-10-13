// MaterialManager.cpp
// @author octopoulos
// @version 2025-10-07

#include "stdafx.h"
#include "materials/MaterialManager.h"
//
#include "imgui.h" // ImGui::

static std::mutex materialMutex;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
///////

void MaterialManager::Destroy()
{
	DESTROY_GUARD();
	materials.clear();
}

sMaterial MaterialManager::GetMaterial(std::string_view name) const
{
	std::lock_guard<std::mutex> lock(materialMutex);

	if (const auto& it = materials.find(name); it != materials.end())
		return it->second;
	return nullptr;
}

sMaterial MaterialManager::LoadMaterial(std::string_view name, std::string_view vsName, std::string_view fsName, const VEC_STR& texFiles, const VEC<TextureData>& texDatas)
{
	// 1) check cache
	if (const auto exist = GetMaterial(name))
	{
		ui::Log("LoadMaterial: Cached %s", Cstr(name));
		return exist;
	}

	// 2) create material
	auto material = std::make_shared<Material>(vsName, fsName);
	if (!material)
	{
		ui::LogError("LoadMaterial: Cannot created %s", Cstr(name));
		return nullptr;
	}

	// 3) add texture
	ui::Log("LoadMaterial: texFiles=%lld texDatas=%lld", texFiles.size(), texDatas.size());
	if (texFiles.size()) material->LoadTextures(texFiles);
	if (texDatas.size())
	{

		for (auto& texData : texDatas)
		{
			ui::Log("texData: %d %d %s", texData.type, bgfx::isValid(texData.handle), Cstr(texData.name));
			material->LoadTexture(texData.type, texData.name);
		}
	}

	std::lock_guard<std::mutex> lock(materialMutex);
	materials.emplace(name, material);

	// 4) debug
	PrintMaterials();
	return material;
}

void MaterialManager::PrintMaterials()
{
	ui::Log("Materials (%lld):", materials.size());
	for (int i = -1; const auto& [name, material] : materials)
		ui::Log("%2d: %2d : %s", ++i, material.use_count(), Cstr(name));
}

void MaterialManager::ShowInfoTable(bool showTitle) const
{
	if (showTitle) ImGui::TextUnformatted("Materials");

	if (ImGui::BeginTable("2ways", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		const float countWidth = ImGui::CalcTextSize("Count.").x;

		ImGui::TableSetupColumn("Name" , ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, countWidth);
		ImGui::TableHeadersRow();

		// sort data?
		if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs())
			if (sortSpecs->SpecsDirty)
			{
				ui::Log("MaterialManager:Sort");
				sortSpecs->SpecsDirty = false;
			}

		for (const auto& [name, value] : materials)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(name.c_str());
			ImGui::TableNextColumn();
			ImGui::Text("%d", value.use_count());
		}
		ImGui::EndTable();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

MaterialManager& GetMaterialManager()
{
	static MaterialManager materialManager;
	return materialManager;
}
