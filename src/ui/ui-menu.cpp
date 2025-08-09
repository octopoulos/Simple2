// menu.cpp
// @author octopoulos
// @version 2025-08-05

#include "stdafx.h"
#include "app/App.h"
#include "ui/ui.h"

#include "common/imgui/imgui.h"
#include "entry/input.h"

#include "ImGuiFileDialog/ImGuiFileDialog.h"

#ifdef WITH_FX
void FxTestBed();
#endif // WITH_FX

enum OpenActions_ : int
{
	OpenAction_None      = 0,
	OpenAction_OpenScene = 1,
	OpenAction_SaveScene = 2,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// APP
//////

/// https://github.com/octopoulos/ImGuiFileDialog/blob/master/Documentation.md
void App::FilesUi()
{
	if (auto instance = ImGuiFileDialog::Instance(); instance->Display("OpenFileX", 0, { 768.0f, 512.0f }))
	{
		if (instance->IsOk())
		{
			fileFolder                = instance->GetCurrentPath();
			actionFolders[fileAction] = fileFolder;
			OpenedFile(fileAction, instance->GetFilePathName());
		}
		instance->Close();
	}
}

int App::MainUi()
{
	// lock => no UI
	if (GetGlobalInput().mouseLock) return 0;

	// show menu (or [REC] if recording video)
	ShowMainMenu(1.0f);

	// skip some UI elements when recording video
	int drawnFlag = 1;
	if (!wantVideo)
	{
		drawnFlag |= 2;

		MapUi();
		FilesUi();

		ui::DrawWindows();
		if (showImGuiDemo) ImGui::ShowDemoWindow(&showImGuiDemo);
	}

	LearnUi();

#ifdef WITH_FX
	if (showFxTest) FxTestBed();
#endif // WITH_FX

	// popups
	static int currentPopup;
	if (showPopup & Popup_Any)
	{
		ImGui::OpenPopup("popup_Add");
		currentPopup = showPopup;
		showPopup &= ~Popup_Any;
	}
	if (ImGui::BeginPopup("popup_Add"))
	{
		if (currentPopup & Popup_AddGeometry)
		{
			ImGui::Text("Add geometry");
			ImGui::Separator();
			for (int type = GeometryType_None + 1; type < GeometryType_Count; ++type)
			{
				if (ImGui::MenuItem(GeometryName(type).c_str()))
				{
					ui::Log("Add geometry: {} {}", type, GeometryName(type));
					auto geometry = CreateAnyGeometry(type);
					AddGeometry(std::move(geometry));
				}
			}
		}
		else if (currentPopup & Popup_AddMap)
		{
			ImGui::Text("Add map tile");
			ImGui::Separator();
		}
		else if (currentPopup & Popup_AddMesh)
		{
			ImGui::Text("Add mesh");
			ImGui::Separator();
		}

		if (hidePopup & Popup_Any)
		{
			ImGui::CloseCurrentPopup();
			hidePopup &= ~Popup_Any;
		}
		ImGui::EndPopup();
	}

	return drawnFlag;
}

void App::OpenFile(int action)
{
	// clang-format off
	static const UMAP_INT_STR titles = {
		{ OpenAction_OpenScene, "Open Scene" },
		{ OpenAction_SaveScene, "Save Scene" },
	};
	// clang-format on

	fileAction = action;

	IGFD::FileDialogConfig config = {
		.path  = FindDefault(actionFolders, action, "."),
		.flags = ImGuiFileDialogFlags_Modal,
	};
	ImGuiFileDialog::Instance()->OpenDialog("OpenFileX", FindDefault(titles, action, "Choose File"), ".json", config);
}

void App::OpenedFile(int action, const std::filesystem::path& path)
{
	ui::Log("App/OpenedFile: {} action={} folder={}", path, action, fileFolder);
	if (path.empty()) return;

	switch (action)
	{
	case OpenAction_OpenScene: scene->OpenScene(path); break;
	case OpenAction_SaveScene: scene->SaveScene(path); break;
	default:
		ui::Log("OpenedFile: Unknown action: {} {}", action, path);
	}
}

void App::ShowMainMenu(float alpha)
{
	// if (!SetAlpha(alpha))
	//{
	//	menuHeight = 0.0f;
	//	return;
	// }

	if (wantVideo)
	{
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowBgAlpha(0.8f);

		if (ImGui::Begin("##VideoCaptureBar", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration))
		{
			if (ImGui::Button("REC")) wantVideo = false;
		}
		ImGui::End();
		return;
	}

	static int dirtyMenu = 0;
	int        update    = 0;

	if (ImGui::BeginMainMenuBar())
	{
		// file
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Scene...")) OpenFile(OpenAction_OpenScene);
			if (ImGui::BeginMenu("Open Recent"))
			{
				bool first = true;
				for (const auto& name : xsettings.recentFiles)
				{
					if (*name)
					{
						if (first)
						{
							if (ImGui::MenuItem("List Clear"))
							{
								memset(xsettings.recentFiles, 0, sizeof(xsettings.recentFiles));
								break;
							}
							ImGui::Separator();
							first = false;
						}
						std::filesystem::path path = name;
						if (ImGui::MenuItem(path.filename().string().c_str()))
							OpenedFile(OpenAction_OpenScene, path);
					}
				}

				if (first) ImGui::MenuItem("Empty List");
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Save Scene...")) OpenFile(OpenAction_SaveScene);
			ImGui::Separator();
			if (ImGui::MenuItem("Rescan assets")) RescanAssets();
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) quit = true;

			ImGui::EndMenu();
		}

		// physics
		if (ImGui::BeginMenu("Physics"))
		{
			ImGui::MenuItem("Paused", nullptr, &xsettings.physPaused);
			ImGui::MenuItem("Show Collision Volumes", nullptr, &xsettings.bulletDebug);
			ImGui::EndMenu();
		}

		// render
		if (ImGui::BeginMenu("Render"))
		{
			{
				bool selected = xsettings.projection == Projection_Orthographic;
				if (ImGui::MenuItem("Orthographic Projection", nullptr, selected))
					xsettings.projection = 1 - xsettings.projection;
			}
			ImGui::MenuItem("Mesh Instancing", nullptr, &xsettings.instancing);
			ImGui::Separator();
			if (ImGui::MenuItem("Capture Screenshot")) wantScreenshot = 6;
			if (ImGui::MenuItem("Capture Screenshot with UI")) wantScreenshot = 5;
			if (entryReset)
			{
				const bool canCapture = (*entryReset & BGFX_RESET_CAPTURE);

				ImGui::Separator();
				if (ImGui::MenuItem("Allow Video Capture", "", &xsettings.videoCapture))
				{
					if (xsettings.videoCapture && !canCapture)
					{
						ui::Log("need to RESET");
						// bgfx::reset(xsettings.windowSize[0], xsettings.windowSize[1], *entryReset);
					}
				}

				if (canCapture)
				{
					ImGui::MenuItem("Use Nvidia Encoding", "", &xsettings.nvidiaEnc);
					if (ImGui::MenuItem(wantVideo ? fmt::format("Stop Video Capture ({})", videoFrame).c_str() : "Start Video Capture", "", wantVideo))
					{
						wantVideo = !wantVideo;
						if (!wantVideo)
							*entryReset &= ~BGFX_RESET_CAPTURE;
						else if (!CreateDirectories("temp"))
							wantVideo = false;
					}
				}
			}
			ImGui::EndMenu();
		}

		// windows
		if (ImGui::BeginMenu("Windows"))
		{
			// clang-format off
			ui::AddMenu("Controls"    , nullptr, ui::GetControlsWindow());
			ui::AddMenu("Log"         , nullptr, ui::GetLogWindow());
			ui::AddMenu("Scene"       , nullptr, ui::GetSceneWindow());
			ui::AddMenu("Settings"    , nullptr, ui::GetSettingsWindow());
			ImGui::Separator();
			ImGui::MenuItem("ImGui Demo", nullptr, &showImGuiDemo);
			ui::AddMenu("Theme Editor", nullptr, ui::GetThemeWindow());
			ImGui::MenuItem("FX Test", nullptr, &showFxTest);
			// clang-format on

			ImGui::EndMenu();
		}

		// help
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("Good Luck!");
			ImGui::EndMenu();
		}
		// menuHeight = ImGui::GetWindowHeight();

		// save directly if update = 1, or after update-1 frames delay
		if (update) dirtyMenu = update;
		// if (dirtyMenu && !--dirtyMenu) SaveGameSettings();

		ImGui::EndMainMenuBar();
	}

	// drawGui();

	// ImGui::PopStyleColor();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI
/////

namespace ui
{

static float menuHeight = 0.0f;

float GetMenuHeight() { return menuHeight; }

} // namespace ui
