// SettingsWindow.cpp
// @author octopoulos
// @version 2025-08-21

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
enum ShowSettingFlags : int
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
		BEGIN_PADDING();

		if (!ImGui::Begin("Settings", &isOpen))
		{
			ImGui::End();
			END_PADDING();
			return;
		}

		const int showTree = xsettings.settingTree;
		int       tree     = showTree & ~(Show_App | Show_Capture | Show_Map | Show_Net | Show_System);

		// APP
		//////

		// ui::AddSpace();
		BEGIN_COLLAPSE("App", Show_App, 3)
		{
			//AddInputText("appId", "AppId", 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
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

		BEGIN_COLLAPSE("Map", Show_Map, 5)
		{
			AddSliderInt("angleInc", "Angle Increment");
			AddSliderFloat("iconSize", "Icon Size", "%.0f");
			AddCheckBox("smoothPos", "Smooth",  "Translation");
			AddCheckBox("smoothQuat", "",  "Rotation");
			if (ImGui::Button("Rescan Assets")) app->RescanAssets();
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
			BEGIN_TREE("Render", Show_SystemRender, 5)
			{
				AddCheckBox("fixedView", "", "Fixed View");
				AddCheckBox("gridDraw", "", "Grid");
				AddSliderInt("gridSize", "Grid Size");
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
				AddSliderInt("dpr", "DPR");
				if (AddDragFloat("fontScale", "Font Scale", 0.001f, "%.3f")) ImGui::GetStyle().FontScaleMain = xsettings.fontScale;
				AddSliderInt("fullScreen", "Full Screen", nullptr);
				AddSliderInt("settingPad", "Setting Pad");
				if (AddCombo("theme", "Theme")) UpdateTheme();
				AddDragFloat("uiScale", "UI Scale");
				AddDragInt("windowPos", "Window Pos");
				AddDragInt("windowSize", "Window Size");
				AddCheckBox("maximized", "", "Maximized");

				if (applyChange)
				{
					if (ImGui::Button("Apply Changes"))
					{
						// int      winX;
						// int      winY;
						// uint32_t fullFlags;
						memcpy(&prevSettings, &xsettings, sizeof(XSettings));
					}
				}
				END_TREE();
			}
		}

		xsettings.settingTree = tree;

		ImGui::End();
		END_PADDING();
	}
};

static SettingsWindow settingsWindow;

CommonWindow& GetSettingsWindow() { return settingsWindow; }

} // namespace ui
