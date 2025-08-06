// ui-settings.cpp
// @author octopoulos
// @version 2025-08-02

#include "stdafx.h"
#include "ui/ui.h"
//
#include "app/App.h"

//extern std::unique_ptr<Engine> engine;

namespace ui
{

#define ACTIVATE_RECT(id)                                \
	if (ImGui::IsItemActive() || ImGui::IsItemHovered()) \
	app->ActivateRectangle(id)

// see app:ShowMoreFlags
enum ShowFlags : int
{
	Show_Analysis          = 1 << 0, // main
	Show_AnalysisGlobal    = 1 << 1,
	Show_Capture           = 1 << 2, // main
	Show_CaptureDevice     = 1 << 3,
	Show_CaptureFile       = 1 << 4,
	Show_CaptureFolder     = 1 << 5,
	Show_CaptureOcr        = 1 << 6,
	Show_CaptureProcess    = 1 << 7,
	Show_Net               = 1 << 8, // main
	Show_NetMain           = 1 << 9,
	Show_NetUser           = 1 << 10,
	Show_System            = 1 << 11, // main
	Show_SystemPerformance = 1 << 12,
	Show_SystemRender      = 1 << 13,
	Show_SystemUI          = 1 << 14,
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
		name   = "Settings";
		isOpen = true;
		ui::Log("SettingsWindow: tree={}", xsettings.tree);
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
		if (!ImGui::Begin("Settings", &isOpen))
		{
			ImGui::End();
			return;
		}

		//const auto app = static_cast<App*>(engine.get());
		//app->ScreenFocused(0);
		int tree = xsettings.tree & ~(Show_Analysis | Show_Capture | Show_Net | Show_System);

		// ANALYSIS
		///////////

		ui::AddSpace();
		if (ImGui::CollapsingHeader("Analysis", SHOW_TREE(Show_Analysis)))
		{
			tree |= Show_Analysis;

			// global
			if (ImGui::TreeNodeEx("Global", SHOW_TREE(Show_AnalysisGlobal)))
			{
				tree |= Show_AnalysisGlobal;
				ImGui::InputText("App Id", xsettings.appId, sizeof(xsettings.appId), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
				AddSliderInt("gameId", "Game Id", nullptr);
				if (ImGui::Button("Load Defaults")) LoadGameSettings(xsettings.gameId, "", "-def");
				ImGui::SameLine();
				if (ImGui::Button("Save Defaults")) SaveGameSettings("", true, "-def");
				ImGui::TreePop();
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
			if (ImGui::TreeNodeEx("Main", SHOW_TREE(Show_NetMain)))
			{
				tree |= Show_NetMain;
				//AddDragInt("mousePos", "Mouse Pos");
				if (ImGui::IsMousePosValid())
				{
					const auto& io   = ImGui::GetIO();
					const auto& pos  = io.MousePos;
					//const auto& pos2 = app->GetMousePos();
					//const auto& rect = app->GetScreenRect();
					//ImGui::Text("%.0f %.0f %d - %.0f %.0f %d - %.0f %.0f", pos.x, pos.y, io.MouseDown[0] ? 1 : 0, pos2[0], pos2[1], app->GetMouseButton(), (pos.x - rect[0]) / rect[2], (pos.y - rect[1]) / rect[3]);
				}
				else ImGui::TextUnformatted("<Invalid>");

				//AddSliderInt("netDebug", "Net Debug");
				//AddSliderInt("netRecheck", "Net Recheck");
				//AddSliderInt("netTransfers", "Net Transfers");
				ImGui::TreePop();
			}

			// user
			if (ImGui::TreeNodeEx("User", SHOW_TREE(Show_NetUser)))
			{
				tree |= Show_NetUser;
				ImGui::InputText("Host", xsettings.userHost, sizeof(xsettings.userHost));
				ImGui::InputText("Email", xsettings.userEmail, sizeof(xsettings.userEmail));
				ImGui::InputText("Password", xsettings.userPw, sizeof(xsettings.userPw), ImGuiInputTextFlags_Password);
				//if (ImGui::Button("UserLogin")) app->UserLogin();
				ImGui::SameLine();
				//if (ImGui::Button("UserGet")) app->UserGet();
				ImGui::SameLine();
				//if (ImGui::Button("SystemGet")) app->SystemGet();
				//if (ImGui::Button("LogTest")) app->LogTest();
				ImGui::SameLine();
				//if (ImGui::Button("MailAdd")) app->MailAdd(0);
				ImGui::SameLine();
				//if (ImGui::Button("MailOcr")) app->MailOcr();
				if (ImGui::Button("Clear Logs")) ClearLog(-1);
				ImGui::SameLine();
				//if (ImGui::Button("Toggle Czur")) app->FindCaptureWindow();
				//ImGui::InputInt("Eid Flags", &xsettings.userEid);
				//if (ImGui::Button("EidCard")) app->UserInfo(InfoState_RequestInfo);
				ImGui::TreePop();
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
			if (ImGui::TreeNodeEx("Performance", SHOW_TREE(Show_SystemPerformance)))
			{
				tree |= Show_SystemPerformance;
				AddSliderBool("benchmark", "Benchmark");
				AddSliderInt("drawEvery", "Draw Every");
				AddSliderInt("ioFrameUs", "I/O us");
				AddDragFloat("activeMs", "Active ms");
				AddDragFloat("idleMs", "Idle ms");
				AddDragFloat("idleTimeout", "Idle Timeout");
				AddSliderInt("vsync", "VSync", nullptr);
				ImGui::TreePop();
			}

			// render
			if (ImGui::TreeNodeEx("Render", SHOW_TREE(Show_SystemRender)))
			{
				tree |= Show_SystemRender;
				AddDragFloat("center", "Center");
				AddDragFloat("eye", "Eye");
				AddSliderBool("fixedView", "Fixed View");
				AddSliderInt("projection", "Projection", nullptr);
				AddSliderInt("renderMode", "Render Mode", nullptr);
				ImGui::TreePop();
			}

			// ui
			if (ImGui::TreeNodeEx("UI", SHOW_TREE(Show_SystemUI)))
			{
				tree |= Show_SystemUI;
				AddSliderInt("aspectRatio", "Aspect Ratio", nullptr);
				if (AddDragFloat("fontScale", "Font Scale", 0.001f, "%.3f")) ImGui::GetStyle().FontScaleMain = xsettings.fontScale;
				AddSliderInt("fullScreen", "Full Screen", nullptr);
				AddSliderFloat("iconSize", "Icon Size", "%.0f");
				AddSliderBool("maximized", "Maximized");
				AddSliderBool("stretch", "Stretch");
				if (AddCombo("theme", "Theme")) UpdateTheme();
				AddDragFloat("uiScale", "UI Scale");
				ImGui::DragInt2("Window Pos", xsettings.windowPos, 1, -1, 5120);
				ItemEvent("windowPos");
				ImGui::DragInt2("Window Size", xsettings.windowSize, 1, 0, 5120);
				ItemEvent("windowSize");

				if (xsettings.fullScreen != prevSettings.fullScreen || xsettings.maximized != prevSettings.maximized
				    || memcmp(xsettings.windowPos, prevSettings.windowPos, sizeof(xsettings.windowPos)) != 0
				    || memcmp(xsettings.windowSize, prevSettings.windowSize, sizeof(xsettings.windowSize)) != 0)
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
				ImGui::TreePop();
			}
		}

		xsettings.tree = tree;

		ImGui::End();
	}
};

static SettingsWindow settingsWindow;

CommonWindow& GetSettingsWindow() { return settingsWindow; }

} // namespace ui
