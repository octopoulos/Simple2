// app.cpp
// @author octopoulos
// @version 2025-07-25
//
// export DYLD_LIBRARY_PATH=/opt/homebrew/lib

#include "stdafx.h"
#include "app.h"
#include "common/bgfx_utils.h"
#include "common/ffmpeg-pipe.h"
#include "common/entry/input.h"
#include "engine/ModelLoader.h"
#include "imgui/imgui.h"

#ifdef _WIN32
#	include <windows.h>
#endif

extern std::string VERSION;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INIT
///////

void App::Destroy()
{
	// make sure that everything is removed from the physical world before deleting physics
	// mapNode is shared with the App => not destroyed with the scene
	mapNode.reset();
	scene.reset();

	physics.reset();
}

int App::Initialize()
{
	ui::Log("App::Initialize");
	FindAppDirectory(true);
	ScanModels("runtime/models", "runtime/models-prev");

	if (const int result = InitializeScene(); result < 0) return result;

	ui::Log("xsettings.aspectRatio={}", xsettings.aspectRatio);
	return 1;
}

int App::InitializeScene()
{
	// 1)
	scene = std::make_unique<Scene>();

	// camera
	{
		camera = std::make_shared<Camera>();
		scene->AddNamedChild(camera, "camera");
	}

	// cursor
	{
		cursor = std::make_shared<Mesh>();
		scene->AddNamedChild(cursor, "cursor");
	}

	// mapNode
	{
		mapNode = std::make_shared<Object3d>();
		mapNode->type |= ObjectType_Group;
		scene->AddNamedChild(mapNode, "map");
	}

	// 2)
	physics = std::make_unique<PhysicsWorld>();

	// 3) cube vertex layout
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

		// cursor
		{
			cursor->geometry = std::make_shared<Geometry>(vbh, ibh);
			cursor->material = std::make_shared<Material>(shaderManager.LoadProgram("vs_cursor", "fs_cursor"));

			cursor->ScaleRotationPosition(
			    { 0.5f, 1.0f, 0.5f },
			    { 0.0f, 0.0f, 0.0f },
			    { 0.0f, 0.0f, 0.0f });
		}

		// floor
		{
			auto cubeMesh      = std::make_shared<Mesh>();
			cubeMesh->geometry = std::make_shared<Geometry>(vbh, ibh);
			cubeMesh->material = std::make_shared<Material>(shaderManager.LoadProgram("vs_cube", "fs_cube"));

			cubeMesh->ScaleRotationPosition(
			    { 9.0f, 1.0f, 9.0f },
			    { 0.0f, 0.0f, 0.0f },
			    { 0.0f, -2.0f, 0.0f }
			);
			cubeMesh->CreateShapeBody(physics.get(), ShapeType_Box, 0.0f, { 9.0f, 1.0f, 9.0f, 0.0f });

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

	// 4) load BIN model
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
		    { 4.0f, -1.0f, -2.0f }
		);
		object->CreateShapeBody(physics.get(), ShapeType_TriangleMesh);
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
		parent->program = shaderManager.LoadProgram("vs_model_instance", "fs_model_instance");
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

				object->program = shaderManager.LoadProgram("vs_model", "fs_model");
				object->ScaleRotationPosition(
				    { scale, scaleY, scale },
				    { sinf(i * 0.3f), 3.0f, 0.0f },
				    { cosf(i * 0.2f) * radius, 6.0f + i * 0.5f, sinf(i * 0.2f) * radius });
				// TODO: allow to reuse the parent mesh
				object->CreateShapeBody(physics.get(), (i & 7) ? ShapeType_Box : ShapeType_Cylinder, 1.0f);

				parent->AddNamedChild(object, fmt::format("donut3-{}", i));
			}
		}
	}

	// print scene children
	{
		ui::Log("Scene.children={}", scene->children.size());
		for (int i = -1; const auto& child : scene->children)
			ui::Log(" {:2}: {}", ++i, child->name);
	}

	// 5) uniforms
	{
		uTime = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
	}

	// 6) time
	startTime = bx::getHPCounter();
	lastUs    = NowUs();
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RENDER
/////////

