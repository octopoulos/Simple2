// ui-menu.cpp
// @author octopoulos
// @version 2025-09-27

#include "stdafx.h"
#include "app/App.h"
#include "ui/ui.h"
//
#include "common/imgui/imgui.h"        // ImGui::
#include "entry/input.h"               // GetGlobalInput
#include "materials/MaterialManager.h" // GetMaterialManager
#include "scenes/Scene.h"              // Scene
#include "textures/TextureManager.h"   // GetTextureManager

#include "ImGuiFileDialog/ImGuiFileDialog.h"

// see app:ShowMoreFlags
enum ShowVarFlags : int
{
	Show_Camera      = 1 << 0,
	Show_Cursor      = 1 << 1,
	Show_Materials   = 1 << 2,
	Show_SelectedObj = 1 << 3,
	Show_Textures    = 1 << 4,
	Show_Vars        = 1 << 5,
};

static const std::string SIMAGE_EXTS = "Textures{.bmp,.dds,.exr,.gif,.hdr,.jpg,.ktx,.pic,.pgm,.png,.ppm,.psd,.tga}";

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
			openFolder                = instance->GetCurrentPath();
			actionFolders[openAction] = openFolder;
			OpenedFile(openAction, openParam, instance->GetFilePathName());
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

		FilesUi();

		ui::DrawWindows();
		VarsUi();
		if (showImGuiDemo) ImGui::ShowDemoWindow(&showImGuiDemo);
	}

	LearnUi();
	TestUi();
	PopupsUi();

	return drawnFlag;
}

void App::OpenedFile(int action, int param, const std::filesystem::path& path)
{
	ui::Log("App/OpenedFile: {} action={} param={} folder={}", path, action, param, openFolder);
	if (path.empty()) return;

	switch (action)
	{
	case OpenAction_Image:
		if (auto target = selectWeak.lock())
		{
			if (target->type & ObjectType_Mesh)
			{
				ui::Log("current_path={}", std::filesystem::current_path());
				ui::Log("relative={}", std::filesystem::relative(path, "mnt/d"));
				ui::Log("proximate={}", std::filesystem::proximate(path, "mnt/d"));
				Mesh::SharedPtr(target)->material->LoadTexture(param, path.string());
			}
		}
		break;
	case OpenAction_OpenScene: OpenScene(path); break;
	case OpenAction_SaveScene: SaveScene(path); break;
	default:
		ui::Log("OpenedFile: Unknown action: {} {} {}", action, param, path);
	}

	FocusScreen();
}

void App::OpenFile(int action, int param)
{
	// clang-format off
	static const UMAP<int, std::pair<std::string, std::string>> titleExts = {
		{ OpenAction_Image     , { "Choose an image"         , SIMAGE_EXTS      } },
		{ OpenAction_OpenScene , { "Open Scene"              , ".json"          } },
		{ OpenAction_SaveScene , { "Save Scene"              , ".json"          } },
		{ OpenAction_ShaderFrag, { "Choose a fragment shader", "((fs_.*\\.sc))" } },
		{ OpenAction_ShaderVert, { "Choose a vertex shader"  , "((vs_.*\\.sc))" } },
	};
	// clang-format on

	openAction = action;
	openParam  = param;

	IGFD::FileDialogConfig config;
	config.path  = FindDefault(actionFolders, action, "data");
	config.flags = ImGuiFileDialogFlags_Modal;

	switch (action)
	{
	case OpenAction_Image:
		config.path = FindDefault(actionFolders, action, "runtime/textures");
		break;
	case OpenAction_SaveScene:
		config.filePathName = xsettings.recentFiles[0];
		config.flags |= ImGuiFileDialogFlags_ConfirmOverwrite;
		break;
	case OpenAction_ShaderFrag:
	case OpenAction_ShaderVert:
		config.path = FindDefault(actionFolders, action, "shaders");
		break;
	}

	if (const auto& it = titleExts.find(action); it != titleExts.end())
	{
		const auto& [title, ext] = it->second;
		ImGuiFileDialog::Instance()->OpenDialog("OpenFileX", title, Cstr(ext), config);
	}
}

