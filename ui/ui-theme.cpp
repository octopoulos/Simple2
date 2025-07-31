// ui-theme.cpp
// @author octopoulos
// @version 2025-07-26

#include "stdafx.h"
#include "ui/ui.h"
//
#include "ui/xsettings.h"

//#include "imgui_impl_opengl3.h"

namespace ui
{

class ThemeWindow : public CommonWindow
{
public:
	ThemeWindow() { name = "Theme"; }

	void Draw()
	{
		CHECK_DRAW();
		auto& style = ImGui::GetStyle();

		if (!ImGui::Begin("Theme Editor", &isOpen))
		{
			ImGui::End();
			return;
		}

		ImGui::Button("Import Custom");
		ImGui::SameLine();
		ImGui::Button("Export Custom");

		if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Colors"))
			{
				ImGui::PushItemWidth(-160);
				for (int i = 0; i < ImGuiCol_COUNT; ++i)
				{
					auto name = ImGui::GetStyleColorName(i);
					ImGui::PushID(i);
					ImGui::ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					ImGui::TextUnformatted(name);
					ImGui::PopID();
				}
				ImGui::PopItemWidth();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Font"))
			{
				ImGuiIO&     io    = ImGui::GetIO();
				ImFontAtlas* atlas = io.Fonts;
				ImGui::ShowFontAtlas(atlas);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Sizes"))
			{
				ImGui::Text("Main");
				ImGui::SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("CellPadding", (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
				ImGui::SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
				ImGui::SliderFloat("ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
				ImGui::SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");
				ImGui::Text("Borders");
				ImGui::SliderFloat("WindowBorderSize", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("ChildBorderSize", &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("PopupBorderSize", &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("FrameBorderSize", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("TabBorderSize", &style.TabBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::Text("Rounding");
				ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("PopupRounding", &style.PopupRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("LogSliderDeadzone", &style.LogSliderDeadzone, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");
				ImGui::Text("Alignment");
				ImGui::SliderFloat2("WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
				int window_menu_button_position = style.WindowMenuButtonPosition + 1;
				if (ImGui::Combo("WindowMenuButtonPosition", (int*)&window_menu_button_position, "None\0Left\0Right\0"))
					style.WindowMenuButtonPosition = (ImGuiDir)(window_menu_button_position - 1);
				ImGui::Combo("ColorButtonPosition", (int*)&style.ColorButtonPosition, "Left\0Right\0");
				ImGui::SliderFloat2("ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f");
				// ImGui::SameLine(); HelpMarker("Alignment applies when a button is larger than its text content.");
				ImGui::SliderFloat2("SelectableTextAlign", (float*)&style.SelectableTextAlign, 0.0f, 1.0f, "%.2f");
				// ImGui::SameLine(); HelpMarker("Alignment applies when a selectable is larger than its text content.");
				ImGui::Text("Safe Area Padding");
				// ImGui::SameLine(); HelpMarker("Adjust if you cannot see the edges of your screen (e.g. on a TV where scaling has not been configured).");
				ImGui::SliderFloat2("DisplaySafeAreaPadding", (float*)&style.DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f");
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}
};

static ThemeWindow themeWindow;
CommonWindow&      GetThemeWindow() { return themeWindow; }

// FONTS
////////

std::unordered_map<std::string, ImFont*> fontNames;

ImFont* FindFont(const std::string& name)
{
	if (const auto it = fontNames.find(name); it != fontNames.end()) return it->second;
	return fontNames["mono"];
}

// clang-format off
#define FONT_ITEM(name, size) { #name, (void*)name##_data, name##_size, size }
// clang-format on

void UpdateFonts()
{
	auto& io = ImGui::GetIO();
	io.Fonts->Clear();

	const std::vector<std::tuple<const char*, void*, uint32_t, int>> fonts = {
		// FONT_ITEM(RobotoCondensed, 16),
		// FONT_ITEM(RobotoMedium, 16),
		// FONT_ITEM(SourceSansPro, 18),
	};
	for (auto& [name, fontData, dataSize, fontSize] : fonts)
	{
		ImFontConfig fontConfig         = ImFontConfig();
		fontConfig.FontDataOwnedByAtlas = false;
		strcpy(fontConfig.Name, name);
		auto font       = io.Fonts->AddFontFromMemoryTTF(fontData, dataSize, fontSize * xsettings.uiScale, &fontConfig);
		fontNames[name] = font;
	}

	fontNames["mono"] = io.Fonts->AddFontDefault();

	//ImGui_ImplOpenGL3_CreateFontsTexture();
}

// THEMES
/////////

static void CommonStyle(ImGuiStyle& style)
{
	style.WindowRounding    = 5.0f;
	style.PopupRounding     = 5.0f;
	style.PopupBorderSize   = 0.0f;
	style.FramePadding      = ImVec2(10.0f, 4.0f);
	style.FrameRounding     = 5.0f;
	style.ScrollbarRounding = 12.0f;
	style.GrabRounding      = 12.0f;
}

static void SetThemeClassic(ImGuiStyle& style)
{
	ImGui::StyleColorsClassic(&style);

	CommonStyle(style);
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 0.17f);
}

/// Load colors from a text file
static void SetThemeCustom(ImGuiStyle& style)
{
}

static void SetThemeDark(ImGuiStyle& style)
{
	ImGui::StyleColorsDark(&style);

	CommonStyle(style);
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Border] = ImVec4(1.00f, 1.00f, 1.00f, 0.17f);
}

static void SetThemeLight(ImGuiStyle& style)
{
	ImGui::StyleColorsLight(&style);

	CommonStyle(style);
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.17f);
}

/// Slight modification of the XEMU theme
static void SetThemeXemu(ImGuiStyle& style)
{
	CommonStyle(style);
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]              = ImVec4(0.06f, 0.06f, 0.06f, 0.98f);
	colors[ImGuiCol_ChildBg]               = ImVec4(0.10f, 0.10f, 0.10f, 0.45f);
	colors[ImGuiCol_PopupBg]               = ImVec4(0.16f, 0.16f, 0.16f, 0.90f);
	colors[ImGuiCol_Border]                = ImVec4(0.11f, 0.11f, 0.11f, 0.60f);
	colors[ImGuiCol_BorderShadow]          = ImVec4(0.16f, 0.16f, 0.16f, 0.00f);
	colors[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.28f, 0.71f, 0.25f, 0.78f);
	colors[ImGuiCol_FrameBgActive]         = ImVec4(0.28f, 0.71f, 0.25f, 1.00f);
	colors[ImGuiCol_TitleBg]               = ImVec4(0.17f, 0.44f, 0.15f, 1.00f);
	colors[ImGuiCol_TitleBgActive]         = ImVec4(0.26f, 0.66f, 0.23f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.16f, 0.16f, 0.16f, 0.75f);
	colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.14f, 0.14f, 0.00f);
	colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.20f, 0.51f, 0.18f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.28f, 0.71f, 0.25f, 0.78f);
	colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.28f, 0.71f, 0.25f, 1.00f);
	colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.66f, 0.23f, 1.00f);
	colors[ImGuiCol_SliderGrab]            = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.66f, 0.23f, 1.00f);
	colors[ImGuiCol_Button]                = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	colors[ImGuiCol_ButtonHovered]         = ImVec4(0.28f, 0.71f, 0.25f, 1.00f);
	colors[ImGuiCol_ButtonActive]          = ImVec4(0.26f, 0.66f, 0.23f, 1.00f);
	colors[ImGuiCol_Header]                = ImVec4(0.28f, 0.71f, 0.25f, 0.31f);
	colors[ImGuiCol_HeaderHovered]         = ImVec4(0.28f, 0.71f, 0.25f, 1.00f);
	colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.66f, 0.23f, 1.00f);
	colors[ImGuiCol_Separator]             = ImVec4(0.21f, 0.21f, 0.21f, 0.60f);
	colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.13f, 0.87f, 0.16f, 0.78f);
	colors[ImGuiCol_SeparatorActive]       = ImVec4(0.25f, 0.75f, 0.10f, 1.00f);
	colors[ImGuiCol_ResizeGrip]            = ImVec4(0.47f, 0.83f, 0.49f, 0.04f);
	colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.28f, 0.71f, 0.25f, 0.78f);
	colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.28f, 0.71f, 0.25f, 1.00f);
	colors[ImGuiCol_Tab]                   = ImVec4(0.22f, 0.55f, 0.20f, 0.86f);
	colors[ImGuiCol_TabHovered]            = ImVec4(0.28f, 0.71f, 0.25f, 1.00f);
	colors[ImGuiCol_TabSelected]           = ImVec4(0.26f, 0.66f, 0.23f, 1.00f);
	colors[ImGuiCol_TabDimmed]             = ImVec4(0.19f, 0.49f, 0.17f, 0.97f);
	colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.22f, 0.57f, 0.20f, 1.00f);
	//colors[ImGuiCol_DockingPreview]        = ImVec4(0.26f, 0.66f, 0.23f, 0.70f);
	//colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines]             = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
	colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.28f, 0.71f, 0.25f, 1.00f);
	colors[ImGuiCol_PlotHistogram]         = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
	colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.28f, 0.71f, 0.25f, 1.00f);
	colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg]            = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.09f);
	colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.28f, 0.71f, 0.25f, 0.43f);
	colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavCursor]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.16f, 0.16f, 0.16f, 0.73f);
}

void UpdateTheme()
{
	ImGuiStyle style;

	// clang-format off
	switch (xsettings.theme)
	{
	case Theme_Classic: SetThemeClassic(style); break;
	case Theme_Custom : SetThemeCustom(style); break;
	case Theme_Dark   : SetThemeDark(style); break;
	case Theme_Light  : SetThemeLight(style); break;
	case Theme_Xemu   : SetThemeXemu(style); break;
	}
	// clang-format on

	ImGui::GetStyle() = style;
	ImGui::GetStyle().ScaleAllSizes(xsettings.uiScale);
}

} // namespace ui
