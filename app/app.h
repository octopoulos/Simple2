// app.h
// @author octopoulos
// @version 2025-07-25

#pragma once

#include "engine/Camera.h"
#include "engine/Mesh.h"
#include "engine/Scene.h"
#include "engine/ShaderManager.h"
#include "engine/TextureManager.h"
#include "physics/PhysicsWorld.h"
#include "ui/xsettings.h"

struct Tile
{
	sMesh       mesh = nullptr; ///< link to visual node
	std::string name = "";      ///< resource name
	float       x    = 0.0f;    ///< right
	float       y    = 0.0f;    ///< up
	float       z    = 0.0f;    ///< front
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

	/// Fixed-rate input logic (ex: key movement, snap-to-grid)
	void FixedControls();

	/// Per-frame input logic (ex: smooth camera movement)
	void FluidControls();

	/// Throw a donut
	void ThrowDonut();

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
	sMesh                cursor    = nullptr; ///< current cursor for placing tiles
	MAP_STR<MAP_STR_INT> kitModels = {};      ///< model database: [title, filename]
	int                  iconSize  = 64;      ///< icon size for the map tiles previews
	sObject3d            mapNode   = nullptr; ///< root of the map scene
	std::vector<Tile>    tiles     = {};      ///< tiles

	/// Insert an object into the map
	void AddObject(const std::string& name);

	/// UI for the map maker
	void MapUi();

	/// Open a map file
	bool OpenMap(const std::filesystem::path& filename);

	/// Save a map file
	bool SaveMap(const std::filesystem::path& filename);

	/// Scan models in the given folder and its subfolders
	void ScanModels(const std::filesystem::path& folder, const std::filesystem::path& folderPrev, int depth = 0, const std::string& relative = "");

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// RENDER
	/////////

private:
	bool                pauseNextFrame = false; ///< pause next frame
	int                 renderFlags    = 0;     ///< render flags
	int                 renderFrame    = 0;     ///< current rendered frame
	bgfx::UniformHandle uTime          = {};    ///

public:
	/// Render everything except UI
	void Render();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SCENE
	////////

private:
	sCamera                       camera         = nullptr; ///< camera
	std::unique_ptr<PhysicsWorld> physics        = nullptr; ///< physics world
	int                           physicsFrame   = 0;       ///< current physics frame
	std::unique_ptr<Scene>        scene          = nullptr; ///< scene container
	ShaderManager                 shaderManager  = {};      ///
	TextureManager                textureManager = {};      ///

	bgfx::IndexBufferHandle  ibh = {}; /// TODO: delete
	bgfx::VertexBufferHandle vbh = {}; /// TODO: delete

	float   curTime   = 0.0f; ///< current time
	float   deltaTime = 0.0f; ///< last delta (current - last)
	float   lastTime  = 0.0f; ///< last rendered time
	int64_t lastUs    = 0;    ///< microseconds
	int64_t startTime = 0;    ///< initial time

	/// Create scene and physics
	int InitializeScene();

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
	bool         showImGuiDemo = false; ///< show ImGui demo window
	int          videoFrame    = 0;     ///< how many video frames have been captured so far

	/// Handle file dialogs
	void FilesUi();

	/// Open an ImGuiFileDialog
	void OpenFile(int action);

	/// Show the menu bar
	void ShowMainMenu(float alpha);

public:
	int  wantScreenshot = 0;     ///< capture a screenshot this frame? &1: with UI, &2: without UI, &4: capture next frame
	bool wantVideo      = false; ///< want video capture

	/// Show all custom UI elements
	void MainUi();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// WINDOW
	/////////

private:
	bool     hasFocus = true;  ///
	uint32_t isDebug  = 0;     ///
	bool     quit     = false; ///< exit the mainloop
	uint32_t screenX  = 1328;  ///
	uint32_t screenY  = 800;   ///

public:
	uint32_t* entryReset = nullptr; ///< pointer to entry::m_reset

	/// Synchronization with entry
	void SynchronizeEvents(uint32_t _screenX, uint32_t _screenY);
};
