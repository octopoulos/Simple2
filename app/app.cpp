// app.cpp
// @author octopoulos
// @version 2025-07-11
//
// export DYLD_LIBRARY_PATH=/opt/homebrew/lib

#include "stdafx.h"
#include "app.h"
#include "common/bgfx_utils.h"
#include "common/camera.h"
#include "imgui/imgui.h"

#include "engine/ModelLoader.h"

#ifdef _WIN32
#	include <windows.h>
#endif
#if BX_PLATFORM_OSX
extern "C" void* GetMacOSMetalLayer(SDL_Window* window);
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ENTRY
////////

void App::init(int32_t argc, const char* const* argv, uint32_t width, uint32_t height)
{
	Args args(argc, argv);

	screenX = width;
	screenY = height;
	isDebug = BGFX_DEBUG_NONE;
	isReset = BGFX_RESET_VSYNC;

	bgfx::Init init;
	init.type              = args.m_type;
	init.vendorId          = args.m_pciId;
	init.platformData.nwh  = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
	init.platformData.ndt  = entry::getNativeDisplayHandle();
	init.platformData.type = entry::getNativeWindowHandleType();
	init.resolution.width  = screenX;
	init.resolution.height = screenY;
	init.resolution.reset  = isReset;
	bgfx::init(init);

	bgfx::setDebug(isDebug);
	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

	Initialize();
}

int App::shutdown()
{
	Destroy();
	return 0;
}

bool App::update()
{
	if (entry::processEvents(screenX, screenY, isDebug, isReset, &mouseState)) return false;

	imguiBeginFrame(mouseState.m_mx, mouseState.m_my, (mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0) | (mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0) | (mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0), mouseState.m_mz, uint16_t(screenX), uint16_t(screenY));
	showExampleDialog(this);
	imguiEndFrame();

	curTime   = TO_FLOAT((bx::getHPCounter() - startTime) / TO_DOUBLE(bx::getHPFrequency()));
	deltaTime = curTime - lastTime;

	cameraUpdate(deltaTime, mouseState, ImGui::MouseOverArea());

	Render();

	// Advance to next frame. Rendering thread will be kicked to process submitted rendering primitives
	bgfx::frame();
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INIT
///////

App::~App()
{
	Destroy();
}

void App::Destroy()
{
	imguiDestroy();

	shaderManager.Destroy();
	bgfx::destroy(program);
	bgfx::destroy(ibh);
	bgfx::destroy(vbh);
	bgfx::shutdown();
	SDL_DestroyWindow(window);
	SDL_Quit();
}

int App::Initialize()
{
	FindAppDirectory(true);

	if (const int result = InitScene(); result < 0) return result;

	imguiCreate();

	cameraCreate();
	cameraSetPosition({ 0.0f, 0.0, -5.0f });
	// cameraSetVerticalAngle(-bx::kPi / 1.0f);
	// cameraSetHorizontalAngle(bx::kPi / 1.0f);
	return 1;
}

int App::InitScene()
{
	// Define cube vertex layout
	bgfx::VertexLayout cubeLayout;
	cubeLayout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();

	struct PosColorVertex
	{
		float    x, y, z;
		uint32_t abgr;
	};

	static PosColorVertex cubeVertices[] = {
		{ -1, 1,  1,  0xff000000 },
		{ 1,  1,  1,  0xff0000ff },
		{ -1, -1, 1,  0xff00ff00 },
		{ 1,  -1, 1,  0xff00ffff },
		{ -1, 1,  -1, 0xffff0000 },
		{ 1,  1,  -1, 0xffff00ff },
		{ -1, -1, -1, 0xffffff00 },
		{ 1,  -1, -1, 0xffffffff },
	};

	static const uint16_t cubeTriList[] = {
		0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7,
		0, 2, 4, 4, 2, 6, 1, 5, 3, 5, 7, 3,
		0, 4, 1, 4, 5, 1, 2, 3, 6, 6, 3, 7
	};

	vbh     = bgfx::createVertexBuffer(bgfx::makeRef(cubeVertices, sizeof(cubeVertices)), cubeLayout);
	ibh     = bgfx::createIndexBuffer(bgfx::makeRef(cubeTriList, sizeof(cubeTriList)));
	program = shaderManager.LoadProgram("vs_cube", "fs_cube");

	if (!bgfx::isValid(program)) THROW_RUNTIME("Shader program creation failed");
	ui::Log("Shaders loaded successfully");

	// load BIN model
	ModelLoader loader;
	if (auto object = loader.LoadModel("kenney_car-kit-obj/race-future"))
	{
		object->program = shaderManager.LoadProgram("vs_model", "fs_model");
		bx::mtxRotateXYZ(object->transform, -0.3f, 2.5f, 0.0f);
		scene.AddNamedChild("car", object);
	}
	if (auto object = loader.LoadModel("building-n"))
	{
		object->program = shaderManager.LoadProgram("vs_model", "fs_model");
		bx::mtxSRT(
		    object->transform,
		    1.0f, 1.0f, 1.0f, // scale
		    0.0f, 1.5f, 0.0f, // rotation
		    3.0f, -1.0f, 0.0f // translation
		);
		scene.AddNamedChild("building", object);
	}
	if (auto object = loader.LoadModel("bunny"))
	{
		object->program = shaderManager.LoadProgram("vs_mesh", "fs_mesh");
		bx::mtxSRT(
		    object->transform,
		    1.0f, 1.0f, 1.0f, // scale
		    0.0f, 1.5f, 0.0f, // rotation
		    -3.0f, 0.0f, 0.0f // translation
		);
		scene.AddNamedChild("bunny", object);
	}
	if (auto object = loader.LoadModel("Donut"))
	{
		object->program = shaderManager.LoadProgram("vs_model", "fs_model");
		bx::mtxSRT(
		    object->transform,
		    1.0f, 1.0f, 1.0f, // scale
		    0.0f, 3.0f, 0.0f, // rotation
		    0.0f, 1.0f, -2.0f // translation
		);
		scene.AddNamedChild("donut", object);
	}
	for (int i = 0; i < 15; ++i)
	{
		if (auto object = loader.LoadModel("Donut"))
		{
			object->program = shaderManager.LoadProgram("vs_model", "fs_model");
			bx::mtxSRT(
			    object->transform,
			    1.0f, 1.0f, 1.0f,                             // scale
			    sinf(i * 0.3f), 3.0f, 0.0f,                   // rotation
			    i * 0.4f - 2.4f, sinf(i * 0.5f) + 0.1f, -2.5f // translation
			);
			scene.AddNamedChild(fmt::format("donut-{}", i), object);
		}
	}

	// print scene children
	{
		ui::Log("Scene.children={}", scene.children.size());
		for (int i = -1; const auto& child : scene.children)
			ui::Log(" {:2}: {}", ++i, child->name);
	}

	physicsWorld = std::make_unique<PhysicsWorld>();
	physicsWorld->ResetCube();

	startTime = bx::getHPCounter();
	lastUs    = NowUs();
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CONTROLS
///////////

void App::EventKeyDown(int code)
{
	const auto repeat = keys[code];

	ui::Log("SDL_EVENT_KEY_DOWN: {} {}", code, repeat);

	// NO REPEAT
	////////////

	if (!repeat)
	{
		switch (code)
		{
		case 10: useGlm = !useGlm; break;
		}
	}

	// REPEAT ALLOWED
	/////////////////

	keys[code] = NowMs();
	lastCode   = code;
}

void App::EventKeyUp(int code)
{
	keys[code] = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WINDOW
/////////

void App::MainLoop()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_KEY_DOWN:
			EventKeyDown(event.key.scancode);
			break;
		case SDL_EVENT_KEY_UP:
			EventKeyUp(event.key.scancode);
			break;
		case SDL_EVENT_MOUSE_WHEEL:
			mouseScroll += event.wheel.y;
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			mouseButton = event.button.button;
			mousePos[0] = event.motion.x;
			mousePos[1] = event.motion.y;
			ui::Log("Mouse button down, resetting cube");
			physicsWorld->ResetCube();
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			mouseButton = 0;
			break;
		case SDL_EVENT_MOUSE_MOTION:
			mousePos[0] = event.motion.x;
			mousePos[1] = event.motion.y;
			break;
		case SDL_EVENT_QUIT:
			ui::Log("Quit event received");
			quit = true;
			break;
		case SDL_EVENT_TEXT_INPUT:
			break;
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			hasFocus = true;
			break;
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			hasFocus = false;
			break;
		case SDL_EVENT_WINDOW_RESIZED:
			screenX = event.window.data1;
			screenY = event.window.data2;
			// ResizeWindow(event.window.data1, event.window.data2);
			break;
		}
	}
	Render();
}

