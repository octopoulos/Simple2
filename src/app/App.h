// App.h
// @author octopoulos
// @version 2025-08-19

#pragma once

#include "core/Camera.h"
#include "objects/Mesh.h"
#include "scenes/Scene.h"
#include "physics/PhysicsWorld.h"
#include "ui/xsettings.h"

enum Popups_ : int
{
	Popup_None        = 0,
	Popup_Add         = 1 << 0,
	Popup_AddGeometry = 1 << 1,
	Popup_AddMap      = 1 << 2,
	Popup_AddMesh     = 1 << 3,
	Popup_Delete      = 1 << 4,
	Popup_Any         = Popup_Add | Popup_AddGeometry | Popup_AddMap | Popup_AddMesh | Popup_Delete,
};

class App
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INIT
	///////

public:
	App() = default;

	~App() { Destroy(); }

	int  Initialize();
	void Destroy();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONTROLS
	///////////

private:
	float inputDelta = 1.0f / 120.0f; ///< input fixed time step (in seconds)
	int   inputFrame = 0;             ///< current input frame
	float inputLag   = 0.0f;          ///< accumulated lag for fixed-step input

	/// Check if arrows are being pushed, &1: down, &2: left, &4: right, &8: up
	int ArrowsFlag();

	/// Fixed-rate input logic (ex: key movement, snap-to-grid)
	void FixedControls();

	/// Per-frame input logic (ex: smooth camera movement)
	void FluidControls();

	/// Move the cursor using the arrows
	void MoveCursor(bool force);

	/// Throw a geometry
	/// @param geometryType: GeometryType_None for random
	void ThrowGeometry(int action, int geometryType = GeometryType_None, std::string_view textureName = "");

	/// Throw a mesh instance
	void ThrowMesh(int action, std::string_view name, int shapeType, std::string_view textureName = "");

public:
	/// Controls function executed every frame, and run before Render
	void Controls();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LEARN
	////////

private:
	bool showLearn = false; ///< show the learning window

	void LearnUi();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// MAP
	//////

private:
	sObject3d mapNode = nullptr; ///< root of the map scene

	/// Insert a geometry into the map
	void AddGeometry(uGeometry geometry);

	/// Open a map file
	bool OpenMap(const std::filesystem::path& filename);

	/// Save a map file
	bool SaveMap(const std::filesystem::path& filename);

	/// Scan models in the given folder and its subfolders
	void ScanModels(const std::filesystem::path& folder, const std::filesystem::path& folderPrev, int depth = 0, const std::string& relative = "");

public:
	MAP_STR<MAP_STR_INT> kitModels = {}; ///< model database: [title, filename]

	/// Insert an object into the map
	void AddObject(std::string_view modelName);

	/// Rescan folders to update the assets
	void RescanAssets();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// RENDER
	/////////

private:
	bool                pauseNextFrame = false; ///< pause next frame
	int                 renderFlags    = 0;     ///< render flags
	int                 renderFrame    = 0;     ///< current rendered frame
	bgfx::UniformHandle uLight         = {};    ///
	bgfx::UniformHandle uTime          = {};    ///

public:
	/// Render everything except UI
	void Render();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SCENE
	////////

private:
	sCamera                       camera       = nullptr; ///< camera
	sMesh                         cursor       = nullptr; ///< current cursor for placing objects
	float                         curTime      = 0.0f;    ///< current time
	float                         deltaTime    = 0.0f;    ///< last delta (current - last)
	float                         lastTime     = 0.0f;    ///< last rendered time
	int64_t                       lastUs       = 0;       ///< microseconds
	std::unique_ptr<PhysicsWorld> physics      = nullptr; ///< physics world
	int                           physicsFrame = 0;       ///< current physics frame
	std::weak_ptr<Object3d>       prevSelected = {};      ///< previously selected object
	int64_t                       startTime    = 0;       ///< initial time

	/// Delete the selected object
	void DeleteSelected();

	/// Create scene and physics
	int InitializeScene();

	/// Open a scene file + replace the scene
	bool OpenScene(const std::filesystem::path& filename);

	/// Save the scene to a file
	bool SaveScene(const std::filesystem::path& filename);

public:
	sObject3d               scene       = nullptr; ///< scene container
	std::weak_ptr<Object3d> selectedObj = {};      ///< selected object for edit

	/// Select the object to follow
	void SelectObject(const sObject3d& obj);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SETTINGS
	///////////

protected:
	std::filesystem::path imguiPath = {}; ///< imgui.ini

	/// Load default ImGui settings
	/// - after ImGui has been created
	void InitializeImGui();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// UI
	/////

private:
	UMAP_INT_STR actionFolders = {};    ///< open image & save screenshot in different folders
	int          fileAction    = 0;     ///< action to take in OpenedFile
	std::string  fileFolder    = {};    ///< folder after OpenFile
	int          hidePopup     = 0;     ///< close specific ImGui popups
	bool         showFxTest    = false; ///< show FxTestBed
	bool         showImGuiDemo = false; ///< show ImGui demo window
	int          showPopup     = 0;     ///< show specific ImGui popups
	int          videoFrame    = 0;     ///< how many video frames have been captured so far

	/// Handle file dialogs
	void FilesUi();

	/// Open an ImGuiFileDialog
	void OpenFile(int action);

	/// Opened a file with the dialog
	void OpenedFile(int action, const std::filesystem::path& path);

	/// Show all the popups
	void PopupsUi();

	/// Show the menu bar
	void ShowMainMenu(float alpha);

	/// Show/hide popups
	void ShowPopup(int flag);

public:
	int  wantScreenshot = 0;     ///< capture a screenshot this frame? &1: with UI, &2: without UI, &4: capture next frame
	bool wantVideo      = false; ///< want video capture

	/// Show all custom UI elements
	/// @returns 0: no UI drawn, &1: something was drawn, &2: no video
	int MainUi();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// WINDOW
	/////////

private:
	uint32_t isDebug = 0;    ///
	uint32_t screenX = 1328; ///
	uint32_t screenY = 800;  ///

public:
	uint32_t* entryReset = nullptr; ///< pointer to entry::m_reset

	/// Synchronization with entry
	void SynchronizeEvents(uint32_t _screenX, uint32_t _screenY);
};
