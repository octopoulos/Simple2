// app.cpp
// @author octopoulos
// @version 2025-07-21
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
// INIT
///////

void App::Destroy()
{
	scene.reset();
	physics.reset();
}

int App::Initialize()
{
	FindAppDirectory(true);
	ScanModels("runtime/models", "runtime/models-prev");

	if (const int result = InitScene(); result < 0) return result;

	// camera
	{
		cameraCreate();
		cameraSetPosition({ -11.77f, 6.74f, -15.57f });
		cameraSetHorizontalAngle(0.7f);
		cameraSetVerticalAngle(-0.45f);
	}
	return 1;
}

int App::InitScene()
{
	scene = std::make_unique<Scene>();

	cursor  = std::make_shared<Mesh>();
	mapNode = std::make_shared<Object3d>();
	scene->AddNamedChild(cursor, "cursor");
	scene->AddNamedChild(mapNode, "map");

	physics = std::make_unique<PhysicsWorld>();

	// cube vertex layout
	{
		bgfx::VertexLayout cubeLayout;
		// clang-format off
		cubeLayout.begin()
		    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::Color0  , 4, bgfx::AttribType::Uint8, true)
		    .end();
		// clang-format on

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

		vbh = bgfx::createVertexBuffer(bgfx::makeRef(cubeVertices, sizeof(cubeVertices)), cubeLayout);
		ibh = bgfx::createIndexBuffer(bgfx::makeRef(cubeTriList, sizeof(cubeTriList)));

		// cube
		{
			auto cubeMesh      = std::make_shared<Mesh>();
			cubeMesh->geometry = std::make_shared<Geometry>(vbh, ibh);
			cubeMesh->material = std::make_shared<Material>(shaderManager.LoadProgram("vs_cube", "fs_cube"));

			cubeMesh->ScaleRotationPosition(
			    { 0.5f, 0.5f, 0.5f },
			    { MerseneFloat(0.0f, bx::kPi2), MerseneFloat(0.0f, bx::kPi2), MerseneFloat(0.0f, bx::kPi2) },
			    { 0.0f, 5.0f, 0.0f }
			);
			cubeMesh->CreateShapeBody(physics.get(), ShapeType_Box, 1.0f, { 0.5f, 0.5f, 0.5f, 0.0f });

			scene->AddNamedChild(std::move(cubeMesh), "cube");
		}

		// floor
		{
			auto cubeMesh      = std::make_shared<Mesh>();
			cubeMesh->geometry = std::make_shared<Geometry>(vbh, ibh);
			cubeMesh->material = std::make_shared<Material>(shaderManager.LoadProgram("vs_cube", "fs_cube"));

			cubeMesh->ScaleRotationPosition(
			    { 8.0f, 0.5f, 8.0f },
			    { 0.0f, 0.0f, 0.0f },
			    { 0.0f, -2.0f, 0.0f }
			);
			cubeMesh->CreateShapeBody(physics.get(), ShapeType_Box, 0.0f, { 8.0f, 0.5f, 8.0f, 0.0f });

			scene->AddNamedChild(std::move(cubeMesh), "floor");
		}

		// base
		{
			auto cubeMesh      = std::make_shared<Mesh>();
			cubeMesh->geometry = std::make_shared<Geometry>(vbh, ibh);
			cubeMesh->material = std::make_shared<Material>(shaderManager.LoadProgram("vs_base", "fs_base"));

			cubeMesh->ScaleRotationPosition(
			    { 40.0f, 1.0f, 40.0f },
			    { 0.0f, 0.0f, 0.0f },
			    { 0.0f, -10.0f, 0.0f });
			cubeMesh->CreateShapeBody(physics.get(), ShapeType_Box, 0.0f, { 40.0f, 1.0f, 40.0f, 0.1f });

			scene->AddNamedChild(std::move(cubeMesh), "base");
		}
	}

	// load BIN model
	ModelLoader loader;
	if (auto object = loader.LoadModel("kenney_car-kit-obj/race-future", true))
	{
		object->program = shaderManager.LoadProgram("vs_model", "fs_model");
		//textureManager.LoadTexture("fieldstone-rgba.dds");
		//textureManager.LoadTexture("fieldstone-n.dds");
		object->ScaleRotationPosition(
		    { 1.0f, 1.0f, 1.0f },
		    { -0.3f, 2.5f, 0.0f },
		    { 0.0f, 1.0f, 0.0f }
		);
		object->CreateShapeBody(physics.get(), ShapeType_ConvexHull, 3.0f);
		scene->AddNamedChild(object, "car");
	}
	if (auto object = loader.LoadModel("building-n", true))
	{
		object->program = shaderManager.LoadProgram("vs_model", "fs_model");
		object->ScaleRotationPosition(
		    { 1.0f, 1.0f, 1.0f },
		    { 0.0f, 1.5f, 0.0f },
		    { 3.0f, -1.5f, -2.5f }
		);
		object->CreateShapeBody(physics.get(), ShapeType_TriangleMesh, 0.0f);
		scene->AddNamedChild(object, "building");
	}
	if (auto object = loader.LoadModel("bunny_decimated", true))
	{
		object->program = shaderManager.LoadProgram("vs_mesh", "fs_mesh");
		object->ScaleRotationPosition(
		    { 1.0f, 1.0f, 1.0f },
		    { 0.0f, 1.5f, 0.0f },
		    { -3.0f, 5.0f, 0.0f }
		);
		object->CreateShapeBody(physics.get(), ShapeType_Cylinder, 3.0f);
		scene->AddNamedChild(object, "bunny");
	}
	if (auto object = loader.LoadModel("donut"))
	{
		object->program = shaderManager.LoadProgram("vs_model", "fs_model");
		object->ScaleRotationPosition(
		    { 1.5f, 1.5f, 1.5f },
		    { 0.0f, 3.0f, 0.0f },
		    { 0.0f, 1.0f, -2.0f }
		);
		object->CreateShapeBody(physics.get(), ShapeType_Box, 1.0f);
		scene->AddNamedChild(object, "donut");
	}

	// donuts
	if (auto parent = loader.LoadModel("donut3"))
	{
		//parent->type |= ObjectType_Group;
		parent->type |= ObjectType_Group | ObjectType_Instance;
		parent->program = shaderManager.LoadProgram("vs_instancing", "fs_instancing");
		scene->AddNamedChild(parent, "donut3-group");

		for (int i = 0; i < 120; ++i)
		{
			//if (auto object = std::make_shared<Mesh>())
			if (auto object = loader.LoadModel("donut3"))
			{
				object->type |= ObjectType_Instance;

				const float radius = sinf(i * 0.02f) * 7.0f;
				const float scale  = MerseneFloat(0.25f, 0.75f);
				const float scaleY = scale * MerseneFloat(0.7f, 1.5f);

				//object->program = shaderManager.LoadProgram("vs_model", "fs_model");
				//object->program = shaderManager.LoadProgram("vs_cube", "fs_cube");
				object->ScaleRotationPosition(
				    { scale, scaleY, scale },
				    { sinf(i * 0.3f), 3.0f, 0.0f },
				    { cosf(i * 0.2f) * radius, 6.0f + i * 0.5f, sinf(i * 0.2f) * radius });
				// TODO: allow to reuse the parent mesh
				object->CreateShapeBody(physics.get(), (i & 7) ? ShapeType_Box : ShapeType_Cylinder, 1.0f);

				object->name = fmt::format("donut3-{}", i);
				parent->AddChild(object);
				//scene->AddNamedChild(object, fmt::format("donut3-{}", i));
			}
		}
	}

	// print scene children
	{
		ui::Log("Scene.children={}", scene->children.size());
		for (int i = -1; const auto& child : scene->children)
			ui::Log(" {:2}: {}", ++i, child->name);
	}

	startTime = bx::getHPCounter();
	lastUs    = NowUs();
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WINDOW
/////////

