// MapWindow.cpp
// @author octopoulos
// @version 2025-09-27

#include "stdafx.h"
#include "ui/ui.h"
//
#include "app/App.h"                 // App
#include "textures/TextureManager.h" // GetTextureManager

namespace ui
{

class MapWindow : public CommonWindow
{
public:
	MapWindow()
	{
		name = "Map";
		type = WindowType_Map;
	}

	void Draw()
	{
		auto app = App::GetApp();
		if (!app) return;

		if (!BeginDraw()) return;

		const auto  style        = ImGui::GetStyle();
		const auto& wsize        = ImGui::GetWindowSize() - style.WindowPadding;
		const float imagePadding = style.FramePadding.x * 2;
		const float imageSize    = xsettings.iconSize;
		const float spacing      = bx::clamp(xsettings.iconSize / 16.0f, 2.0f, 8.0f);
		const int   itemsPerRow  = bx::max(1, int((wsize.x - style.ScrollbarSize) / (imageSize + imagePadding + spacing)));

		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
		for (const auto& [kit, models] : app->kitModels)
		{
			if (ImGui::TreeNodeEx(Format("[%d] %s", models.size(), kit.size() ? Cstr(kit) : "*"), ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanFullWidth))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

				int       count    = 0;
				const int numModel = TO_INT(models.size());

				for (const auto& [name, hasPreview] : models)
				{
					bool       hasImage = false;
					const auto kitName  = FormatStr("%s/%s", Cstr(kit), Cstr(name));
					// ImGui::TextUnformatted(name.c_str());

					// display previews here (png files)
					if (hasPreview)
					{
						const auto preview = FormatStr("runtime/models-prev/%s.png", Cstr(kitName));
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
							if (ImGui::ImageButton(Format("##%s", Cstr(kitName)), ImTextureRef(texId), ImVec2(imageSize, imageSize), uv0, uv1))
								app->AddObject(kitName);

							if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", Cstr(name));
							hasImage = true;
						}
					}

					// no image => show text in boxes
					if (!hasImage)
					{
						const ImVec2 boxSize = { imageSize + imagePadding, imageSize + imagePadding };
						if (ImGui::Button(Format("##%s", Cstr(kitName)), boxSize))
							app->AddObject(kitName);

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
							wrapWidth);
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

		ImGui::PopStyleVar();
		ImGui::End();
	}
};

static MapWindow mapWindow;

CommonWindow& GetMapWindow() { return mapWindow; }

} // namespace ui
