// app.cpp
// @author octopoulos
// @version 2025-07-19
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
		cameraSetPosition({ -6.0f, 2.0f, -10.0f });
		cameraSetHorizontalAngle(0.7f);
		cameraSetVerticalAngle(-0.4f);
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

		// cube
		{
			auto cubeMesh      = std::make_shared<Mesh>();
			cubeMesh->geometry = std::make_shared<Geometry>(vbh, ibh);
			cubeMesh->material = std::make_shared<Material>(program);

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
			cubeMesh->material = std::make_shared<Material>(program);

			cubeMesh->ScaleRotationPosition(
			    { 5.5f, 0.5f, 5.5f },
			    { 0.0f, 0.0f, 0.0f },
			    { 0.0f, -2.0f, 0.0f }
			);
			cubeMesh->CreateShapeBody(physics.get(), ShapeType_Box, 0.0f, { 5.5f, 0.5f, 5.5f, 0.0f });

			scene->AddNamedChild(std::move(cubeMesh), "floor");
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
		object->CreateShapeBody(physics.get(), ShapeType_ConvexHull, 3.0f, { 1.0f, 0.5f, 1.0f, 0.0f });
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
		object->CreateShapeBody(physics.get(), ShapeType_TriangleMesh, 0.0f, { 1.0f, 2.1f, 1.0f, 0.0f });
		scene->AddNamedChild(object, "building");
	}
	if (auto object = loader.LoadModel("bunny", true))
	{
		object->program = shaderManager.LoadProgram("vs_mesh", "fs_mesh");
		object->ScaleRotationPosition(
		    { 1.0f, 1.0f, 1.0f },
		    { 0.0f, 1.5f, 0.0f },
		    { -3.0f, 5.0f, 0.0f }
		);
		object->CreateShapeBody(physics.get(), ShapeType_ConvexHull, 3.0f, { 1.0f, 1.0f, 1.0f, 0.0f });
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
		object->CreateShapeBody(physics.get(), ShapeType_Cylinder, 1.0f, { 0.45f, 0.2f, 0.0f, 0.0f });
		scene->AddNamedChild(object, "donut");
	}

	for (int i = 0; i < 30; ++i)
	{
		if (auto object = loader.LoadModel("donut3"))
		{
			object->program     = shaderManager.LoadProgram("vs_model", "fs_model");
			object->scale       = { 0.5f, 0.5f, 0.5f };
			object->scaleMatrix = glm::scale(glm::mat4(1.0f), object->scale);

			object->ScaleRotationPosition(
			    { 0.5f, 0.5f, 0.5f },
			    { sinf(i * 0.3f), 3.0f, 0.0f },
			    { i * 0.4f - 6.0f, sinf(i * 0.5f) + 6.0f, -2.5f + MerseneFloat() }
			);
			object->CreateShapeBody(physics.get(), ShapeType_Cylinder, 1.0f, { 0.31f, 0.11f, 0.3f, 0.0f });

			scene->AddNamedChild(object, fmt::format("donut3-{}", i));
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

		cameraUpdate(deltaTime, mouseState, ImGui::MouseOverArea());
	}

	// 2) camera view
	{
		bgfx::setViewRect(0, 0, 0, screenX, screenY);

		float view[16];
		cameraGetViewMtx(view);

		float proj[16];
		bx::mtxProj(proj, 60.0f, float(screenX) / float(screenY), 0.1f, 2000.0f, bgfx::getCaps()->homogeneousDepth);

		bgfx::setViewTransform(0, view, proj);
	}

	// 3) physics
	{
		physics->StepSimulation(deltaTime);
		lastTime = curTime;

		for (auto& child : scene->children)
		{
			if (child->type == ObjectType_Mesh)
			{
				if (auto mesh = static_cast<Mesh*>(child.get()); mesh->bodies.size())
				{
					const auto& body = mesh->bodies[0];
					if (body->mass >= 0.0f)
					{
						btTransform transform;
						auto        motionState = body->body->getMotionState();
						motionState->getWorldTransform(transform);

						float matrix[16];
						transform.getOpenGLMatrix(matrix);
						mesh->transform = glm::make_mat4(matrix) * mesh->scaleMatrix;
						// mesh->position  = glm::vec3(mesh->transform[3]);
					}
					continue;
				}
			}

			if (child->name == "donut")
			{
				child->ScaleRotationPosition(
				    { 1.5f, 1.5f, 1.5f },
				    { 0.0f, lastTime * 3.0f, 0.0f },
				    { -5.0f, 1.0f, -2.0f }
				);
			}
			else if (child->name == "car")
			{
				child->ScaleRotationPosition(
				    { 1.0f, 1.0f, 1.0f },
				    { lastTime, 6.0f, 0.0f },
				    { 0.0f, 1.0f, -2.0f }
				);
			}
		}

		physics->DrawDebug();
	}

	// 4) draw the scene
	scene->RenderScene(0, screenX, screenY);
}

void App::SynchronizeEvents(uint32_t _screenX, uint32_t _screenY, entry::MouseState& _mouseState)
{
	screenX = _screenX;
	screenY = _screenY;
	std::memcpy(&mouseState, &_mouseState, sizeof(entry::MouseState));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ENTRY
////////

class EntryApp : public entry::AppI
{
public:
	EntryApp(const char* _name, const char* _description, const char* _url)
	    : entry::AppI(_name, _description, _url)
	{
	}

	virtual void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width  = _width;
		m_height = _height;
		m_debug  = 0
		    | BGFX_DEBUG_PROFILER
			| BGFX_DEBUG_TEXT
			;
		m_reset  = 0
		    //| BGFX_RESET_HDR10
		    | BGFX_RESET_MSAA_X8
		    | BGFX_RESET_VSYNC
			;

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

		imguiCreate();

		app = std::make_unique<App>();
		app->Initialize();
	}

	virtual int shutdown() override
	{
		app.reset();

		imguiDestroy();

		bgfx::shutdown();
		return 0;
	}

	virtual bool update() override
	{
		// 1) events
		{
			if (entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState)) return false;
			app->SynchronizeEvents(m_width, m_height, m_mouseState);
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

	entry::MouseState m_mouseState = {};

	uint32_t m_width  = 1328;
	uint32_t m_height = 800;
	uint32_t m_debug  = 0;
	uint32_t m_reset  = 0;

	std::unique_ptr<App> app;
};

ENTRY_IMPLEMENT_MAIN(EntryApp, "App", "Simple App", "https://shark-it.be");