void App::PopupsUi()
{
	static int customBg = 0;

	// 1) new popup activation?
	if (showPopup & Popup_Any)
	{
		ImGui::OpenPopup("popup_Any");
		currentPopup = showPopup;
		customBg     = 0;
		showPopup &= ~Popup_Any;

		bool isAbs = false;
		int  offX  = 0.0f;
		int  offY  = 0.0f;
		int  sizeX = 0.0f;
		if (currentPopup & Popup_Delete)
		{
			offX = 322.0f;
			offY = 118.0f;
		}
		else if (currentPopup & Popup_Transform)
		{
			if (const auto& window = ui::GetSceneWindow(); window.isOpen)
			{
				isAbs = true;
				sizeX = 420.0f;
				offX  = window.pos.x - sizeX - 4.0f;
				offY  = window.pos.y;
			}
			customBg = 1;
		}

		if (isAbs)
		{
			if (offX || offY) ImGui::SetNextWindowPos(ImVec2(offX, offY));
		}
		else
		{
			const ImVec2 mousePos = ImGui::GetMousePos();
			const ImVec2 popupPos = ImVec2(bx::max(mousePos.x - offX, 0.0f), bx::max(mousePos.y - offY, 0.0f));
			ImGui::SetNextWindowPos(popupPos);
		}

		if (sizeX > 0.0f) ImGui::SetNextWindowSize(ImVec2(sizeX, 0.0f));
	}

	// 2) show popup
	// blender style popup background
	if (customBg) ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.240f, 0.240f, 0.240f, 1.00f));
	const int pushedBg = customBg;

	if (ImGui::BeginPopup("popup_Any"))
	{
		if (currentPopup)
		{
			if (currentPopup & Popup_AddGeometry)
			{
				ImGui::TextUnformatted("Add geometry");
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
				ImGui::TextUnformatted("Add map tile");
				ImGui::Separator();
			}
			else if (currentPopup & Popup_AddMesh)
			{
				ImGui::TextUnformatted("Add mesh");
				ImGui::Separator();
				if (ImGui::MenuItem("Rubik Cube")) AddObject(":RubikCube");
			}
			else if (currentPopup & Popup_Delete)
			{
				if (auto target = selectWeak.lock())
				{
					ImGui::TextUnformatted("Delete selected object?");
					ImGui::Text("(%s)", target->name.c_str());
					ImGui::Dummy(ImVec2(0.0f, 8.0f));
					if (ImGui::Button("Cancel", ImVec2(160.0f, 0.0f))) hidePopup |= Popup_Delete;
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button       , ImVec4(0.278f, 0.447f, 0.702f, 1.00f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.384f, 0.545f, 0.792f, 1.00f));
					if (ImGui::Button("Delete", ImVec2(160.0f, 0.0f))) DeleteSelected();
					ImGui::PopStyleColor(2);
				}
				else hidePopup |= Popup_Delete;
			}
			else if (currentPopup & Popup_Transform)
			{
				ImGui::TextUnformatted("Transform");
				ShowObjectSettings(true, ShowObject_Basic | ShowObject_Transform);
			}
		}

		// close popup
		if (hidePopup & Popup_Any)
		{
			ImGui::CloseCurrentPopup();
			hidePopup &= ~Popup_Any;
			currentPopup = 0;
			customBg     = 0;
		}
		ImGui::EndPopup();
	}
	else currentPopup = 0;

	if (pushedBg) ImGui::PopStyleColor();
}

