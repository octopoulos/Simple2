// menu.cpp
// @author octopoulos
// @version 2025-07-23

#include "stdafx.h"
#include "app.h"
#include "ui/ui.h"
#include "ui/engine-settings.h"

#include "dear-imgui/imgui.h"

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
			//OpenedFile(fileAction, instance->GetFilePathName());
		}
		instance->Close();
	}
#endif
}

void App::MainUi()
{
	MapUi();
	ShowMainMenu(1.0f);
	FilesUi();
	//ui::DrawWindows();
	if (showImGuiDemo) ImGui::ShowDemoWindow(&showImGuiDemo);
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
				// for (const auto& name : xsettings.recentFiles)
				//{
				//	if (*name)
				//	{
				//		if (first)
				//		{
				//			if (ImGui::MenuItem("List Clear"))
				//			{
				//				memset(xsettings.recentFiles, 0, sizeof(xsettings.recentFiles));
				//				break;
				//			}
				//			ImGui::Separator();
				//			first = false;
				//		}
				//		std::filesystem::path path = name;
				//		if (ImGui::MenuItem(path.filename().string().c_str())) app->OpenedFile(1, path);
				//	}
				// }

				if (first) ImGui::MenuItem("Empty List");
				ImGui::EndMenu();
			}
			//ImGui::Separator();
			// if (ImGui::MenuItem(fmt::format("Save {} Defaults", GameName()).c_str())) SaveGameSettings("", true, "-def");
			// if (ImGui::MenuItem("Save Screenshot...", xsettings.shortcutScreenshot)) app->OpenFile(OpenAction_Screenshot);
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) quit = true;
			ImGui::EndMenu();
		}

		// windows
		if (ImGui::BeginMenu("Render"))
		{
			{
				bool selected = appSettings->projection == Projection_Orthographic;
				if (ImGui::MenuItem("Orthographic projection", nullptr, selected))
					appSettings->projection = 1 - appSettings->projection;
			}
			{
				bool selected = renderFlags & RenderFlag_Instancing;
				if (ImGui::MenuItem("Use instancing", nullptr, selected))
					renderFlags ^= RenderFlag_Instancing;
			}
			ImGui::EndMenu();
		}

		// windows
		if (ImGui::BeginMenu("Windows"))
		{
			// AddMenu("Controls", xsettings.shortcutControls, GetControlsWindow());
			// AddMenu("Log", xsettings.shortcutLog, GetLogWindow());
			// app->MenuWindows();
			// AddMenu("Settings", xsettings.shortcutSettings, GetSettingsWindow());
			//ImGui::Separator();
			ImGui::MenuItem("ImGui Demo", nullptr, &showImGuiDemo);
			ui::AddMenu("Theme Editor", nullptr, ui::GetThemeWindow());

			ImGui::EndMenu();
		}

		// help
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("Good luck!");
			ImGui::EndMenu();
		}

		// menuHeight = ImGui::GetWindowHeight();
		ImGui::EndMainMenuBar();

		// save directly if update = 1, or after update-1 frames delay
		if (update) dirtyMenu = update;
		// if (dirtyMenu && !--dirtyMenu) SaveGameSettings();
	}

	// drawGui();

	// ImGui::PopStyleColor();
}
