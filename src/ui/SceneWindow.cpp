// SceneWindow.cpp
// @author octopoulos
// @version 2025-08-04

#include "stdafx.h"
#include "ui/ui.h"
//
#include "app/App.h"
#include "ui/xsettings.h"

namespace ui
{

class SceneWindow : public CommonWindow
{
public:
	SceneWindow() { name = "Scene"; }

	void Draw()
	{
		CHECK_DRAW();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		if (!ImGui::Begin("Scene", &isOpen))
		{
			ImGui::End();
			return;
		}

		if (ImGui::BeginTable("3ways", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody))
		{
			const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;

			ImGui::TableSetupColumn("##Name", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("##Vis" , ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 5.0f);
			ImGui::TableSetupColumn("##Type", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 3.0f);

			DrawObject(app->scene, 0);

			ImGui::EndTable();
		}

		ImGui::PopStyleVar();
		ImGui::End();
	}

	void DrawCells(const sObject3d& node)
	{
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			app->SelectObject(node);
			ui::Log("SceneWindow/DrawObject: {}", node->name);
		}

		ImGui::TableNextColumn();
		ImGui::PushID(node->name.c_str());

		if (node->visible)
		{
			if (ImGui::SmallButton("VIS"))
				node->visible = !node->visible;
		}
		else if (ImGui::SmallButton("HID"))
			node->visible = !node->visible;

		ImGui::PopID();
		ImGui::TableNextColumn();
		ImGui::Text("%d", node->type);
	}

	void DrawObject(const sObject3d& node, int depth)
	{
		static const ImGuiTreeNodeFlags treeBaseFlags = ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_SpanAvailWidth;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		const bool hidden = !node->visible;
		if (hidden) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

		if (const auto numChild = node->children.size())
		{
			if (ImGui::TreeNodeEx(fmt::format("({}) {}", numChild, node->name).c_str(), treeBaseFlags | (depth ? 0 : ImGuiTreeNodeFlags_DefaultOpen)))
			{
				DrawCells(node);
				for (const auto& child : node->children)
					DrawObject(child, depth + 1);
				ImGui::TreePop();
			}
			else DrawCells(node);
		}
		else
		{
			ImGui::TreeNodeEx(node->name.c_str(), treeBaseFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			DrawCells(node);
		}

		if (hidden) ImGui::PopStyleColor();
	}
};

static SceneWindow sceneWindow;

CommonWindow& GetSceneWindow() { return sceneWindow; }

} // namespace ui