void App::Render()
{
	// 1) update time
	{
		curTime   = TO_FLOAT((bx::getHPCounter() - startTime) / TO_DOUBLE(bx::getHPFrequency()));
		deltaTime = curTime - lastTime;
		lastTime  = curTime;

		float timeVec[4] = { curTime, 0.0f, 0.0f, 0.0f };
		bgfx::setUniform(uTime, timeVec);
	}

	// 2) controls
	FluidControls();
	{
		inputLag += deltaTime;
		while (inputLag >= inputDelta)
		{
			FixedControls();
			inputLag -= inputDelta;
		}
	}

	// 3) camera view
	camera->UpdateViewProjection(0, TO_FLOAT(screenX), TO_FLOAT(screenY));

	// 4) physics
	if (!xsettings.physPaused)
	{
		physics->StepSimulation(deltaTime);
		++physicsFrame;

		for (auto& child : scene->children)
			child->SynchronizePhysics();

		if (xsettings.bulletDebug) physics->DrawDebug();

		if (pauseNextFrame)
		{
			xsettings.physPaused = true;
			pauseNextFrame       = false;
		}
	}

	// 5) draw the scene
	{
		renderFlags = 0;
		if (xsettings.instancing) renderFlags |= RenderFlag_Instancing;

		scene->RenderScene(0, renderFlags);
	}

	// 6) capture screenshot?
	if (wantScreenshot)
	{
		// ignore this frame to give time to hide the GUI
		if (wantScreenshot & 4)
			wantScreenshot &= ~4;
		else
		{
			wantScreenshot = 0;
			if (CreateDirectories("temp"))
			{
				bgfx::FrameBufferHandle fbh = BGFX_INVALID_HANDLE;
				bgfx::requestScreenShot(fbh, fmt::format("temp/{}.png", FormatDateTime(2, true)).c_str());
			}
		}
	}

	++renderFrame;
	if (wantVideo) ++videoFrame;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SETTINGS
///////////

void App::InitializeImGui()
{
	static char iniFilename[512];

	imguiPath = ConfigFolder() / "imgui.ini";
	strcpy(iniFilename, imguiPath.string().c_str());
	ImGuiIO& io    = ImGui::GetIO();
	io.IniFilename = iniFilename;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WINDOW
/////////

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

struct BgfxCallback : public bgfx::CallbackI
{
	FfmpegPipe ffmpeg     = {};    ///
	uint32_t   height     = 0;     ///
	int        numFailure = 0;     ///< how many times ffmpeg.Open failed
	uint32_t   pitch      = 0;     ///
	bool       wantVideo  = false; ///< capturing?
	uint32_t   width      = 0;     ///
	bool       yflip      = false; ///

	virtual void captureBegin(uint32_t _width, uint32_t _height, uint32_t _pitch, bgfx::TextureFormat::Enum _format, bool _yflip) override
	{
		height = _height;
		pitch  = _pitch;
		width  = _width;
		yflip  = _yflip;
	}

	virtual void captureEnd() override
	{
		ffmpeg.Close();
	}

	virtual void captureFrame(const void* data, uint32_t size) override
	{
		if (!wantVideo) return;

		if (!ffmpeg.pipe)
		{
			if (numFailure > 3)
				wantVideo = false;
			else if (!ffmpeg.Open(fmt::format("temp/{}.mp4", FormatDateTime(2, true)), width, height, 60, yflip))
				++numFailure;
		}
		else ffmpeg.WriteFrame(data, size);
	}

	virtual void screenShot(const char* _filePath, uint32_t _width, uint32_t _height, uint32_t _pitch, const void* _data, uint32_t _size, bool _yflip) override
	{
		bx::FileWriter writer;
		bx::Error      err;
		if (bx::open(&writer, _filePath, false, &err))
		{
			bimg::imageWritePng(&writer, _width, _height, _pitch, _data, bimg::TextureFormat::BGRA8, _yflip, &err);
			bx::close(&writer);
		}
	}

	// clang-format off
	virtual bool     cacheRead(uint64_t _id, void* _data, uint32_t _size) override { return false; }
	virtual uint32_t cacheReadSize(uint64_t _id) override { return 0; }
	virtual void     cacheWrite(uint64_t _id, const void* _data, uint32_t _size) override {}
	virtual void     fatal(const char* _filePath, uint16_t _line, bgfx::Fatal::Enum _code, const char* _str) override {}
	virtual void     profilerBegin(const char* _name, uint32_t _abgr, const char* _filePath, uint16_t _line) override {}
	virtual void     profilerBeginLiteral(const char* _name, uint32_t _abgr, const char* _filePath, uint16_t _line) override {}
	virtual void     profilerEnd() override {}
	virtual void     traceVargs(const char* _filePath, uint16_t _line, const char* _format, va_list _argList) override {}
	// clang-format on
};

class EntryApp : public entry::AppI
{
private:
	std::unique_ptr<App> app      = nullptr; ///< main application
	BgfxCallback         callback = {};      ///< for video capture + screenshot
	uint32_t             debug    = 0;       ///
	uint32_t             height   = 800;     ///
	uint32_t             reset    = 0;       ///
	uint32_t             width    = 1328;    ///

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
			width  = _width;
			height = _height;
			debug  = 0
			    | BGFX_DEBUG_PROFILER
			    | BGFX_DEBUG_TEXT;
			reset = 0
			    //| BGFX_RESET_HDR10
			    | (xsettings.videoCapture ? BGFX_RESET_CAPTURE : 0)
			    | BGFX_RESET_MSAA_X8
			    | BGFX_RESET_VSYNC;

			bgfx::Init init;
			init.type              = args.m_type;
			init.vendorId          = args.m_pciId;
			init.platformData.nwh  = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
			init.platformData.ndt  = entry::getNativeDisplayHandle();
			init.platformData.type = entry::getNativeWindowHandleType();
			init.resolution.width  = width;
			init.resolution.height = height;
			init.resolution.reset  = reset;
			init.callback          = &callback;
			bgfx::init(init);

			bgfx::setDebug(debug);

			// set view 0 clear state
			bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
		}

		// 2) imGui
		imguiCreate();

		// 3) app
		{
			app = std::make_unique<App>();
			app->Initialize();
			app->entryReset = &reset;
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
			callback.wantVideo = app->wantVideo;

			if (entry::processEvents(width, height, debug, reset)) return false;
			app->SynchronizeEvents(width, height);
		}

		// 2) imGui
		if (!(app->wantScreenshot & 2))
		{
			// TODO: move this inside imguiBeginFrame
			const auto& ginput   = GetGlobalInput();
			const auto& buttons  = ginput.buttons;
			const auto  imButton = (buttons[1] ? IMGUI_MBUT_LEFT : 0) | (buttons[3] ? IMGUI_MBUT_RIGHT : 0) | (buttons[2] ? IMGUI_MBUT_MIDDLE : 0);
			const auto& mouseAbs = ginput.mouseAbs;

			imguiBeginFrame(mouseAbs[0], mouseAbs[1], imButton, mouseAbs[2], uint16_t(width), uint16_t(height));
			{
				if (!app->wantVideo) showExampleDialog(this);
				app->MainUi();
			}
			imguiEndFrame();
		}

		// 3) render
		{
			bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));
			bgfx::touch(0);

			app->Render();
		}

		// 4) frame
		bgfx::frame();
		return true;
	}
};

ENTRY_IMPLEMENT_MAIN(EntryApp, "App", VERSION.c_str(), "https://shark-it.be");
