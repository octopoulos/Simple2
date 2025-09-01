// ThemeWindow.cpp
// @author octopoulos
// @version 2025-08-27

#include "stdafx.h"
#include "ui/ui.h"
//
#include "ui/xsettings.h" // xsettings

namespace ui
{

static const char* GetTreeLinesFlagsName(ImGuiTreeNodeFlags flags)
{
	if (flags == ImGuiTreeNodeFlags_DrawLinesNone) return "DrawLinesNone";
	if (flags == ImGuiTreeNodeFlags_DrawLinesFull) return "DrawLinesFull";
	if (flags == ImGuiTreeNodeFlags_DrawLinesToNodes) return "DrawLinesToNodes";
	return "";
}

class ThemeWindow : public CommonWindow
{
public:
	ThemeWindow()
	{
		name = "Theme";
		type = WindowType_Theme;
	}

	void Draw()
	{
		if (!BeginDraw()) return;

		ImGui::Button("Import Custom");
		ImGui::SameLine();
		ImGui::Button("Export Custom");

		auto& style = ImGui::GetStyle();

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
					if (ImGui::Button(name)) ImGui::DebugFlashStyleColor((ImGuiCol)i);
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
				ImGui::SeparatorText("Main");
				{
					ImGui::SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
					ImGui::SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
					ImGui::SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
					ImGui::SliderFloat2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
					ImGui::SliderFloat2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
					ImGui::SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
					ImGui::SliderFloat("ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
					ImGui::SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");
				}
				ImGui::SeparatorText("Borders");
				{
					ImGui::SliderFloat("WindowBorderSize", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
					ImGui::SliderFloat("ChildBorderSize", &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
					ImGui::SliderFloat("PopupBorderSize", &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
					ImGui::SliderFloat("FrameBorderSize", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
				}
				ImGui::SeparatorText("Rounding");
				{
					ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
					ImGui::SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
					ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
					ImGui::SliderFloat("PopupRounding", &style.PopupRounding, 0.0f, 12.0f, "%.0f");
					ImGui::SliderFloat("ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
					ImGui::SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
				}
				ImGui::SeparatorText("Tabs");
				{
					ImGui::SliderFloat("TabBorderSize", &style.TabBorderSize, 0.0f, 1.0f, "%.0f");
					ImGui::SliderFloat("TabBarBorderSize", &style.TabBarBorderSize, 0.0f, 2.0f, "%.0f");
					ImGui::SliderFloat("TabBarOverlineSize", &style.TabBarOverlineSize, 0.0f, 3.0f, "%.0f");
					ImGui::DragFloat("TabCloseButtonMinWidthSelected", &style.TabCloseButtonMinWidthSelected, 0.1f, -1.0f, 100.0f, (style.TabCloseButtonMinWidthSelected < 0.0f) ? "%.0f (Always)" : "%.0f");
					ImGui::DragFloat("TabCloseButtonMinWidthUnselected", &style.TabCloseButtonMinWidthUnselected, 0.1f, -1.0f, 100.0f, (style.TabCloseButtonMinWidthUnselected < 0.0f) ? "%.0f (Always)" : "%.0f");
					ImGui::SliderFloat("TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");
				}
				ImGui::SeparatorText("Tables");
				{
					ImGui::SliderFloat2("CellPadding", (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
					ImGui::SliderAngle("TableAngledHeadersAngle", &style.TableAngledHeadersAngle, -50.0f, +50.0f);
					ImGui::SliderFloat2("TableAngledHeadersTextAlign", (float*)&style.TableAngledHeadersTextAlign, 0.0f, 1.0f, "%.2f");
				}
				ImGui::SeparatorText("Trees");
				{
					if (ImGui::BeginCombo("TreeLinesFlags", GetTreeLinesFlagsName(style.TreeLinesFlags)))
					{
						const ImGuiTreeNodeFlags options[] = { ImGuiTreeNodeFlags_DrawLinesNone, ImGuiTreeNodeFlags_DrawLinesFull, ImGuiTreeNodeFlags_DrawLinesToNodes };
						for (ImGuiTreeNodeFlags option : options)
							if (ImGui::Selectable(GetTreeLinesFlagsName(option), style.TreeLinesFlags == option))
								style.TreeLinesFlags = option;
						ImGui::EndCombo();
					}
					ImGui::SliderFloat("TreeLinesSize", &style.TreeLinesSize, 0.0f, 2.0f, "%.0f");
					ImGui::SliderFloat("TreeLinesRounding", &style.TreeLinesRounding, 0.0f, 12.0f, "%.0f");
				}
				ImGui::SeparatorText("Windows");
				{
					ImGui::SliderFloat2("WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
					ImGui::SliderFloat("WindowBorderHoverPadding", &style.WindowBorderHoverPadding, 1.0f, 20.0f, "%.0f");
					int window_menu_button_position = style.WindowMenuButtonPosition + 1;
					if (ImGui::Combo("WindowMenuButtonPosition", (int*)&window_menu_button_position, "None\0Left\0Right\0"))
						style.WindowMenuButtonPosition = (ImGuiDir)(window_menu_button_position - 1);
				}
				ImGui::SeparatorText("Widgets");
				{
					ImGui::Combo("ColorButtonPosition", (int*)&style.ColorButtonPosition, "Left\0Right\0");
					ImGui::SliderFloat2("ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f");
					ImGui::SliderFloat2("SelectableTextAlign", (float*)&style.SelectableTextAlign, 0.0f, 1.0f, "%.2f");
					ImGui::SliderFloat("SeparatorTextBorderSize", &style.SeparatorTextBorderSize, 0.0f, 10.0f, "%.0f");
					ImGui::SliderFloat2("SeparatorTextAlign", (float*)&style.SeparatorTextAlign, 0.0f, 1.0f, "%.2f");
					ImGui::SliderFloat2("SeparatorTextPadding", (float*)&style.SeparatorTextPadding, 0.0f, 40.0f, "%.0f");
					ImGui::SliderFloat("LogSliderDeadzone", &style.LogSliderDeadzone, 0.0f, 12.0f, "%.0f");
					ImGui::SliderFloat("ImageBorderSize", &style.ImageBorderSize, 0.0f, 1.0f, "%.0f");
				}
				ImGui::SeparatorText("Tooltips");
				{
					for (int n = 0; n < 2; n++)
						if (ImGui::TreeNodeEx(n == 0 ? "HoverFlagsForTooltipMouse" : "HoverFlagsForTooltipNav"))
						{
							ImGuiHoveredFlags* p = (n == 0) ? &style.HoverFlagsForTooltipMouse : &style.HoverFlagsForTooltipNav;
							ImGui::CheckboxFlags("ImGuiHoveredFlags_DelayNone", p, ImGuiHoveredFlags_DelayNone);
							ImGui::CheckboxFlags("ImGuiHoveredFlags_DelayShort", p, ImGuiHoveredFlags_DelayShort);
							ImGui::CheckboxFlags("ImGuiHoveredFlags_DelayNormal", p, ImGuiHoveredFlags_DelayNormal);
							ImGui::CheckboxFlags("ImGuiHoveredFlags_Stationary", p, ImGuiHoveredFlags_Stationary);
							ImGui::CheckboxFlags("ImGuiHoveredFlags_NoSharedDelay", p, ImGuiHoveredFlags_NoSharedDelay);
							ImGui::TreePop();
						}
				}
				ImGui::SeparatorText("Misc");
				{
					ImGui::SliderFloat2("DisplayWindowPadding", (float*)&style.DisplayWindowPadding, 0.0f, 30.0f, "%.0f");
					ImGui::SliderFloat2("DisplaySafeAreaPadding", (float*)&style.DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f");
				}
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}
};

static ThemeWindow themeWindow;

CommonWindow& GetThemeWindow() { return themeWindow; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

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
	ImGuiIO& io = ImGui::GetIO();
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
	style.FontScaleDpi      = xsettings.dpr;
	style.FontScaleMain     = xsettings.fontScale;
	style.FramePadding      = ImVec2(10.0f, 4.0f);
	style.FrameRounding     = 5.0f;
	style.GrabRounding      = 12.0f;
	style.PopupBorderSize   = 0.0f;
	style.PopupRounding     = 5.0f;
	style.ScrollbarRounding = 12.0f;
	style.WindowPadding     = ImVec2(8.0f, 8.0f);
	style.WindowRounding    = 5.0f;
}

/// Blender theme approximation
static void SetThemeBlender(ImGuiStyle& style)
{
	CommonStyle(style);
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Text]                      = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_TextDisabled]              = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
	colors[ImGuiCol_WindowBg]                  = ImVec4(0.190f, 0.190f, 0.190f, 0.880f); // 48 48 48 224
	colors[ImGuiCol_ChildBg]                   = ImVec4(1.000f, 1.000f, 1.000f, 0.055f); // 255 255 255 15
	colors[ImGuiCol_PopupBg]                   = ImVec4(0.090f, 0.090f, 0.090f, 0.980f); // 24 24 24 255
	colors[ImGuiCol_Border]                    = ImVec4(0.110f, 0.110f, 0.110f, 0.600f);
	colors[ImGuiCol_BorderShadow]              = ImVec4(0.160f, 0.160f, 0.160f, 0.000f);
	colors[ImGuiCol_FrameBg]                   = ImVec4(0.330f, 0.330f, 0.330f, 0.880f); // 84 84 84 224
	colors[ImGuiCol_FrameBgHovered]            = ImVec4(0.470f, 0.470f, 0.470f, 0.880f); // 121 121 121 224
	colors[ImGuiCol_FrameBgActive]             = ImVec4(0.160f, 0.160f, 0.160f, 0.880f); // 41 41 41 224
	colors[ImGuiCol_TitleBg]                   = ImVec4(0.190f, 0.190f, 0.190f, 0.880f); // 48 48 48 224
	colors[ImGuiCol_TitleBgActive]             = ImVec4(0.320f, 0.320f, 0.320f, 0.880f); // 82 82 82 224
	colors[ImGuiCol_TitleBgCollapsed]          = ImVec4(0.160f, 0.160f, 0.160f, 0.750f);
	colors[ImGuiCol_MenuBarBg]                 = ImVec4(0.090f, 0.090f, 0.090f, 1.000f); // 24 24 24 255
	colors[ImGuiCol_ScrollbarBg]               = ImVec4(0.190f, 0.190f, 0.190f, 0.880f); // 48 48 48 224
	colors[ImGuiCol_ScrollbarGrab]             = ImVec4(0.240f, 0.240f, 0.240f, 0.880f); // 62 62 62 224
	colors[ImGuiCol_ScrollbarGrabHovered]      = ImVec4(0.330f, 0.330f, 0.330f, 0.880f); // 84 84 84 224
	colors[ImGuiCol_ScrollbarGrabActive]       = ImVec4(0.330f, 0.330f, 0.330f, 0.880f); // 84 84 84 224
	colors[ImGuiCol_CheckMark]                 = ImVec4(0.280f, 0.470f, 0.750f, 0.880f); // 71 114 179 224
	colors[ImGuiCol_SliderGrab]                = ImVec4(0.280f, 0.470f, 0.750f, 0.880f); // 71 114 179 224
	colors[ImGuiCol_SliderGrabActive]          = ImVec4(0.280f, 0.470f, 0.750f, 1.000f); // 71 114 179 255
	colors[ImGuiCol_Button]                    = ImVec4(0.330f, 0.330f, 0.330f, 1.000f); // 84 84 84 255
	colors[ImGuiCol_ButtonHovered]             = ImVec4(0.400f, 0.400f, 0.400f, 1.000f); // 101 101 101 255
	colors[ImGuiCol_ButtonActive]              = ImVec4(0.280f, 0.470f, 0.750f, 1.000f); // 71 114 179 255
	colors[ImGuiCol_Header]                    = ImVec4(0.240f, 0.240f, 0.240f, 0.880f); // 61 61 61 224
	colors[ImGuiCol_HeaderHovered]             = ImVec4(0.240f, 0.240f, 0.240f, 0.880f); // 61 61 61 224
	colors[ImGuiCol_HeaderActive]              = ImVec4(0.240f, 0.240f, 0.240f, 0.880f); // 61 61 61 224
	colors[ImGuiCol_Separator]                 = ImVec4(0.210f, 0.210f, 0.210f, 0.600f);
	colors[ImGuiCol_SeparatorHovered]          = ImVec4(0.280f, 0.470f, 0.750f, 0.880f); // 71 114 179 224
	colors[ImGuiCol_SeparatorActive]           = ImVec4(0.280f, 0.470f, 0.750f, 1.000f); // 71 114 179 255
	colors[ImGuiCol_ResizeGrip]                = ImVec4(0.280f, 0.470f, 0.750f, 0.040f); //
	colors[ImGuiCol_ResizeGripHovered]         = ImVec4(0.280f, 0.470f, 0.750f, 0.880f); // 71 114 179 224
	colors[ImGuiCol_ResizeGripActive]          = ImVec4(0.280f, 0.470f, 0.750f, 1.000f); // 71 114 179 255
	colors[ImGuiCol_InputTextCursor]           = ImVec4(0.440f, 0.660f, 1.000f, 1.000f); // 113 168 255 255
	colors[ImGuiCol_TabHovered]                = ImVec4(0.140f, 0.140f, 0.140f, 0.880f); // 35 35 35 224
	colors[ImGuiCol_Tab]                       = ImVec4(0.110f, 0.110f, 0.110f, 0.880f); // 29 29 29 224
	colors[ImGuiCol_TabSelected]               = ImVec4(0.190f, 0.190f, 0.190f, 0.880f); // 48 48 48 224
	colors[ImGuiCol_TabSelectedOverline]       = ImVec4(0.280f, 0.470f, 0.750f, 0.880f); // 71 114 179 224
	colors[ImGuiCol_TabDimmed]                 = ImVec4(0.100f, 0.100f, 0.100f, 0.880f); // 26 26 26 224
	colors[ImGuiCol_TabDimmedSelected]         = ImVec4(0.180f, 0.180f, 0.180f, 0.880f); // 46 46 46 224
	colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.280f, 0.470f, 0.750f, 0.880f); // 71 114 179 224
	colors[ImGuiCol_DockingPreview]            = ImVec4(0.280f, 0.470f, 0.750f, 0.880f); // 71 114 179 224
	colors[ImGuiCol_DockingEmptyBg]            = ImVec4(0.200f, 0.200f, 0.200f, 0.500f); //
	colors[ImGuiCol_PlotLines]                 = ImVec4(0.860f, 0.930f, 0.890f, 0.630f);
	colors[ImGuiCol_PlotLinesHovered]          = ImVec4(0.280f, 0.710f, 0.250f, 1.000f);
	colors[ImGuiCol_PlotHistogram]             = ImVec4(0.860f, 0.930f, 0.890f, 0.630f);
	colors[ImGuiCol_PlotHistogramHovered]      = ImVec4(0.280f, 0.710f, 0.250f, 1.000f);
	colors[ImGuiCol_TableHeaderBg]             = ImVec4(0.190f, 0.190f, 0.200f, 1.000f);
	colors[ImGuiCol_TableBorderStrong]         = ImVec4(0.310f, 0.310f, 0.350f, 1.000f);
	colors[ImGuiCol_TableBorderLight]          = ImVec4(0.230f, 0.230f, 0.250f, 1.000f);
	colors[ImGuiCol_TableRowBg]                = ImVec4(0.152f, 0.152f, 0.152f, 0.880f); // 40 40 40 224
	colors[ImGuiCol_TableRowBgAlt]             = ImVec4(0.165f, 0.165f, 0.165f, 0.880f); // 43 43 43 224
	colors[ImGuiCol_TextLink]                  = ImVec4(0.280f, 0.470f, 0.750f, 0.880f); // 71 114 179 224
	colors[ImGuiCol_TextSelectedBg]            = ImVec4(0.280f, 0.470f, 0.750f, 0.880f); // 71 114 179 224
	colors[ImGuiCol_TreeLines]                 = ImVec4(0.400f, 0.400f, 0.400f, 1.000f); // 102 102 102 255
	colors[ImGuiCol_DragDropTarget]            = ImVec4(1.000f, 1.000f, 0.000f, 0.900f);
	colors[ImGuiCol_NavCursor]                 = ImVec4(0.260f, 0.590f, 0.980f, 1.000f);
	colors[ImGuiCol_NavWindowingHighlight]     = ImVec4(1.000f, 1.000f, 1.000f, 0.700f);
	colors[ImGuiCol_NavWindowingDimBg]         = ImVec4(0.800f, 0.800f, 0.800f, 0.200f);
	colors[ImGuiCol_ModalWindowDimBg]          = ImVec4(0.160f, 0.160f, 0.160f, 0.730f);

	style.ChildRounding = 3.0f;
	style.FramePadding  = ImVec2(10.0f, 3.0f);
	style.FrameRounding = 3.0f;
	style.GrabRounding  = 2.0f;
	style.ScrollbarSize = 10.0f;

	if (const std::filesystem::path interFont = "runtime/fonts/inter.ttf"; IsFile(interFont))
	{
		ImGuiIO& io    = ImGui::GetIO();
		io.FontDefault = io.Fonts->AddFontFromFileTTF(interFont.string().c_str());
	}
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

	colors[ImGuiCol_Text]                  = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_TextDisabled]          = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
	colors[ImGuiCol_WindowBg]              = ImVec4(0.060f, 0.060f, 0.060f, 0.980f);
	colors[ImGuiCol_ChildBg]               = ImVec4(0.100f, 0.100f, 0.100f, 0.450f);
	colors[ImGuiCol_PopupBg]               = ImVec4(0.160f, 0.160f, 0.160f, 0.900f);
	colors[ImGuiCol_Border]                = ImVec4(0.110f, 0.110f, 0.110f, 0.600f);
	colors[ImGuiCol_BorderShadow]          = ImVec4(0.160f, 0.160f, 0.160f, 0.000f);
	colors[ImGuiCol_FrameBg]               = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.280f, 0.710f, 0.250f, 0.780f);
	colors[ImGuiCol_FrameBgActive]         = ImVec4(0.280f, 0.710f, 0.250f, 1.000f);
	colors[ImGuiCol_TitleBg]               = ImVec4(0.170f, 0.440f, 0.150f, 1.000f);
	colors[ImGuiCol_TitleBgActive]         = ImVec4(0.260f, 0.660f, 0.230f, 1.000f);
	colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.160f, 0.160f, 0.160f, 0.750f);
	colors[ImGuiCol_MenuBarBg]             = ImVec4(0.140f, 0.140f, 0.140f, 0.000f);
	colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.200f, 0.510f, 0.180f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.280f, 0.710f, 0.250f, 0.780f);
	colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.280f, 0.710f, 0.250f, 1.000f);
	colors[ImGuiCol_CheckMark]             = ImVec4(0.260f, 0.660f, 0.230f, 1.000f);
	colors[ImGuiCol_SliderGrab]            = ImVec4(0.260f, 0.260f, 0.260f, 1.000f);
	colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.260f, 0.660f, 0.230f, 1.000f);
	colors[ImGuiCol_Button]                = ImVec4(0.360f, 0.360f, 0.360f, 1.000f);
	colors[ImGuiCol_ButtonHovered]         = ImVec4(0.280f, 0.710f, 0.250f, 1.000f);
	colors[ImGuiCol_ButtonActive]          = ImVec4(0.260f, 0.660f, 0.230f, 1.000f);
	colors[ImGuiCol_Header]                = ImVec4(0.280f, 0.710f, 0.250f, 0.310f);
	colors[ImGuiCol_HeaderHovered]         = ImVec4(0.280f, 0.710f, 0.250f, 1.000f);
	colors[ImGuiCol_HeaderActive]          = ImVec4(0.260f, 0.660f, 0.230f, 1.000f);
	colors[ImGuiCol_Separator]             = ImVec4(0.210f, 0.210f, 0.210f, 0.600f);
	colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.130f, 0.870f, 0.160f, 0.780f);
	colors[ImGuiCol_SeparatorActive]       = ImVec4(0.250f, 0.750f, 0.100f, 1.000f);
	colors[ImGuiCol_ResizeGrip]            = ImVec4(0.470f, 0.830f, 0.490f, 0.040f);
	colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.280f, 0.710f, 0.250f, 0.780f);
	colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.280f, 0.710f, 0.250f, 1.000f);
	colors[ImGuiCol_Tab]                   = ImVec4(0.220f, 0.550f, 0.200f, 0.860f);
	colors[ImGuiCol_TabHovered]            = ImVec4(0.280f, 0.710f, 0.250f, 1.000f);
	colors[ImGuiCol_TabSelected]           = ImVec4(0.260f, 0.660f, 0.230f, 1.000f);
	colors[ImGuiCol_TabDimmed]             = ImVec4(0.190f, 0.490f, 0.170f, 0.970f);
	colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.220f, 0.570f, 0.200f, 1.000f);
	colors[ImGuiCol_DockingPreview]        = ImVec4(0.260f, 0.660f, 0.230f, 0.700f);
	colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
	colors[ImGuiCol_PlotLines]             = ImVec4(0.860f, 0.930f, 0.890f, 0.630f);
	colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.280f, 0.710f, 0.250f, 1.000f);
	colors[ImGuiCol_PlotHistogram]         = ImVec4(0.860f, 0.930f, 0.890f, 0.630f);
	colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.280f, 0.710f, 0.250f, 1.000f);
	colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.190f, 0.190f, 0.200f, 1.000f);
	colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.310f, 0.310f, 0.350f, 1.000f);
	colors[ImGuiCol_TableBorderLight]      = ImVec4(0.230f, 0.230f, 0.250f, 1.000f);
	colors[ImGuiCol_TableRowBg]            = ImVec4(1.000f, 1.000f, 1.000f, 0.060f);
	colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.000f, 1.000f, 1.000f, 0.090f);
	colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.280f, 0.710f, 0.250f, 0.430f);
	colors[ImGuiCol_DragDropTarget]        = ImVec4(1.000f, 1.000f, 0.000f, 0.900f);
	colors[ImGuiCol_NavCursor]             = ImVec4(0.260f, 0.590f, 0.980f, 1.000f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 1.000f, 1.000f, 0.700f);
	colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.800f, 0.800f, 0.800f, 0.200f);
	colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.160f, 0.160f, 0.160f, 0.730f);
}

void UpdateTheme()
{
	ImGuiStyle style;

	// clang-format off
	switch (xsettings.theme)
	{
	case Theme_Blender: SetThemeBlender(style); break;
	case Theme_Classic: SetThemeClassic(style); break;
	case Theme_Custom : SetThemeCustom (style); break;
	case Theme_Dark   : SetThemeDark   (style); break;
	case Theme_Light  : SetThemeLight  (style); break;
	case Theme_Xemu   : SetThemeXemu   (style); break;
	}
	// clang-format on

	ImGui::GetStyle() = style;
	ImGui::GetStyle().ScaleAllSizes(xsettings.dpr * xsettings.uiScale);
}

} // namespace ui
