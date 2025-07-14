// app.h
// @author octopoulos
// @version 2025-07-05

#pragma once

#include "engine/ShaderManager.h"
#include "physics.h"

class App
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INIT
	///////

private:
	int InitBackend();

public:
	App();
	~App();

	int  Init();
	void Destroy();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SCENE
	////////

private:
	Scene         scene         = {}; //
	ShaderManager shaderManager = {}; //

	bgfx::IndexBufferHandle  ibh     = {}; //
	bgfx::VertexBufferHandle vbh     = {}; //
	bgfx::ProgramHandle      program = {}; // For cube and floor

	float   lastTime  = 0.0f; // last rendered time
	int64_t lastUs    = 0;    // microseconds
	int64_t startTime = 0;    // initial time

	std::unique_ptr<PhysicsWorld> physicsWorld = nullptr; //

	int InitScene();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// UI
	/////

private:
	int64_t keys[SDL_SCANCODE_COUNT] = {}; // pushed keys
	int     lastCode                 = 0;  // last pushed key
	int64_t now                      = 0;  // current timestamp in us

	void EventKeyDown(int code);
	void EventKeyUp(int code);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// WINDOW
	/////////

private:
	bool        hasFocus     = true;    //
	int         mouseButton  = 0;       // last known mouse button
	float       mousePos[3]  = {};      // last known mouse position: x, y
	int         mouseScroll  = 0;       // mouse wheel
	bool        quit         = false;   // exit the mainloop
	int         screenHeight = 800;     //
	int         screenWidth  = 1280;    //
	bool        useGlm       = false;   //
	SDL_Window* window       = nullptr; //

	void MainLoop();
	void Render();

public:
	void Run();
};
