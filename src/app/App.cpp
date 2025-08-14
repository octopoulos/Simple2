// App.cpp
// @author octopoulos
// @version 2025-08-10
//
// export DYLD_LIBRARY_PATH=/opt/homebrew/lib

#include "stdafx.h"
#include "app/App.h"
//
#include "core/ShaderManager.h"
#include "entry/input.h"
#include "loaders/MeshLoader.h"
#include "textures/TextureManager.h"
#include "ui/ui.h"

#include "common/bgfx_utils.h"
#include "common/debugdraw/debugdraw.h"
#include "common/ffmpeg-pipe.h"
#include "common/imgui/imgui.h"

#ifdef _WIN32
#	include <windows.h>
#endif

extern std::string VERSION;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INIT
///////

void App::Destroy()
{
	// synchronize settings at exit
	ui::SaveWindows();
	bx::store(xsettings.cameraEye, camera->pos2);
	bx::store(xsettings.cameraAt, camera->target2);

	// make sure that everything is removed from the physical world before deleting physics
	// mapNode is shared with the App => not destroyed with the scene
	mapNode.reset();
	scene.reset();

	physics.reset();

	GetShaderManager().Destroy();
	GetTextureManager().Destroy();

	bgfx::destroy(uLight);
	bgfx::destroy(uTime);
}

