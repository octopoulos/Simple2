// menu.cpp
// @author octopoulos
// @version 2025-07-30

#include "stdafx.h"
#include "app/App.h"
#include "ui/ui.h"

#include "common/imgui/imgui.h"
#include "entry/input.h"

#ifdef WITH_FILE_DIALOG
#	include "ImGuiFileDialog/ImGuiFileDialog.h"
#endif // WITH_FILE_DIALOG

/// https://github.com/octopoulos/ImGuiFileDialog/blob/master/Documentation.md
void App::FilesUi()
{
#ifdef WITH_FILE_DIALOG
	if (auto instance = ImGuiFileDialog::Instance(); instance->Display("OpenFileX", 0, { 768.0f, 512.0f }))
	{
		if (instance->IsOk())
		{
			fileFolder                = instance->GetCurrentPath();
			actionFolders[fileAction] = fileFolder;
			// OpenedFile(fileAction, instance->GetFilePathName());
		}
		instance->Close();
	}
#endif
}

int App::MainUi()
{
	// lock => no UI
	if (GetGlobalInput().mouseLock) return 0;

	// skip some UI elements when recording video
	int drawnFlag = 1;
	if (!wantVideo)
	{
		drawnFlag |= 2;

		MapUi();
		FilesUi();

		// ui::DrawWindows();
		if (showImGuiDemo) ImGui::ShowDemoWindow(&showImGuiDemo);
	}

	LearnUi();

	// popups
	if (showPopup & 1)
	{
		ImGui::OpenPopup("popup_Add");
		showPopup &= ~1;
	}
	if (ImGui::BeginPopup("popup_Add"))
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

		if (hidePopup & 1)
		{
			ImGui::CloseCurrentPopup();
			hidePopup &= ~1;
		}
		ImGui::EndPopup();
	}

	// show menu (or [REC] if recording video)
	ShowMainMenu(1.0f);

	return drawnFlag;
}

void App::OpenFile(int action)
{
	static const UMAP_INT_STR titles = {
		{ 1, "Open file"       },
		{ 2, "Save file"       },
		{ 3, "Save screenshot" },
	};

	fileAction = action;

#ifdef WITH_FILE_DIALOG
	IGFD::FileDialogConfig config = {
		.path  = FindDefault(actionFolders, action, "."),
		.flags = ImGuiFileDialogFlags_Modal,
	};
	ImGuiFileDialog::Instance()->OpenDialog("OpenFileX", FindDefault(titles, action, "Choose File"), "((.*))", config);
#endif // WITH_FILE_DIALOG
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
	// const bool hasFrame  = app->HasFrame();
	// const int  ioBusy    = app->IoBusy();
	int        update    = 0;

	if (ImGui::BeginMainMenuBar())
	{
		// file
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Image/Video...", "")) OpenFile(0);
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
							ui::Log("app->OpenedFile(1, path);");
					}
				}

				if (first) ImGui::MenuItem("Empty List");
				ImGui::EndMenu();
			}
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
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) quit = true;

			ImGui::EndMenu();
		}

		// physics
		if (ImGui::BeginMenu("Physics"))
		{
			ImGui::MenuItem("Paused", nullptr, &xsettings.physPaused);
			ImGui::MenuItem("Draw Collision Volumes", nullptr, &xsettings.bulletDebug);
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
			ImGui::EndMenu();
		}

		// windows
		if (ImGui::BeginMenu("Windows"))
		{
			// AddMenu("Controls", xsettings.shortcutControls, GetControlsWindow());
			// AddMenu("Log", xsettings.shortcutLog, GetLogWindow());
			// app->MenuWindows();
			// AddMenu("Settings", xsettings.shortcutSettings, GetSettingsWindow());
			// ImGui::Separator();
			ImGui::MenuItem("ImGui Demo", nullptr, &showImGuiDemo);
			ui::AddMenu("Theme Editor", nullptr, ui::GetThemeWindow());

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