void App::ShowMainMenu(float alpha)
{
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
			if (ImGui::MenuItem("New")) Scene::SharedPtr(scene)->Clear();
			if (ImGui::MenuItem("Open...")) OpenFile(OpenAction_OpenScene);
			ImGui::Separator();
			if (ImGui::BeginMenu("Open Recent"))
			{
				int numRecent = 0;
				for (const auto& name : xsettings.recentFiles)
				{
					if (*name)
					{
						++numRecent;
						std::filesystem::path path = name;
						if (ImGui::MenuItem(Cstr(path.filename())))
							OpenedFile(OpenAction_OpenScene, 0, path);
					}
				}
				if (!numRecent)
				{
					ImGui::MenuItem("Empty List");
					ImGui::Separator();
				}
				else
				{
					ImGui::Separator();
					if (ImGui::MenuItem("Clear List")) memset(xsettings.recentFiles, 0, sizeof(xsettings.recentFiles));
				}
				ImGui::MenuItem("Auto Load", nullptr, &xsettings.autoLoad);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Save")) SaveScene();
			if (ImGui::MenuItem("Save As...")) OpenFile(OpenAction_SaveScene);
			ImGui::MenuItem("Auto Save", nullptr, &xsettings.autoSave);
			ImGui::Separator();
			if (ImGui::MenuItem("Rescan Assets")) RescanAssets();
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) entry::ExitApp();

			ImGui::EndMenu();
		}

		// debug
		if (ImGui::BeginMenu("Capture"))
		{
			if (ImGui::MenuItem("Capture Screenshot")) wantScreenshot = 6;
			if (ImGui::MenuItem("Capture Screenshot with UI")) wantScreenshot = 5;
			if (entryReset)
			{
				const bool canCapture = (*entryReset & BGFX_RESET_CAPTURE);

				ImGui::Separator();
				if (ImGui::MenuItem("Allow Video Capture", "", &xsettings.captureVideo))
				{
					if (xsettings.captureVideo && !canCapture)
					{
						ui::Log("need to RESET");
						// bgfx::reset(xsettings.windowSize[0], xsettings.windowSize[1], *entryReset);
					}
				}

				if (canCapture)
				{
					ImGui::MenuItem("Use Nvidia Encoding", "", &xsettings.nvidiaEnc);
					if (ImGui::MenuItem(wantVideo ? Format("Stop Video Capture (%d)", videoFrame) : "Start Video Capture", "", wantVideo))
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

		// debug
		if (ImGui::BeginMenu("Debug"))
		{
			ui::AddMenuFlag("No Render", xsettings.debug, BGFX_DEBUG_IFH);
			ui::AddMenuFlag("Profiler" , xsettings.debug, BGFX_DEBUG_PROFILER);
			ui::AddMenuFlag("Stats"    , xsettings.debug, BGFX_DEBUG_STATS);
			ui::AddMenuFlag("Text"     , xsettings.debug, BGFX_DEBUG_TEXT);
			ui::AddMenuFlag("Wireframe", xsettings.debug, BGFX_DEBUG_WIREFRAME);
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
			ImGui::Separator();
			ui::AddMenuFlag("Depth Clamp"        , xsettings.reset, BGFX_RESET_DEPTH_CLAMP);
			ui::AddMenuFlag("Flip After Render"  , xsettings.reset, BGFX_RESET_FLIP_AFTER_RENDER);
			ui::AddMenuFlag("Flush After Render" , xsettings.reset, BGFX_RESET_FLUSH_AFTER_RENDER);
			ui::AddMenuFlag("Full Screen"        , xsettings.reset, BGFX_RESET_FULLSCREEN);
			ui::AddMenuFlag("HDR10"              , xsettings.reset, BGFX_RESET_HDR10);
			ui::AddMenuFlag("High DPI"           , xsettings.reset, BGFX_RESET_HIDPI);
			ui::AddMenuFlag("Max Anisotropy"     , xsettings.reset, BGFX_RESET_MAXANISOTROPY);
			ui::AddMenuFlag("MSAA x2"            , xsettings.reset, BGFX_RESET_MSAA_X2);
			ui::AddMenuFlag("MSAA x4"            , xsettings.reset, BGFX_RESET_MSAA_X4);
			ui::AddMenuFlag("MSAA x8"            , xsettings.reset, BGFX_RESET_MSAA_X8);
			ui::AddMenuFlag("MSAA x16"           , xsettings.reset, BGFX_RESET_MSAA_X16);
			ui::AddMenuFlag("sRGB Back-buffer"   , xsettings.reset, BGFX_RESET_SRGB_BACKBUFFER);
			ui::AddMenuFlag("Suspend Rendering"  , xsettings.reset, BGFX_RESET_SUSPEND);
			ui::AddMenuFlag("Transp. Back-buffer", xsettings.reset, BGFX_RESET_TRANSPARENT_BACKBUFFER);
			ui::AddMenuFlag("V-Sync"             , xsettings.reset, BGFX_RESET_VSYNC);
			ImGui::EndMenu();
		}

		// windows
		if (ImGui::BeginMenu("Windows"))
		{
			// clang-format off
			ui::AddMenu("Controls", nullptr, ui::GetControlsWindow());
			ui::AddMenu("Log"     , nullptr, ui::GetLogWindow());
			ui::AddMenu("Map"     , nullptr, ui::GetMapWindow());
			ui::AddMenu("Object"  , nullptr, ui::GetObjectWindow());
			ui::AddMenu("Scene"   , nullptr, ui::GetSceneWindow());
			ui::AddMenu("Settings", nullptr, ui::GetSettingsWindow());
			ImGui::MenuItem("Vars", nullptr, &xsettings.showVars);
			ImGui::Separator();
			ImGui::MenuItem("ImGui Demo", nullptr, &showImGuiDemo);
			ui::AddMenu("Theme Editor", nullptr, ui::GetThemeWindow());
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
}

void App::ShowPopup(int flag)
{
	if (currentPopup & flag) hidePopup |= flag;
	showPopup ^= flag;
}

void App::ShowObjectSettings(bool isPopup, int show)
{
	if (auto target = (camera->follow & CameraFollow_Cursor) ? cursor : selectWeak.lock())
	{
		if (target->type & ObjectType_Mesh)
			Mesh::SharedPtr(target)->ShowSettings(isPopup, show);
		else
			target->ShowSettings(isPopup, show);
	}
}

void App::VarsUi()
{
	if (!xsettings.showVars) return;

	if (ImGui::Begin("Vars", &xsettings.showVars))
	{
		const int showTree = xsettings.varTree;
		int       tree     = showTree & ~(Show_Camera | Show_Cursor | Show_Materials | Show_SelectedObj | Show_Textures | Show_Vars);

		BEGIN_PADDING();

		// camera
		if (ImGui::CollapsingHeader("Camera", SHOW_TREE(Show_Camera)))
		{
			tree |= Show_Camera;
			camera->ShowInfoTable(false);
		}

		// cursor
		if (ImGui::CollapsingHeader("Cursor", SHOW_TREE(Show_Cursor)))
		{
			tree |= Show_Cursor;
			cursor->ShowInfoTable(false);
		}

		// materials
		if (ImGui::CollapsingHeader("Materials", SHOW_TREE(Show_Materials)))
		{
			tree |= Show_Materials;
			GetMaterialManager().ShowInfoTable(false);
		}

		// selected object
		if (ImGui::CollapsingHeader("Selected Object", SHOW_TREE(Show_SelectedObj)))
		{
			tree |= Show_SelectedObj;
			if (auto object = selectWeak.lock()) object->ShowInfoTable();
		}

		// textures
		if (ImGui::CollapsingHeader("Textures", SHOW_TREE(Show_Textures)))
		{
			tree |= Show_Textures;
			GetTextureManager().ShowInfoTable(false);
		}

		// vars
		if (ImGui::CollapsingHeader("Vars", SHOW_TREE(Show_Vars)))
		{
			tree |= Show_Vars;

			const ImGuiIO& io = ImGui::GetIO();

			// clang-format off
			ui::ShowTable({
				{ "BaseFolder"            , BaseFolder().string()                    },
				{ "current_path"          , std::filesystem::current_path().string() },
				{ "currentPopup"          , std::to_string(currentPopup)             },
				{ "hidePopup"             , std::to_string(hidePopup)                },
				{ "inputFrame"            , std::to_string(inputFrame)               },
				{ "inputLag"              , std::to_string(inputLag)                 },
				{ "io.WantCaptureMouse"   , BoolString(io.WantCaptureMouse)          },
				{ "io.WantCaptureKeyboard", BoolString(io.WantCaptureKeyboard)       },
				{ "io.WantTextInput"      , BoolString(io.WantTextInput)             },
				{ "IsAnyItemFocused"      , BoolString(ImGui::IsAnyItemFocused())    },
				{ "isDebug"               , std::to_string(isDebug)                  },
				{ "kitModels.size"        , std::to_string(kitModels.size())         },
				{ "mapNode.childId"       , std::to_string(mapNode->childId)         },
				{ "MouseOverArea"         , BoolString(ImGui::MouseOverArea())       },
				{ "openAction"            , std::to_string(openAction)               },
				{ "openFolder"            , openFolder                               },
				{ "openParam"             , std::to_string(openParam)                },
				{ "pauseNextFrame"        , BoolString(pauseNextFrame)               },
				{ "physicsFrame"          , std::to_string(physicsFrame)             },
				{ "renderFrame"           , std::to_string(renderFrame)              },
				{ "screenX"               , std::to_string(screenX)                  },
				{ "screenY"               , std::to_string(screenY)                  },
				{ "showLearn"             , BoolString(showLearn)                    },
				{ "showPopup"             , std::to_string(showPopup)                },
				{ "showTest"              , BoolString(showTest)                     },
				{ "videoFrame"            , std::to_string(videoFrame)               },
				{ "x.debug"               , std::to_string(xsettings.debug)          },
				{ "x.objectTree"          , std::to_string(xsettings.objectTree)     },
				{ "x.physPaused"          , std::to_string(xsettings.physPaused)     },
				{ "x.reset"               , std::to_string(xsettings.reset)          },
				{ "x.settingTree"         , std::to_string(xsettings.settingTree)    },
				{ "x.varTree"             , std::to_string(xsettings.varTree)        },
			});
			// clang-format on
		}

		xsettings.varTree = tree;

		END_PADDING();
	}
	ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI
/////

namespace ui
{

static float menuHeight = 0.0f;

float GetMenuHeight() { return menuHeight; }

} // namespace ui
