// SettingsWindow.cpp
// @author octopoulos
// @version 2025-08-08

#include "stdafx.h"
#include "ui/ui.h"
//
#include "app/App.h"

namespace ui
{

#define ACTIVATE_RECT(id)                                \
	if (ImGui::IsItemActive() || ImGui::IsItemHovered()) \
	app->ActivateRectangle(id)

// see app:ShowMoreFlags
enum ShowFlags : int
{
	Show_App               = 1 << 0,
	Show_Capture           = 1 << 1,
	Show_Map               = 1 << 2,
	Show_Net               = 1 << 3,
	Show_NetMain           = 1 << 4,
	Show_NetUser           = 1 << 5,
	Show_System            = 1 << 6, // main
	Show_SystemPerformance = 1 << 7,
	Show_SystemRender      = 1 << 8,
	Show_SystemUI          = 1 << 9,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SettingsWindow : public CommonWindow
{
private:
	int       changed      = 0;
	XSettings prevSettings = {};

public:
	SettingsWindow()
	{
		name = "Settings";
		type = WindowType_Settings;
		ui::Log("SettingsWindow: settingTree={}", xsettings.settingTree);
	}

	void Draw()
	{
		static int init;
		if (!init)
		{
			memcpy(&prevSettings, &xsettings, sizeof(XSettings));
			init = 1;
		}

		CHECK_DRAW();

		auto&      style      = ImGui::GetStyle();
		const auto paddingX   = style.WindowPadding.x;
		const int  settingPad = xsettings.settingPad;
		if (settingPad >= 0) ImGui::PushStyleVarX(ImGuiStyleVar_WindowPadding, xsettings.settingPad);

		if (!ImGui::Begin("Settings", &isOpen))
		{
			ImGui::End();
			ImGui::PopStyleVar();
			return;
		}

		//const auto app = static_cast<App*>(engine.get());
		//app->ScreenFocused(0);
		int tree = xsettings.settingTree & ~(Show_App | Show_Capture | Show_Map | Show_Net | Show_System);

		// APP
		//////

		// ui::AddSpace();
		BEGIN_COLLAPSE("App", Show_App, 3)
		{
			AddInputText("appId", "AppId", 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
			AddSliderInt("gameId", "Game Id", nullptr);
			if (ImGui::Button("Load Defaults")) LoadGameSettings(xsettings.gameId, "", "-def");
			ImGui::SameLine();
			if (ImGui::Button("Save Defaults")) SaveGameSettings("", true, "-def");
			END_COLLAPSE();
		}

		// CAPTURE
		//////////

		// ui::AddSpace();
		BEGIN_COLLAPSE("Capture", Show_Capture, 3)
		{
			AddInputText("captureDir", "Directory");
			AddCheckBox("captureVideo", "Video", "Enable Video");
			AddCheckBox("nvidiaEnc", "", "Use nVidia Encoding");
			END_COLLAPSE();
		}

		// MAP
		//////

		BEGIN_COLLAPSE("Map", Show_Map, 3)
		{
			// AddInputText("captureDir", "Directory");
			// AddCheckBox("captureVideo", "", "Enable Video");
			// AddCheckBox("nvidiaEnc", "", "Use nVidia Encoding");
			END_COLLAPSE();
		}

		// NET
		//////

		BEGIN_COLLAPSE("Net", Show_Net, 4)
		{
			AddInputText("userHost", "Host");
			AddInputText("userEmail", "Email");
			AddInputText("userPw", "Password", 256, ImGuiInputTextFlags_Password);
			if (ImGui::Button("Clear Logs")) ClearLog(-1);
			END_COLLAPSE();
		}

		// SYSTEM
		/////////

		// ui::AddSpace();
		if (ImGui::CollapsingHeader("System", SHOW_TREE(Show_System)))
		{
			tree |= Show_System;
			tree &= ~(Show_SystemPerformance | Show_SystemRender | Show_SystemUI);

			// performance
			BEGIN_TREE("Performance", Show_SystemPerformance, 7)
			{
				AddSliderBool("benchmark", "Benchmark");
				AddSliderInt("drawEvery", "Draw Every");
				AddSliderInt("ioFrameUs", "I/O us");
				AddDragFloat("activeMs", "Active ms");
				AddDragFloat("idleMs", "Idle ms");
				AddDragFloat("idleTimeout", "Idle Timeout");
				AddSliderInt("vsync", "VSync", nullptr);
				END_TREE();
			}

			// render
			BEGIN_TREE("Render", Show_SystemRender, 3)
			{
				AddSliderBool("fixedView", "Fixed View");
				AddSliderInt("projection", "Projection", nullptr);
				AddSliderInt("renderMode", "Render Mode", nullptr);
				END_TREE();
			}

			// ui
			const bool applyChange =
			    xsettings.fullScreen != prevSettings.fullScreen || xsettings.maximized != prevSettings.maximized
			    || memcmp(xsettings.windowPos, prevSettings.windowPos, sizeof(xsettings.windowPos)) != 0
			    || memcmp(xsettings.windowSize, prevSettings.windowSize, sizeof(xsettings.windowSize)) != 0;

			BEGIN_TREE("UI", Show_SystemUI, 10 + (applyChange ? 1 : 0))
			{
				AddCombo("aspectRatio", "Aspect Ratio");
				if (AddDragFloat("fontScale", "Font Scale", 0.001f, "%.3f")) ImGui::GetStyle().FontScaleMain = xsettings.fontScale;
				AddSliderInt("fullScreen", "Full Screen", nullptr);
				AddSliderFloat("iconSize", "Icon Size", "%.0f");
				AddSliderInt("settingPad", "Setting Pad");
				if (AddCombo("theme", "Theme")) UpdateTheme();
				AddDragFloat("uiScale", "UI Scale");
				AddDragInt("windowPos", "Window Pos");
				AddDragInt("windowSize", "Window Size");
				AddCheckBox("maximized", "", "Maximized");
				//ImGui::DragInt2("Window Pos", xsettings.windowPos, 1, -1, 5120);
				//ItemEvent("windowPos");
				//ImGui::DragInt2("Window Size", xsettings.windowSize, 1, 0, 5120);
				//ItemEvent("windowSize");

				if (applyChange)
				{
					if (ImGui::Button("Apply Changes"))
					{
						int      winX;
						int      winY;
						uint32_t fullFlags;
						//app->GetNewPositionFlags(winX, winY, &fullFlags, nullptr);

						//auto window = app->GetWindow();
						//if (xsettings.maximized)
						//	SDL_MaximizeWindow(window);
						//else
						//{
						//	SDL_SetWindowFullscreen(window, fullFlags);
						//	if (!fullFlags) SDL_RestoreWindow(window);
						//	SDL_SetWindowSize(window, xsettings.windowSize[0], xsettings.windowSize[1]);
						//	SDL_SetWindowPosition(window, winX, winY);
						//}
						memcpy(&prevSettings, &xsettings, sizeof(XSettings));
					}
				}
				END_TREE();
			}
		}

		xsettings.settingTree = tree;

		ImGui::End();
		if (settingPad >= 0) ImGui::PopStyleVar();
	}
};

static SettingsWindow settingsWindow;

CommonWindow& GetSettingsWindow() { return settingsWindow; }

} // namespace ui
