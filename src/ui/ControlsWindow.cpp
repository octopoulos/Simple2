// ControlsWindow.cpp
// @author octopoulos
// @version 2025-08-02

#include "stdafx.h"
#include "ui/ui.h"
#include "ui/xsettings.h"

namespace ui
{

int DrawControlButton(uint32_t texId, const ImVec4& color, std::string name, const char* label, int textButton, float uiScale)
{
	// clang-format off
	static std::unordered_map<std::string, std::tuple<ImVec2, ImVec2>> buttonNames = {
		{ "Config"  , { { 0.0f, 0.0f }, { 0.25f, 0.25f } }},
		{ "FullScr" , { { 0.25f, 0.0f }, { 0.5f, 0.25f } }},
		//{ "FullScr2", { { 0.5f, 0.0f }, { 0.75f, 0.25f } }},
		//{ "Grid"  , { { 0.75f, 0.0f }, { 1.0f, 0.25f } }},
		//{ "List"  , { { 0.0f, 0.25f }, { 0.25f, 0.5f } }},
		{ "Open"    , { { 0.25f, 0.25f }, { 0.5f, 0.5f } }},
		//{ "Pads",     { { 0.5f, 0.25f }, { 0.75f, 0.5f } }},
		{ "Pause"   ,   { { 0.75f, 0.25f }, { 1.0f, 0.5f } }},
		{ "Reset"   ,   { { 0.0f, 0.5f }, { 0.25f, 0.75f } }},
		{ "Start"   ,   { { 0.25f, 0.5f }, { 0.5f, 0.75f } }},
		{ "Stop"   ,    { { 0.5f, 0.5f }, { 0.75f, 0.75f } }},
	};
	// clang-format on

	const auto   scale = std::clamp(uiScale, 1.0f, 2.0f);
	const ImVec2 buttonDims(32.0f * scale, 32.0f * scale);
	const ImVec2 childDims(64.0f * scale, 64.0f * scale);
	const ImVec2 offset(16.0f * scale, 4.0f * scale);

	const auto  nameStr    = label ? label : name.c_str();
	const auto& style      = ImGui::GetStyle();
	const auto  padding    = style.WindowPadding;
	const auto& [uv0, uv1] = buttonNames[name];

	ImGui::BeginChild(nameStr, childDims, true, ImGuiWindowFlags_NoScrollbar);
	const auto pos = ImGui::GetCursorPos() - padding;
	ImGui::SetCursorPos(pos + offset);
	ImGui::ImageWithBg((ImTextureID)(intptr_t)texId, buttonDims, uv0, uv1, color);
	if (textButton)
	{
		const float offset = (childDims.x - ImGui::CalcTextSize(nameStr).x) / 2;
		ImGui::SetCursorPosX(pos.x + offset);
		ImGui::TextUnformatted(nameStr);
	}
	ImGui::EndChild();

	const int flag = ImGui::IsItemClicked() ? 1 : 0;

	// same line?
	{
		const float windowX2 = ImGui::GetWindowPos().x + ImGui::GetContentRegionAvail().x;
		const float lastX2   = ImGui::GetItemRectMax().x;
		const float nextX2   = lastX2 + style.ItemSpacing.x / 2 + childDims.x;
		if (nextX2 < windowX2)
			ImGui::SameLine();
	}

	return flag;
}

class ControlsWindow : public CommonWindow
{
private:
	uint32_t texId = 0;

public:
	ControlsWindow()
	{
		name   = "Controls";
		isOpen = true;
	}

	void Draw()
	{
		CHECK_DRAW();
		if (!drawn)
		{
			//texId = LoadTexture(controls_data, controls_size, "controls");
			++drawn;
		}

		if (!SetAlpha(alpha)) return;

		if (ImGui::Begin("Controls", &isOpen))
		{
			//const auto app = static_cast<App*>(engine.get());
			//app->ScreenFocused(3);

			const auto& style = ImGui::GetStyle();
			const auto  color = style.Colors[ImGuiCol_Text];

			ImGui::PushFont(FindFont("RobotoCondensed"));
			//if (DrawButton(color, "Open")) app->OpenFile(OpenAction_Capture);
			//if (DrawButton(color, "FullScr", "Screen")) app->SourceType(CapType::Screen);
			//if (DrawButton(color, "Stop")) app->SourceType(CapType::None);
			if (DrawButton(color, "Config")) GetSettingsWindow().isOpen ^= 1;
			//if (DrawButton(color, "Reset")) app->JumpToFrame(-1, app->Pause());
			//if (DrawButton(color, app->Pause() ? "Start" : "Pause")) app->Pause(2);
			// if (DrawButton(color, "Pads")) {}
			// if (DrawButton(color, "List")) {}
			// if (DrawButton(color, "Grid")) {}
			ImGui::PopFont();
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	/// Image text button aligned on a row
	int DrawButton(const ImVec4& color, std::string name, const char* label = nullptr)
	{
		return DrawControlButton(texId, color, name, label ? label : name.c_str(), xsettings.textButton, xsettings.uiScale);
	}
};

static ControlsWindow controlsWindow;

CommonWindow& GetControlsWindow() { return controlsWindow; }

} // namespace ui
