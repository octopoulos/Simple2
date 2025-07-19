// menu.cpp
// @author octopoulos
// @version 2025-07-15

#include "stdafx.h"
#include "app.h"

#include "dear-imgui/imgui.h"

//#define WITH_FILE_DIALOG 1

#ifdef WITH_FILE_DIALOG
#	include "ImGuiFileDialog/ImGuiFileDialog.h"
#endif // WITH_FILE_DIALOG

/**
 * Handle file dialogs
 * https://github.com/octopoulos/ImGuiFileDialog/blob/master/Documentation.md
 */
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

/**
 * Open an ImGuiFileDialog
 */
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

/**
 * Show the menu bar
 */
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
			ImGui::Separator();
			// if (ImGui::MenuItem(fmt::format("Save {} Defaults", GameName()).c_str())) SaveGameSettings("", true, "-def");
			// if (ImGui::MenuItem("Save Screenshot...", xsettings.shortcutScreenshot)) app->OpenFile(OpenAction_Screenshot);
			ImGui::Separator();
			// if (ImGui::MenuItem("Exit")) app->Quit();
			ImGui::EndMenu();
		}

		// capture
		if (ImGui::BeginMenu("Capture"))
		{
			// if (ImGui::MenuItem("Capture Device", xsettings.shortcutDevice)) app->CaptureVideo(CapType::Device);
			// if (ImGui::MenuItem("Capture Screen", xsettings.shortcutScreen)) app->SourceType(CapType::Screen);
			ImGui::Separator();
			// if (ImGui::MenuItem("Start", xsettings.shortcutStart, false, app->HasVideo())) app->Pause(0);
			// if (ImGui::MenuItem("Pause", xsettings.shortcutPause, false, app->HasVideo())) app->Pause(1);
			// if (ImGui::MenuItem("Stop", xsettings.shortcutStop, false, app->HasVideo())) app->SourceType(CapType::None);
			ImGui::Separator();
			// if (ImGui::MenuItem("OCR now")) app->OcrNow();
			ImGui::EndMenu();
		}

		// view
		if (ImGui::BeginMenu("View"))
		{
			// clang-format off
			//if (ImGui::MenuItem("View Source","F1", xsettings.target == TARGET_SOURCE)) xsettings.target = TARGET_SOURCE;
			//if (ImGui::MenuItem("View Paint" ,"F2", xsettings.target == TARGET_PAINT )) xsettings.target = TARGET_PAINT;
			//if (ImGui::MenuItem("View OCR",   "F3", xsettings.target == TARGET_OCR   )) xsettings.target = TARGET_OCR;
			//if (ImGui::MenuItem("View Debug", "F4", xsettings.target == TARGET_VIEW  )) xsettings.target = TARGET_VIEW;
			//if (ImGui::MenuItem("View Gray",  "F5", xsettings.target == TARGET_GRAY  )) xsettings.target = TARGET_GRAY;
			//if (ImGui::MenuItem("View Hue",   "F6", xsettings.target == TARGET_HUE   )) xsettings.target = TARGET_HUE;
			//if (ImGui::MenuItem("R Channel",  "F7", xsettings.target == TARGET_R     )) xsettings.target = TARGET_R;
			//if (ImGui::MenuItem("G Channel",  "F8", xsettings.target == TARGET_G     )) xsettings.target = TARGET_G;
			//if (ImGui::MenuItem("B Channel",  "F9", xsettings.target == TARGET_B     )) xsettings.target = TARGET_B;
			// clang-format on
			ImGui::EndMenu();
		}

		// windows
		if (ImGui::BeginMenu("Windows"))
		{
			// AddMenu("Controls", xsettings.shortcutControls, GetControlsWindow());
			// AddMenu("Log", xsettings.shortcutLog, GetLogWindow());
			// app->MenuWindows();
			// AddMenu("Settings", xsettings.shortcutSettings, GetSettingsWindow());
			ImGui::Separator();
			// ImGui::MenuItem("ImGui Demo", nullptr, &showImGuiDemo);
			// AddMenu("Theme Editor", nullptr, GetThemeWindow());

			ImGui::EndMenu();
		}

		// help
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("Lvi - Оксана (Gdansk): Привіт!!!");
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