void App::Render()
{
	// 1) update time
	{
		curTime   = TO_FLOAT((bx::getHPCounter() - startTime) / TO_DOUBLE(bx::getHPFrequency()));
		deltaTime = curTime - lastTime;
	}

	// 2) controls
	Controls();

	// 3) camera view
	{
		bgfx::setViewRect(0, 0, 0, screenX, screenY);

		// view
		float view[16];
		cameraGetViewMtx(view);

		// projection
		const float fscreenX  = TO_FLOAT(screenX);
		const float fscreenY  = TO_FLOAT(screenY);
		const bool  homoDepth = bgfx::getCaps()->homogeneousDepth;
		float       proj[16];

		if (isPerspective)
			bx::mtxProj(proj, 60.0f, fscreenX / fscreenY, 0.1f, 2000.0f, homoDepth);
		else
		{
			float       zoom  = 0.02f;
			const float zoomX = fscreenX * zoom;
			const float zoomY = fscreenY * zoom;
			bx::mtxOrtho(proj, -zoomX, zoomX, -zoomY, zoomY, -1000.0f, 1000.0f, 0.0f, homoDepth);
		}

		bgfx::setViewTransform(0, view, proj);
	}

	// 4) physics
	{
		physics->StepSimulation(deltaTime);
		lastTime = curTime;

		for (auto& child : scene->children)
			child->SynchronizePhysics();

		if (bulletDebug) physics->DrawDebug();
	}

	// 5) draw the scene
	scene->RenderScene(0, screenX, screenY);
}

