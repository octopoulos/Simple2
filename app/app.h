// app.h
// @author octopoulos
// @version 2025-07-16

#pragma once

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

class App : public entry::AppI
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ENTRY
	////////

public:
	App(const char* _name, const char* _description, const char* _url)
	    : entry::AppI(_name, _description, _url)
	{
	}

	void init(int32_t argc, const char* const* argv, uint32_t width, uint32_t height) override;
	int  shutdown() override;
	bool update() override;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INIT
	///////

public:
	~App();

	int  Initialize();
	void Destroy();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// MAP
	//////

private:
	sMesh                cursor    = nullptr; ///< current cursor for placing tiles
	MAP_STR<MAP_STR_INT> kitModels = {};      ///< model database: [title, filename]
	sObject3d            mapNode   = nullptr; ///< root of the map scene
	std::vector<Tile>    tiles     = {};      ///< tiles

	void AddObject(const std::string& name);
	void MapUi();
	bool OpenMap(const std::filesystem::path& filename);
	bool SaveMap(const std::filesystem::path& filename);
	void ScanModels(const std::filesystem::path& folder, const std::filesystem::path& folderPrev, int depth = 0, const std::string& relative = "");

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SCENE
	////////

private:
	std::unique_ptr<PhysicsWorld> physicsWorld   = nullptr; ///< physics world
	Scene                         scene          = {};      ///
	ShaderManager                 shaderManager  = {};      ///
	TextureManager                textureManager = {};      ///

	bgfx::IndexBufferHandle  ibh     = {}; ///
	bgfx::VertexBufferHandle vbh     = {}; ///
	bgfx::ProgramHandle      program = {}; ///< For cube and floor

	float   curTime   = 0.0f; ///< current time
	float   deltaTime = 0.0f; ///< last delta (current - last)
	float   lastTime  = 0.0f; ///< last rendered time
	int64_t lastUs    = 0;    ///< microseconds
	int64_t startTime = 0;    ///< initial time

	int InitScene();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// UI
	/////

private:
	UMAP_INT_STR actionFolders = {};    ///< open image & save screenshot in different folders
	int          fileAction    = 0;     ///< action to take in OpenedFile
	std::string  fileFolder    = {};    ///< folder after OpenFile
	int64_t      keys[512]     = {};    ///< pushed keys
	int          lastCode      = 0;     ///< last pushed key
	int64_t      now           = 0;     ///< current timestamp in us
	bool         showImGuiDemo = false; ///< show ImGui demo window

	void EventKeyDown(int code);
	void EventKeyUp(int code);
	void FilesUi();
	void OpenFile(int action);
	void ShowMainMenu(float alpha);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// WINDOW
	/////////

private:
	bool              hasFocus    = true;  ///
	uint32_t          isDebug     = 0;     ///
	uint32_t          isReset     = 0;     ///
	int               mouseButton = 0;     ///< last known mouse button
	float             mousePos[3] = {};    ///< last known mouse position: x, y
	int               mouseScroll = 0;     ///< mouse wheel
	entry::MouseState mouseState  = {};    ///
	bool              quit        = false; ///< exit the mainloop
	uint32_t          screenX     = 1280;  ///
	uint32_t          screenY     = 800;   ///
	bool              useGlm      = false; ///

public:
	void MainLoop();
	void Render();
};
