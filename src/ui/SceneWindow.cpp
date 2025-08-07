// SceneWindow.cpp
// @author octopoulos
// @version 2025-08-03

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
		auto& style = ImGui::GetStyle();

		if (!ImGui::Begin("Scene", &isOpen))
		{
			ImGui::End();
			return;
		}

		static ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

		if (ImGui::BeginTable("3ways", 3, table_flags))
		{
			const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;

			ImGui::TableSetupColumn("##Name", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("##Size", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 3.0f);
			ImGui::TableSetupColumn("##Type", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 3.0f);
			//ImGui::TableHeadersRow();

			DrawObject(app->scene.get());

			ImGui::EndTable();
		}

		ImGui::End();
	}

	void DrawObject(Object3d* node)
	{
		static ImGuiTreeNodeFlags tree_node_flags_base = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesToNodes;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if (node->children.size())
		{
			if (ImGui::TreeNodeEx(node->name.c_str(), tree_node_flags_base))
			{
				for (const auto& child : node->children)
					DrawObject(child.get());

				ImGui::TreePop();
			}
		}
		else
		{
			ImGui::TreeNodeEx(node->name.c_str(), tree_node_flags_base | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TableNextColumn();
			ImGui::Text("%d", node->name.size());
			ImGui::TableNextColumn();
			ImGui::Text("%d", node->type);
		}
	}
};

static SceneWindow sceneWindow;

CommonWindow& GetSceneWindow() { return sceneWindow; }

} // namespace ui
