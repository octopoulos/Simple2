// SettingsWindow.cpp
// @author octopoulos
// @version 2025-09-09

#include "stdafx.h"
#include "ui/ui.h"
//
#include "app/App.h" // App

namespace ui
{

// see app:ShowMoreFlags
enum ShowSettingFlags : int
{
	Show_App               = 1 << 0,
	Show_Capture           = 1 << 1,
	Show_Input             = 1 << 2,
	Show_Map               = 1 << 3,
	Show_Net               = 1 << 4,
	Show_NetMain           = 1 << 5,
	Show_NetUser           = 1 << 6,
	Show_Physics           = 1 << 7,
	Show_Render            = 1 << 8,
	Show_System            = 1 << 9, // main
	Show_SystemPerformance = 1 << 10,
	Show_SystemUI          = 1 << 11,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SettingsWindow : public CommonWindow
{
private:
	// int       changed      = 0;
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

		BEGIN_PADDING();
		if (!BeginDraw())
		{
			END_PADDING();
			return;
		}

		const int showTree = xsettings.settingTree;
		int       tree     = showTree & ~(Show_App | Show_Capture | Show_Input | Show_Map | Show_Net | Show_Physics | Show_Render | Show_System);

		// APP
		//////

		BEGIN_COLLAPSE("App", Show_App, 3)
		{
			AddSliderInt(0, "gameId", "Game Id", nullptr);
			if (ImGui::Button("Load Defaults")) LoadGameSettings(xsettings.gameId, "", "-def");
			ImGui::SameLine();
			if (ImGui::Button("Save Defaults")) SaveGameSettings("", true, "-def");
			END_COLLAPSE();
		}

		// CAPTURE
		//////////

		BEGIN_COLLAPSE("Capture", Show_Capture, 3)
		{
			AddInputText(0, "captureDir", "Directory");
			AddCheckbox(0, "captureVideo", "Video", "Enable Video");
			AddCheckbox(0, "nvidiaEnc", "", "Use nVidia Encoding");
			END_COLLAPSE();
		}

		// INPUT
		////////

		BEGIN_COLLAPSE("Input", Show_Input, 5)
		{
			AddDragFloat(0, "cameraSpeed", "Camera Speed", nullptr);
			AddSliderInt(0, "repeatDelay", "Repeat Delay");
			AddSliderInt(0, "repeatInterval", "Repeat Interval");
			AddDragFloat(0, "zoomKb", "Zoom Keyboard", nullptr);
			AddDragFloat(0, "zoomWheel", "Zoom Wheel", nullptr);
			END_COLLAPSE();
		}

		// MAP
		//////

		BEGIN_COLLAPSE("Map", Show_Map, 5)
		{
			AddSliderInt(0, "angleInc", "Angle Increment");
			AddDragFloat(0, "iconSize", "Icon Size", nullptr, 0, 1.0f, "%.0f");
			AddCheckbox(0, "smoothPos", "Smooth",  "Translation");
			AddCheckbox(0, "smoothQuat", "",  "Rotation");
			if (ImGui::Button("Rescan Assets"))
			{
				if (auto app = appWeak.lock())
					app->RescanAssets();
			}
			END_COLLAPSE();
		}

		// NET
		//////

		BEGIN_COLLAPSE("Net", Show_Net, 4)
		{
			AddInputText(0, "userHost", "Host");
			AddInputText(0, "userEmail", "Email");
			AddInputText(0, "userPw", "Password", 256, ImGuiInputTextFlags_Password);
			if (ImGui::Button("Clear Logs")) ClearLog(-1);
			END_COLLAPSE();
		}

		// PHYSICS
		//////////

		BEGIN_COLLAPSE("Physics", Show_Physics, 3)
		{
			AddDragFloat(1, "bottom", "Bottom");
			AddCheckbox(0, "physPaused", "", "Paused");
			AddCheckbox(0, "bulletDebug", "", "Show Body Shapes");
			END_COLLAPSE();
		}

		// RENDER
		/////////

		BEGIN_COLLAPSE("Render", Show_Render, 9)
		{
			AddCheckbox(0, "fixedView", "", "Fixed View");
			AddCheckbox(0, "gridDraw", "", "Grid");
			AddSliderInt(0, "gridSize", "Grid Size");
			AddCheckbox(0, "instancing", "", "Mesh Instancing");
			AddDragFloat(2, "lightDir", "Light Direction");
			AddSliderInt(0, "projection", "Projection", nullptr);
			AddSliderInt(0, "renderMode", "Render Mode", nullptr);
			END_COLLAPSE();
		}

		// SYSTEM
		/////////

		if (ImGui::CollapsingHeader("System", SHOW_TREE(Show_System)))
		{
			tree |= Show_System;
			tree &= ~(Show_SystemPerformance | Show_SystemUI);

			// performance
			BEGIN_TREE("Performance", Show_SystemPerformance, 7)
			{
				AddSliderBool(0, "benchmark", "Benchmark");
				AddSliderInt(0, "drawEvery", "Draw Every");
				AddSliderInt(0, "ioFrameUs", "I/O us");
				AddDragFloat(1, "activeMs", "Active ms");
				AddDragFloat(1, "idleMs", "Idle ms");
				AddDragFloat(1, "idleTimeout", "Idle Timeout");
				AddSliderInt(0, "vsync", "VSync", nullptr);
				END_TREE();
			}

			// ui
			const bool applyChange =
			    xsettings.fullScreen != prevSettings.fullScreen || xsettings.maximized != prevSettings.maximized
			    || memcmp(xsettings.windowPos, prevSettings.windowPos, sizeof(xsettings.windowPos)) != 0
			    || memcmp(xsettings.windowSize, prevSettings.windowSize, sizeof(xsettings.windowSize)) != 0;

			BEGIN_TREE("UI", Show_SystemUI, 10 + (applyChange ? 1 : 0))
			{
				AddCombo(0, "aspectRatio", "Aspect Ratio");
				AddSliderInt(0, "dpr", "DPR");
				if (AddDragFloat(1, "fontScale", "Font Scale", nullptr, 1, 0.001f, "%.3f"))
					ImGui::GetStyle().FontScaleMain = xsettings.fontScale;
				AddSliderInt(0, "fullScreen", "Full Screen", nullptr);
				AddSliderInt(0, "settingPad", "Setting Pad");
				if (AddCombo(0, "theme", "Theme")) UpdateTheme();
				AddDragFloat(1, "uiScale", "UI Scale");
				AddDragInt(1, "windowPos", "Window Pos");
				AddDragInt(1, "windowSize", "Window Size");
				AddCheckbox(0, "maximized", "", "Maximized");

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
