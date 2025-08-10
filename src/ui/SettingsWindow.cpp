// SettingsWindow.cpp
// @author octopoulos
// @version 2025-08-06

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
	Show_App               = 1 << 0, // main
	Show_AppGlobal         = 1 << 1,
	Show_Capture           = 1 << 2, // main
	Show_CaptureFile       = 1 << 3,
	Show_CaptureProcess    = 1 << 4,
	Show_Net               = 1 << 5, // main
	Show_NetMain           = 1 << 6,
	Show_NetUser           = 1 << 7,
	Show_System            = 1 << 8, // main
	Show_SystemPerformance = 1 << 9,
	Show_SystemRender      = 1 << 10,
	Show_SystemUI          = 1 << 11,
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
		int tree = xsettings.settingTree & ~(Show_App | Show_Capture | Show_Net | Show_System);

		// APP
		//////

		ui::AddSpace();
		if (ImGui::CollapsingHeader("App", SHOW_TREE(Show_App)))
		{
			tree |= Show_App;
			tree &= ~Show_AppGlobal;

			// global
			BEGIN_TREE("Global", Show_AppGlobal, 3)
			{
				AddInputText("appId", "AppId", 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
				AddSliderInt("gameId", "Game Id", nullptr);
				if (ImGui::Button("Load Defaults")) LoadGameSettings(xsettings.gameId, "", "-def");
				ImGui::SameLine();
				if (ImGui::Button("Save Defaults")) SaveGameSettings("", true, "-def");
				END_TREE();
			}
		}

		// CAPTURE
		//////////

		ui::AddSpace();
		if (ImGui::CollapsingHeader("Capture", SHOW_TREE(Show_Capture)))
		{
			tree |= Show_Capture;
			tree &= ~(Show_CaptureFile | Show_CaptureProcess);

			// file
			BEGIN_TREE("File", Show_CaptureFile, 1)
			{
				END_TREE();
			}

			// process
			BEGIN_TREE("Process", Show_CaptureProcess, 1)
			{
				END_TREE();
			}
		}

		// NET
		//////

		ui::AddSpace();
		if (ImGui::CollapsingHeader("Net", SHOW_TREE(Show_Net)))
		{
			tree |= Show_Net;
			tree &= ~(Show_NetMain | Show_NetUser);

			// main
			BEGIN_TREE("Main", Show_NetMain, 1)
			{
				//AddDragInt("mousePos", "Mouse Pos");
				if (ImGui::IsMousePosValid())
				{
					const auto& io   = ImGui::GetIO();
					const auto& pos  = io.MousePos;
					//const auto& pos2 = app->GetMousePos();
					//const auto& rect = app->GetScreenRect();
					//ImGui::Text("%.0f %.0f %d - %.0f %.0f %d - %.0f %.0f", pos.x, pos.y, io.MouseDown[0] ? 1 : 0, pos2[0], pos2[1], app->GetMouseButton(), (pos.x - rect[0]) / rect[2], (pos.y - rect[1]) / rect[3]);
					ImGui::Text("%.0f %.0f %d", pos.x, pos.y, io.MouseDown[0] ? 1 : 0);
				}
				else ImGui::TextUnformatted("<Invalid>");

				//AddSliderInt("netDebug", "Net Debug");
				//AddSliderInt("netRecheck", "Net Recheck");
				//AddSliderInt("netTransfers", "Net Transfers");
				END_TREE();
			}

			// user
			BEGIN_TREE("User", Show_NetUser, 4)
			{
				AddInputText("userHost", "Host");
				AddInputText("userEmail", "Email");
				AddInputText("userPw", "Password", 256, ImGuiInputTextFlags_Password);
				if (ImGui::Button("Clear Logs")) ClearLog(-1);
				END_TREE();
			}
		}

		// SYSTEM
		/////////

		ui::AddSpace();
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

			BEGIN_TREE("UI", Show_SystemUI, 11 + (applyChange ? 1 : 0))
			{
				AddCombo("aspectRatio", "Aspect Ratio");
				if (AddDragFloat("fontScale", "Font Scale", 0.001f, "%.3f")) ImGui::GetStyle().FontScaleMain = xsettings.fontScale;
				AddSliderInt("fullScreen", "Full Screen", nullptr);
				AddSliderFloat("iconSize", "Icon Size", "%.0f");
				AddSliderBool("maximized", "Maximized");
				AddSliderInt("settingPad", "Setting Pad");
				AddSliderBool("stretch", "Stretch");
				if (AddCombo("theme", "Theme")) UpdateTheme();
				AddDragFloat("uiScale", "UI Scale");
				AddDragInt("windowPos", "Window Pos");
				AddDragInt("windowSize", "Window Size");
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
