// ui.cpp
// @author octopoulos
// @version 2025-08-03

#include "stdafx.h"
#include "ui.h"

namespace ui
{

static std::vector<CommonWindow*> windows;
static UMAP_STR<CommonWindow*>    windowNames;

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

void DrawWindows()
{
	for (const auto& window : windows)
		window->Draw();
}

void ListWindows(App* app)
{
	if (windows.size()) return;

	// clang-format off
	windows.push_back(&GetControlsWindow());
	windows.push_back(&GetLogWindow     ());
	windows.push_back(&GetSceneWindow   ());
	windows.push_back(&GetSettingsWindow());
	windows.push_back(&GetThemeWindow   ());
	// clang-format on

	for (const auto& window : windows)
	{
		window->app               = app;
		windowNames[window->name] = window;
	}
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

bool ShowWindows(bool show, bool force)
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
