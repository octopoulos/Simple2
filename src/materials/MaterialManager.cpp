// MaterialManager.cpp
// @author octopoulos
// @version 2025-08-28

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

sMaterial MaterialManager::LoadMaterial(std::string_view name, std::string_view vsName, std::string_view fsName, std::string_view colorName, std::string_view normalName)
{
	// 1) check cache
	if (const auto exist = GetMaterial(name)) return exist;

	// 2) create material
	auto material = std::make_shared<Material>(vsName, fsName);
	if (!material) return nullptr;

	// 3) add texture
	if (colorName.size()) material->LoadTextures(colorName, normalName);

	std::lock_guard<std::mutex> lock(materialMutex);
	materials.emplace(name, material);

	// 4) debug
	PrintMaterials();
	return material;
}

void MaterialManager::PrintMaterials()
{
	ui::Log("Materials ({}):", materials.size());
	for (int i = -1; const auto& [name, material] : materials)
		ui::Log("{:2}: {:2} : {}", ++i, material.use_count(), name);
}

void MaterialManager::ShowTable()
{
	if (ImGui::BeginTable("2ways", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;

		ImGui::TableSetupColumn("Name" , ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 3.0f);
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