int App::Initialize()
{
	ui::Log("App::Initialize");
	FindAppDirectory(true);

	// load imgui.ini
	{
		InitializeImGui();
		ImGui::LoadIniSettingsFromDisk(imguiPath.string().c_str());

		ui::ListWindows(this);
		ui::UpdateTheme();
	}

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
		cursor->state = 0
		    | BGFX_STATE_BLEND_ALPHA
		    | BGFX_STATE_DEPTH_TEST_LESS
		    | BGFX_STATE_WRITE_A
		    | BGFX_STATE_WRITE_RGB
		    | BGFX_STATE_WRITE_Z;
	}

	// mapNode
	{
		mapNode = std::make_shared<Object3d>();
		mapNode->type |= ObjectType_Group;
		scene->AddNamedChild(mapNode, "map");
	}

	// 2)
	physics = std::make_unique<PhysicsWorld>();

	// 3) add default objects
	{
		// Earth
		{
			auto cubeMesh      = std::make_shared<Mesh>();
			cubeMesh->geometry = CreateIcosahedronGeometry(3.0f, 8);
			cubeMesh->material = std::make_shared<Material>("vs_model_texture_normal", "fs_model_texture_normal");
			cubeMesh->LoadTextures("earth_day_4096.jpg", "earth_normal_2048.jpg");

			cubeMesh->ScaleRotationPosition(
			    { 1.0f, 1.0f, 1.0f },
			    { bx::kPi, -bx::kPiQuarter, 0.0f },
			    { 0.0f, 7.0f, 0.0f }
			);
			cubeMesh->CreateShapeBody(physics.get(), ShapeType_Sphere, 8.0f);

			scene->AddNamedChild(std::move(cubeMesh), "Earth");
		}

		// cursor
		{
			cursor->geometry = CreateBoxGeometry(1.0f, 2.0f, 1.0f, 2, 2, 2);
			cursor->material = std::make_shared<Material>("vs_cursor", "fs_cursor");

			cursor->ScaleRotationPosition(
			    { 1.0f, 1.02f, 1.0f },
			    { 0.0f, 0.0f, 0.0f },
			    { 0.5f, 1.0f, 0.5f }
			);
		}

		// floor
		{
			auto cubeMesh      = std::make_shared<Mesh>();
			cubeMesh->geometry = CreateBoxGeometry(40.0f, 2.0f, 40.0f, 4, 1, 4);
			cubeMesh->material = std::make_shared<Material>("vs_model_texture", "fs_model_texture");
			cubeMesh->LoadTextures("FloorsCheckerboard_S_Diffuse.jpg", "FloorsCheckerboard_S_Normal.jpg");

			cubeMesh->ScaleRotationPosition(
			    { 1.0f, 1.0f, 1.0f },
			    { 0.0f, 0.0f, 0.0f },
			    { 0.0f, -1.0f, 0.0f }
			);
			cubeMesh->CreateShapeBody(physics.get(), ShapeType_Box, 0.0f);

			scene->AddNamedChild(std::move(cubeMesh), "floor");
		}

		// 4 walls
		{
			auto parent = std::make_shared<Mesh>();

			for (int i = 0; i < 2; ++i)
			{
				auto cubeMesh      = std::make_shared<Mesh>();
				cubeMesh->geometry = CreateBoxGeometry(39.0f, 6.0f, 1.0f, 4, 1, 4);
				cubeMesh->material = std::make_shared<Material>("vs_model_texture", "fs_model_texture");
				cubeMesh->LoadTextures("brick_diffuse.jpg");

				cubeMesh->ScaleRotationPosition(
				    { 1.0f, 1.0f, 1.0f },
				    { 0.0f, 0.0f, 0.0f },
				    { i ? -0.5f : 0.5f, 3.0f, i ? -19.5f : 19.5f }
				);
				cubeMesh->CreateShapeBody(physics.get(), ShapeType_Box, 0.0f);

				parent->AddNamedChild(std::move(cubeMesh), fmt::format("wall-{}", 1 + i));
			}
			for (int i = 0; i < 2; ++i)
			{
				auto cubeMesh      = std::make_shared<Mesh>();
				cubeMesh->geometry = CreateBoxGeometry(1.0f, 6.0f, 39.0f, 4, 1, 4);
				cubeMesh->material = std::make_shared<Material>("vs_model_texture", "fs_model_texture");
				cubeMesh->LoadTextures("hardwood2_diffuse.jpg");

				cubeMesh->ScaleRotationPosition(
				    { 1.0f, 1.0f, 1.0f },
				    { 0.0f, 0.0f, 0.0f },
				    { i ? 19.5f : -19.5f, 3.0f, i ? -0.5f : 0.5f }
				);
				cubeMesh->CreateShapeBody(physics.get(), ShapeType_Box, 0.0f);

				parent->AddNamedChild(std::move(cubeMesh), fmt::format("wall-{}", 3 + i));
			}

			scene->AddNamedChild(std::move(parent), "wall-group");
		}
	}

	// 4) load BIN model
	if (auto object = MeshLoader::LoadModelFull("kenney_car-kit/race-future"))
	{
		object->ScaleRotationPosition(
		    { 1.0f, 1.0f, 1.0f },
		    { -0.3f, 2.5f, 0.0f },
		    { 0.0f, 1.0f, 0.0f }
		);
		object->CreateShapeBody(physics.get(), ShapeType_ConvexHull, 3.0f);
		scene->AddNamedChild(object, "car");
	}
	if (auto object = MeshLoader::LoadModelFull("kenney_city-kit-commercial_20/building-n"))
	{
		object->ScaleRotationPosition(
		    { 1.0f, 1.0f, 1.0f },
		    { 0.0f, 1.5f, 0.0f },
		    { 4.0f, 0.0f, -2.0f }
		);
		object->CreateShapeBody(physics.get(), ShapeType_TriangleMesh);
		scene->AddNamedChild(object, "building");
	}
	if (auto object = MeshLoader::LoadModel("bunny_decimated", true))
	{
		object->material = std::make_shared<Material>("vs_mesh", "fs_mesh");
		object->ScaleRotationPosition(
		    { 1.0f, 1.0f, 1.0f },
		    { 0.0f, 1.5f, 0.0f },
		    { -3.0f, 5.0f, 0.0f }
		);
		object->CreateShapeBody(physics.get(), ShapeType_Cylinder, 3.0f);
		scene->AddNamedChild(object, "bunny");
	}
	if (auto object = MeshLoader::LoadModel("donut"))
	{
		object->material = std::make_shared<Material>("vs_model", "fs_model");
		object->ScaleRotationPosition(
		    { 1.5f, 1.5f, 1.5f },
		    { 0.0f, 3.0f, 0.0f },
		    { 0.0f, 1.0f, -2.0f }
		);
		object->CreateShapeBody(physics.get(), ShapeType_Box, 1.0f);
		scene->AddNamedChild(object, "donut");
	}

	// print scene children
	{
		ui::Log("Scene.children={}", scene->children.size());
		for (int i = -1; const auto& child : scene->children)
			ui::Log(" {:2}: {}", ++i, child->name);
	}

	// 5) uniforms
	{
		uLight = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
		uTime  = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
	}

	// 6) extra inits
	{
		ddInit();
	}

	// 7) time
	startTime = bx::getHPCounter();
	lastUs    = NowUs();
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RENDER
/////////

