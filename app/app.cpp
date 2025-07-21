// app.cpp
// @author octopoulos
// @version 2025-07-16
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

	// 1) ImGui
	imguiBeginFrame(mouseState.m_mx, mouseState.m_my, (mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0) | (mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0) | (mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0), mouseState.m_mz, uint16_t(screenX), uint16_t(screenY));
	{
		showExampleDialog(this);
		MapUi();
		ShowMainMenu(1.0f);
		FilesUi();

		if (showImGuiDemo) ImGui::ShowDemoWindow(&showImGuiDemo);
	}
	imguiEndFrame();

	// 2) 3d render
	{
		curTime   = TO_FLOAT((bx::getHPCounter() - startTime) / TO_DOUBLE(bx::getHPFrequency()));
		deltaTime = curTime - lastTime;

		cameraUpdate(deltaTime, mouseState, ImGui::MouseOverArea());
		Render();
	}

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
}

int App::Initialize()
{
	FindAppDirectory(true);
	ScanModels("runtime/models", "runtime/models-prev");

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
	cursor  = std::make_shared<Mesh>();
	mapNode = std::make_shared<Object3d>();
	scene.AddNamedChild(cursor, "cursor");
	scene.AddNamedChild(mapNode, "map");

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
		bx::mtxRotateXYZ(glm::value_ptr(object->transform), -0.3f, 2.5f, 0.0f);
		scene.AddNamedChild(object, "car");
	}
	if (auto object = loader.LoadModel("building-n"))
	{
		object->program = shaderManager.LoadProgram("vs_model", "fs_model");
		bx::mtxSRT(
		    glm::value_ptr(object->transform),
		    1.0f, 1.0f, 1.0f, // scale
		    0.0f, 1.5f, 0.0f, // rotation
		    3.0f, -1.0f, 0.0f // translation
		);
		scene.AddNamedChild(object, "building");
	}
	if (auto object = loader.LoadModel("bunny"))
	{
		object->program = shaderManager.LoadProgram("vs_mesh", "fs_mesh");
		bx::mtxSRT(
		    glm::value_ptr(object->transform),
		    1.0f, 1.0f, 1.0f, // scale
		    0.0f, 1.5f, 0.0f, // rotation
		    -3.0f, 0.0f, 0.0f // translation
		);
		scene.AddNamedChild(object, "bunny");
	}
	if (auto object = loader.LoadModel("donut"))
	{
		object->program = shaderManager.LoadProgram("vs_model", "fs_model");
		bx::mtxSRT(
		    glm::value_ptr(object->transform),
		    1.5f, 1.5f, 1.5f, // scale
		    0.0f, 3.0f, 0.0f, // rotation
		    0.0f, 1.0f, -2.0f // translation
		);
		scene.AddNamedChild(object, "donut");
	}

	for (int i = 0; i < 15; ++i)
	{
		if (auto object = loader.LoadModel("donut3"))
		{
			object->program = shaderManager.LoadProgram("vs_model", "fs_model");
			bx::mtxSRT(
			    glm::value_ptr(object->transform),
			    0.5f, 0.5f, 0.5f,                             // scale
			    sinf(i * 0.3f), 3.0f, 0.0f,                   // rotation
			    i * 0.4f - 2.4f, sinf(i * 0.5f) + 0.1f, -2.5f + MerseneFloat() // translation
			);
			scene.AddNamedChild(object, fmt::format("donut3-{}", i));
		}
	}

	// print scene children
	{
		ui::Log("Scene.children={}", scene.children.size());
		for (int i = -1; const auto& child : scene.children)
			ui::Log(" {:2}: {}", ++i, child->name);
	}

	physicsWorld = std::make_unique<PhysicsWorld>();

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

	physicsWorld->StepSimulation(deltaTime);
	lastTime = curTime;

	// // Render cube
	// {
	// 	btTransform t;
	// 	physicsWorld->cubeRigidBody->getMotionState()->getWorldTransform(t);
	// 	float mat[16];
	// 	t.getBasis().getOpenGLSubMatrix(mat);
	// 	mat[12] = t.getOrigin().x();
	// 	mat[13] = t.getOrigin().y();
	// 	mat[14] = t.getOrigin().z();
	// 	mat[15] = 1;
	// 	bgfx::setTransform(mat);
	// 	bgfx::setVertexBuffer(0, vbh);
	// 	bgfx::setIndexBuffer(ibh);
	// 	bgfx::submit(0, program);
	// }

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
	for (auto& child : scene.children)
	{
		if (child->name == "donut")
		{
			const glm::vec3 position = { 0.0f, 1.0f, -2.0f };
			const glm::quat rotation = glm::quat(glm::vec3(lastTime, 6.0f, 0.0f));
			child->transform         = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
		}
		else if (child->name.starts_with("donut3"))
		{
			//glm::vec3 position        = glm::vec3(child->transform[3]);
			//glm::quat currentRotation = glm::quat_cast(child->transform);
			//glm::quat extraRotation   = glm::quat(glm::vec3(0.001f, child->id * 0.001f, 0.0f));
			//glm::quat newRotation     = currentRotation * extraRotation;
			//glm::mat4 rotationMat     = glm::mat4_cast(newRotation);

			 const glm::vec3 position    = glm::vec3(child->transform[3]);
			 const glm::quat rotation    = glm::quat(glm::vec3(-lastTime * 0.05f * child->id, 3.0f, 0.0f));
			 const glm::mat4 rotationMat = glm::mat4_cast(rotation);

			child->transform[0] = rotationMat[0];
			child->transform[1] = rotationMat[1];
			child->transform[2] = rotationMat[2];
			//child->transform[3] = glm::vec4(position, 1.0f);
		}
	}

	// draw the scene
	scene.RenderScene(0, screenX, screenY);
}

ENTRY_IMPLEMENT_MAIN(App, "App", "Simple app", "https://shark-it.be");