void App::Render()
{
	// camera view
	{
		bgfx::setViewRect(0, 0, 0, screenX, screenY);

		float view[16];
		cameraGetViewMtx(view);

		float proj[16];
		bx::mtxProj(proj, 60.0f, float(screenX) / float(screenY), 0.1f, 2000.0f, bgfx::getCaps()->homogeneousDepth);

		bgfx::setViewTransform(0, view, proj);
	}

	physicsWorld->dynamicsWorld->stepSimulation(deltaTime, 10, 1.0f / 60.0f);
	lastTime = curTime;

	// Render cube
	{
		btTransform t;
		physicsWorld->cubeRigidBody->getMotionState()->getWorldTransform(t);
		float mat[16];
		t.getBasis().getOpenGLSubMatrix(mat);
		mat[12] = t.getOrigin().x();
		mat[13] = t.getOrigin().y();
		mat[14] = t.getOrigin().z();
		mat[15] = 1;
		bgfx::setTransform(mat);
		bgfx::setVertexBuffer(0, vbh);
		bgfx::setIndexBuffer(ibh);
		bgfx::submit(0, program);
	}

	// Floor
	{
		btMatrix3x3 scale(10, 0, 0, 0, 0.5, 0, 0, 0, 10);
		btVector3   trans(0, -2, 0);
		float       mat[16];
		scale.getOpenGLSubMatrix(mat);
		mat[12] = trans.x();
		mat[13] = trans.y();
		mat[14] = trans.z();
		mat[15] = 1;
		bgfx::setTransform(mat);
		bgfx::setVertexBuffer(0, vbh); // Reuse cube vertex buffer for floor
		bgfx::setIndexBuffer(ibh);
		bgfx::submit(0, program);
	}

	// draw the scene
	scene.RenderScene(0, screenX, screenY);
}

ENTRY_IMPLEMENT_MAIN(App, "App", "Simple app", "https://shark-it.be");