void App::Render()
{
	// 1) uniforms
	{
		//const auto  lightDir    = bx::normalize(bx::Vec3(sinf(curTime), 1.0f, cosf(curTime)));
		const auto  lightDir    = bx::normalize(bx::Vec3(0.5f, 1.0f, -0.5f));
		const float lightUni[4] = { lightDir.x, lightDir.y, lightDir.z, 0.0f };
		bgfx::setUniform(uLight, lightUni);

		const float timeVec[4] = { curTime, 0.0f, 0.0f, 0.0f };
		bgfx::setUniform(uTime, timeVec);
	}

	// 2) camera view
	camera->UpdateViewProjection(0, TO_FLOAT(screenX), TO_FLOAT(screenY));

	// 3) physics
	if (!xsettings.physPaused)
	{
		physics->StepSimulation(deltaTime);
		++physicsFrame;

		for (auto& child : scene->children)
			child->SynchronizePhysics();

		if (pauseNextFrame)
		{
			xsettings.physPaused = true;
			pauseNextFrame       = false;
		}
	}

	// 4) draw grid
	{
		DebugDrawEncoder dde;

		dde.begin(0);
		dde.drawGrid(Axis::Y, { 0.0f, 0.0f, 0.0f }, 50);
		dde.end();
	}

	// 5) draw the scene
	{
		renderFlags = 0;
		if (xsettings.instancing) renderFlags |= RenderFlag_Instancing;

		scene->Render(0, renderFlags);
		if (xsettings.bulletDebug) physics->DrawDebug();
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
		ui::Log("SynchronizeEvents: {}x{} => {}x{}", screenX, screenY, _screenX, _screenY);
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
	uint32_t             fheight  = 800;     ///< framebuffer height
	uint32_t             fwidth   = 1328;    ///< framebuffer width
	uint32_t             iheight  = 800;     ///< original height
	uint32_t             iwidth   = 1328;    ///< original width
	uint32_t             reset    = 0;       ///

public:
	EntryApp(const char* _name, const char* _description, const char* _url)
	    : entry::AppI(_name, _description, _url)
	{
	}

	virtual void init(int32_t _argc, const char* const* _argv, uint32_t width, uint32_t height) override
	{
		Args args(_argc, _argv);

		// 1) bgfx
		{
			iheight = height;
			iwidth  = width;
			fheight = iheight * xsettings.dpr;
			fwidth  = iwidth * xsettings.dpr;

			debug  = 0
			    | BGFX_DEBUG_PROFILER
			    | BGFX_DEBUG_TEXT;
			reset = 0
			    | (xsettings.captureVideo ? BGFX_RESET_CAPTURE : 0)
				| BGFX_RESET_HIDPI
			    //| BGFX_RESET_HDR10
			    | BGFX_RESET_MSAA_X8
			    | BGFX_RESET_VSYNC;

			bgfx::Init init;
			init.type              = args.m_type;
			init.vendorId          = args.m_pciId;
			init.platformData.nwh  = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
			init.platformData.ndt  = entry::getNativeDisplayHandle();
			init.platformData.type = entry::getNativeWindowHandleType();
			init.resolution.width  = fwidth;
			init.resolution.height = fheight;
			init.resolution.reset  = reset;
			init.callback          = &callback;

			ui::Log("App/init: {}x{}x{} => {}x{}", iwidth, iheight, xsettings.dpr, fwidth, fheight);
			bgfx::init(init);

			bgfx::setDebug(debug);

			// set view 0 clear state
			bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x393939ff, 1.0f, 0);
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
		auto& ginput = GetGlobalInput();

		// 1) events
		{
			callback.wantVideo = app->wantVideo;
			ginput.ResetAscii();

			if (entry::processEvents(fwidth, fheight, debug, reset)) return false;

			app->SynchronizeEvents(fwidth, fheight);
			app->Controls();
		}

		// 2) imGui
		if (!(app->wantScreenshot & 2))
		{
			const auto& buttons  = ginput.buttons;
			const auto  imButton = (buttons[1] ? IMGUI_MBUT_LEFT : 0) | (buttons[3] ? IMGUI_MBUT_RIGHT : 0) | (buttons[2] ? IMGUI_MBUT_MIDDLE : 0);
			const auto& mouseAbs = ginput.mouseAbs;

			imguiBeginFrame(mouseAbs[0], mouseAbs[1], imButton, mouseAbs[2], uint16_t(fwidth), uint16_t(fheight), ginput.lastAscii);
			{
				if (app->MainUi() & 2) showExampleDialog(this);
			}
			imguiEndFrame();
		}

		// 3) render
		{
			bgfx::setViewRect(0, 0, 0, uint16_t(fwidth), uint16_t(fheight));
			bgfx::touch(0);

			app->Render();
		}

		// 4) frame
		bgfx::frame();
		return true;
	}
};

ENTRY_IMPLEMENT_MAIN(EntryApp, "App", VERSION.c_str(), "https://shark-it.be");
