// SceneWindow.cpp
// @author octopoulos
// @version 2025-08-23

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
	SceneWindow()
	{
		name = "Scene";
		type = WindowType_Scene;
	}

	void Draw()
	{
		CHECK_DRAW();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		if (!ImGui::Begin("Scene", &isOpen))
		{
			ImGui::End();
			return;
		}

		if (ImGui::BeginTable("3ways", 3, ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_RowBg))
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
			app->SelectObject(node, true);

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
		if (!node) return;
		ImGuiTreeNodeFlags treeFlag = ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_SpanAvailWidth;

		ImGui::TableNextRow();

		// blue background
		const bool selected = (node == app->selectedObj.lock());
		if (selected)
		{
			const ImU32 color = ImGui::GetColorU32(ImVec4(0.2f, 0.3f, 0.5f, 1.0f));
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, color);
		}

		ImGui::TableNextColumn();

		// grey or orange text
		int pushed = 0;
		if (!node->visible)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
			pushed = 1;
		}
		else if (selected)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.99f, 0.68f, 0.16f, 1.0f));
			pushed = 2;
		}

		// draw children nodes
		if (const auto numChild = node->children.size())
		{
			// open Scene and Map by default
			if (!depth || (depth == 1 && (node->type & ObjectType_Map)))
				treeFlag |= ImGuiTreeNodeFlags_DefaultOpen;

			if (ImGui::TreeNodeEx(fmt::format("({}) {}", numChild, node->name).c_str(), treeFlag))
			{
				DrawCells(node);
				if (pushed == 2)
				{
					ImGui::PopStyleColor();
					pushed = 0;
				}

				for (const auto& child : node->children)
					DrawObject(child, depth + 1);
				ImGui::TreePop();
			}
			else DrawCells(node);
		}
		// draw node itself
		else
		{
			ImGui::TreeNodeEx(node->name.c_str(), treeFlag | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			DrawCells(node);
		}

		if (pushed) ImGui::PopStyleColor();
	}
};

static SceneWindow sceneWindow;

CommonWindow& GetSceneWindow() { return sceneWindow; }

} // namespace ui