void App::SynchronizeEvents(uint32_t _screenX, uint32_t _screenY)
{
	if (screenX != _screenX || screenY != _screenY)
	{
		ui::Log("Resize {}x{} => {}x{}", screenX, screenY, _screenX, _screenY);
		screenX = _screenX;
		screenY = _screenY;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ENTRY
////////

class EntryApp : public entry::AppI
{
private:
	uint32_t m_debug  = 0;
	uint32_t m_height = 800;
	uint32_t m_reset  = 0;
	uint32_t m_width  = 1328;

	std::unique_ptr<App> app;
	entry::MouseState    m_mouseState = {};

public:
	EntryApp(const char* _name, const char* _description, const char* _url)
	    : entry::AppI(_name, _description, _url)
	{
	}

	virtual void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		// 1) bgfx
		{
			m_width  = _width;
			m_height = _height;
			m_debug  = 0
			    | BGFX_DEBUG_PROFILER
			    | BGFX_DEBUG_TEXT;
			m_reset = 0
			    //| BGFX_RESET_HDR10
			    | BGFX_RESET_MSAA_X8
			    | BGFX_RESET_VSYNC;

			bgfx::Init init;
			init.type              = args.m_type;
			init.vendorId          = args.m_pciId;
			init.platformData.nwh  = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
			init.platformData.ndt  = entry::getNativeDisplayHandle();
			init.platformData.type = entry::getNativeWindowHandleType();
			init.resolution.width  = m_width;
			init.resolution.height = m_height;
			init.resolution.reset  = m_reset;
			bgfx::init(init);

			bgfx::setDebug(m_debug);

			// set view 0 clear state
			bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		}

		// 2) imGui
		imguiCreate();

		// 3) app
		{
			app = std::make_unique<App>();
			app->Initialize();
		}
	}

	virtual int shutdown() override
	{
		// Reversed order:
		// 3) app
		app.reset();

		// 2) imGui
		imguiDestroy();

		// 1) bgfx
		bgfx::shutdown();
		return 0;
	}

	virtual bool update() override
	{
		// 1) events
		{
			if (entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState)) return false;

			app->SynchronizeEvents(m_width, m_height);
		}

		// 2) imGui
		{
			imguiBeginFrame(m_mouseState.m_mx, m_mouseState.m_my, (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0) | (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0) | (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0), m_mouseState.m_mz, uint16_t(m_width), uint16_t(m_height));
			{
				showExampleDialog(this);
				app->MainUi();
			}
			imguiEndFrame();
		}

		// 3) render
		{
			bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
			bgfx::touch(0);
			app->Render();
		}

		// 4) frame
		bgfx::frame();
		return true;
	}
};

ENTRY_IMPLEMENT_MAIN(EntryApp, "App", "Simple App", "https://shark-it.be");
