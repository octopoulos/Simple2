// app.h
// @author octopoulos
// @version 2025-07-21

#pragma once

#include "engine/Mesh.h"
#include "engine/Scene.h"
#include "engine/ShaderManager.h"
#include "engine/TextureManager.h"
#include "physics/PhysicsWorld.h"

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
	// SCENE
	////////

private:
	std::unique_ptr<PhysicsWorld> physics        = nullptr; ///< physics world
	std::unique_ptr<Scene>        scene          = nullptr; ///
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
	int InitScene();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// UI
	/////

private:
	UMAP_INT_STR actionFolders = {};    ///< open image & save screenshot in different folders
	int          fileAction    = 0;     ///< action to take in OpenedFile
	std::string  fileFolder    = {};    ///< folder after OpenFile
	int64_t      now           = 0;     ///< current timestamp in us
	bool         showImGuiDemo = false; ///< show ImGui demo window

	/// Check input combos and perform actions
	void Controls();

	/// Handle file dialogs
	void FilesUi();

	/// Open an ImGuiFileDialog
	void OpenFile(int action);

	/// Show the menu bar
	void ShowMainMenu(float alpha);

public:
	/// Show all custom UI elements
	void MainUi();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// WINDOW
	/////////

private:
	bool              hasFocus      = true;  ///
	uint32_t          isDebug       = 0;     ///
	uint32_t          isReset       = 0;     ///
	int               mouseScroll   = 0;     ///< mouse wheel
	entry::MouseState mouseState    = {};    ///
	bool              isPerspective = true;  ///< perspective or orthogonal
	bool              quit          = false; ///< exit the mainloop
	uint32_t          screenX       = 1328;  ///
	uint32_t          screenY       = 800;   ///

public:
	/// Render everything except UI
	void Render();

	/// Synchronization with entry
	void SynchronizeEvents(uint32_t _screenX, uint32_t _screenY, entry::MouseState& _mouseState);
};
