// ui-controls.cpp
// @author octopoulos
// @version 2025-07-26

#include "stdafx.h"
#include "ui/ui.h"

namespace ui
{

bool AddMenu(const char* text, const char* shortcut, CommonWindow& window)
{
	const bool clicked = ImGui::MenuItem(text, shortcut, &window.isOpen);
	if (clicked)
	{
		if (window.hidden & 1) window.isOpen = true;
		if (window.isOpen) window.hidden &= ~1;
	}
	return clicked;
}

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

void DrawWindows(const std::vector<CommonWindow*>& windows, bool showImGuiDemo)
{
	for (const auto& window : windows)
		window->Draw();

	if (showImGuiDemo)
		ImGui::ShowDemoWindow(&showImGuiDemo);
}

bool SetAlpha(float alpha)
{
	if (alpha <= 0.0f) return false;

	ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_Text];
	color.w      = alpha;
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::SetNextWindowBgAlpha(alpha);
	return true;
}

bool ShowWindows(const std::vector<CommonWindow*>& windows, bool show, bool force)
{
	bool changed = false;
	for (const auto& window : windows)
	{
		if (show)
		{
			if (window->hidden & 1)
			{
				changed = true;
				window->hidden &= ~1;
			}
		}
		else if (window->hidden == 0 || (window->hidden == 2 && force))
		{
			changed = true;
			window->hidden |= 1;
		}
	}
	return changed;
}

} // namespace ui
