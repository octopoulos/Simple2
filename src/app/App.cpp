// App.cpp
// @author octopoulos
// @version 2025-09-11
//
// export DYLD_LIBRARY_PATH=/opt/homebrew/lib

#include "stdafx.h"
#include "app/App.h"
//
#include "entry/input.h"               // GetGlobalInput
#include "loaders/MeshLoader.h"        // MeshLoader::
#include "materials/MaterialManager.h" // GetMaterialManager
#include "materials/ShaderManager.h"   // GetShaderManager
#include "scenes/Scene.h"              // Scene
#include "textures/TextureManager.h"   // GetTextureManager
#include "ui/ui.h"                     // ui::
//
#include "common/bgfx_utils.h"          // Args
#include "common/debugdraw/debugdraw.h" // DebugDrawEncoder
#include "common/ffmpeg-pipe.h"         // FfmpegPipe
#include "common/imgui/imgui.h"         // imguiCreate, imguiDestroy

#include "AI/export.h" // AiTests

#include <bimg/bimg.h>

#ifdef _WIN32
#	include <windows.h>
#endif

#include <CLI/CLI.hpp>

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
	// camera/cursor/mapNode are shared with the App => not destroyed with the scene
	{
		camera.reset();
		cursor.reset();
		mapNode.reset();
	}
	scene.reset();

	physics.reset();

	GetMaterialManager().Destroy();
	GetShaderManager().Destroy();
	GetTextureManager().Destroy();

	// destroy uniforms
	BGFX_DESTROY(uCursorCol);
	BGFX_DESTROY(uLightDir);
	BGFX_DESTROY(uTime);
}

int App::Initialize(std::shared_ptr<App>& app)
{
	// 1) app directory
	ui::Log("App::Initialize");
	FindAppDirectory(true);

	// 2) load imgui.ini
	{
		InitializeImGui();
		ImGui::LoadIniSettingsFromDisk(imguiPath.string().c_str());

		ui::ListWindows(app);
		ui::UpdateTheme();
	}

	GetShaderManager().InitializeUniforms();

	// 3) scan models
	ScanModels("runtime/models", "runtime/models-prev");

	// 4) physics
	physics = std::make_unique<PhysicsWorld>();

	// 5) open/create scene
	if (const int result = InitializeScene(); result < 0) return result;

	// 6) uniforms
	{
		uCursorCol = bgfx::createUniform("u_cursorCol", bgfx::UniformType::Vec4);
		uLightDir  = bgfx::createUniform("u_lightDir" , bgfx::UniformType::Vec4);
		uTime      = bgfx::createUniform("u_time"     , bgfx::UniformType::Vec4);
	}

	// 7) extra inits
	{
		ddInit();
	}

	// 8) time
	startTime = bx::getHPCounter();
	lastUs    = NowUs();

	return 1;
}

int App::InitializeScene()
{
	// 1) create a new scene + add default objects
	{
		scene = std::make_unique<Scene>();

		// camera
		{
			camera = std::make_shared<Camera>("Camera");
			scene->AddChild(camera);
		}

		// cursor
		{
			cursor = std::make_shared<Mesh>("Cursor", ObjectType_Cursor);
			scene->AddChild(cursor);

			cursor->geometry = CreateBoxGeometry(1.0f, 2.0f, 1.0f, 2, 2, 2);
			cursor->material = GetMaterialManager().LoadMaterial("cursor", "vs_cursor", "fs_cursor");
			cursor->material->state = 0
				| BGFX_STATE_BLEND_ALPHA
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_WRITE_A
				| BGFX_STATE_WRITE_RGB
				| BGFX_STATE_WRITE_Z;

			cursor->ScaleIrotPosition(
				{ 1.02f, 1.02f, 1.02f },
				{ 0, 0, 0 },
				{ 0.5f, 1.0f, 0.5f }
			);
		}

		// mapNode
		{
			mapNode = std::make_shared<Object3d>("Map", ObjectType_Group | ObjectType_Map);
			scene->AddChild(mapNode);
		}
	}

	// 2) load a scene
	if (xsettings.autoLoad)
	{
		if (auto recent = xsettings.recentFiles[0]; *recent)
			OpenScene(recent);
	}
	// 3) default scene
	else
	{
		// 4) default objects
		{
			// Earth
			{
				auto cubeMesh      = std::make_shared<Mesh>("Earth");
				cubeMesh->geometry = CreateIcosahedronGeometry(3.0f, 8);
				cubeMesh->material = GetMaterialManager().LoadMaterial("earth", "vs_model_texture_normal", "fs_model_texture_normal", { "earth_day_4096.jpg", "earth_normal_2048.jpg" });

				cubeMesh->ScaleIrotPosition(
					{ 1.0f, 1.0f, 1.0f },
					{ 180, -45, 0 },
					{ 0.0f, 7.0f, 0.0f }
				);
				cubeMesh->CreateShapeBody(GetPhysics(), ShapeType_Sphere, 8.0f);

				scene->AddChild(std::move(cubeMesh));
			}

			// floor
			{
				auto cubeMesh      = std::make_shared<Mesh>("floor");
				cubeMesh->geometry = CreateBoxGeometry(40.0f, 2.0f, 40.0f, 4, 1, 4);
				cubeMesh->material = GetMaterialManager().LoadMaterial("floor", "vs_model_texture", "fs_model_texture", { "FloorsCheckerboard_S_Diffuse.jpg", "FloorsCheckerboard_S_Normal.jpg" });

				cubeMesh->ScaleIrotPosition(
					{ 1.0f, 1.0f, 1.0f },
					{ 0, 0, 0 },
					{ 0.0f, -1.0f, 0.0f }
				);
				cubeMesh->CreateShapeBody(GetPhysics(), ShapeType_Box, 0.0f);

				scene->AddChild(std::move(cubeMesh));
			}

			// 4 walls
			{
				auto parent = std::make_shared<Mesh>("wall-group");

				for (int i = 0; i < 2; ++i)
				{
					auto cubeMesh      = std::make_shared<Mesh>(fmt::format("wall-{}", 1 + i));
					cubeMesh->geometry = CreateBoxGeometry(39.0f, 6.0f, 1.0f, 4, 1, 4);
					cubeMesh->material = GetMaterialManager().LoadMaterial("brick", "vs_model_texture", "fs_model_texture", { "brick_diffuse.jpg" });

					cubeMesh->ScaleIrotPosition(
						{ 1.0f, 1.0f, 1.0f },
						{ 0, 0, 0 },
						{ i ? -0.5f : 0.5f, 3.0f, i ? -19.5f : 19.5f }
					);
					cubeMesh->CreateShapeBody(GetPhysics(), ShapeType_Box, 0.0f);

					parent->AddChild(std::move(cubeMesh));
				}
				for (int i = 0; i < 2; ++i)
				{
					auto cubeMesh      = std::make_shared<Mesh>(fmt::format("wall-{}", 3 + i));
					cubeMesh->geometry = CreateBoxGeometry(1.0f, 6.0f, 39.0f, 4, 1, 4);
					cubeMesh->material = GetMaterialManager().LoadMaterial("wood", "vs_model_texture", "fs_model_texture", { "hardwood2_diffuse.jpg" });

					cubeMesh->ScaleIrotPosition(
						{ 1.0f, 1.0f, 1.0f },
						{ 0, 0, 0 },
						{ i ? 19.5f : -19.5f, 3.0f, i ? -0.5f : 0.5f }
					);
					cubeMesh->CreateShapeBody(GetPhysics(), ShapeType_Box, 0.0f);

					parent->AddChild(std::move(cubeMesh));
				}

				scene->AddChild(std::move(parent));
			}
		}

		// 5) load BIN model
		if (auto object = MeshLoader::LoadModelFull("car", "kenney_car-kit/race-future"))
		{
			object->ScaleRotationPosition(
				{ 1.0f, 1.0f, 1.0f },
				{ -0.3f, 2.5f, 0.0f },
				{ 0.0f, 1.0f, 0.0f }
			);
			object->CreateShapeBody(GetPhysics(), ShapeType_ConvexHull, 3.0f);
			scene->AddChild(object);
		}
		if (auto object = MeshLoader::LoadModelFull("building", "kenney_city-kit-commercial_20/building-n"))
		{
			object->ScaleIrotPosition(
				{ 1.0f, 1.0f, 1.0f },
				{ 0, 90, 0 },
				{ 4.0f, 0.0f, -2.0f }
			);
			object->CreateShapeBody(GetPhysics(), ShapeType_TriangleMesh);
			scene->AddChild(object);
		}
		if (auto object = MeshLoader::LoadModel("bunny", "bunny_decimated", true))
		{
			object->material = GetMaterialManager().LoadMaterial("mesh", "vs_mesh", "fs_mesh");
			object->ScaleIrotPosition(
				{ 1.0f, 1.0f, 1.0f },
				{ 0, 90, 0 },
				{ -3.0f, 5.0f, 0.0f }
			);
			object->CreateShapeBody(GetPhysics(), ShapeType_Cylinder, 3.0f);
			scene->AddChild(object);
		}
		if (auto object = MeshLoader::LoadModel("donut", "donut"))
		{
			object->material = GetMaterialManager().LoadMaterial("model", "vs_model", "fs_model");
			object->ScaleIrotPosition(
				{ 1.5f, 1.5f, 1.5f },
				{ 0, 180, 0 },
				{ 0.0f, 1.0f, -2.0f }
			);
			object->CreateShapeBody(GetPhysics(), ShapeType_Box, 1.0f);
			scene->AddChild(object);
		}
	}

	// 6) print scene children
	{
		ui::Log("Scene.children={}", scene->children.size());
		for (int i = -1; const auto& child : scene->children)
			ui::Log(" {:2}: {}", ++i, child->name);
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RENDER
/////////

void App::Render()
{
	// hack to remove focus from "Example: App"
	if (renderFrame < 1) FocusScreen();

	// 1) uniforms
	{
		const bool isCursor = camera->follow & CameraFollow_Cursor;
		if (auto target = isCursor ? cursor : selectWeak.lock())
		{
			const float y0 = isCursor ? 1.0f : 0.0f;

			glm::vec3 cursorCol = { 0.7f, 0.6f, 0.0f };
			if (target->position.y > y0)
				cursorCol = { 0.0f, 0.5f, 1.0f };
			else if (target->position.y < y0)
				cursorCol = { 1.0f, 0.5f, 0.0f };

			bgfx::setUniform(uCursorCol, VALUE_VEC4(cursorCol, 0.0f));
		}

		//const auto lightDir = glm::normalize(glm::vec3(sinf(curTime), 1.0f, cosf(curTime)));
		const auto lightDir = glm::normalize(glm::make_vec3(xsettings.lightDir));
		bgfx::setUniform(uLightDir, VALUE_VEC4(lightDir, 0.0f));

		bgfx::setUniform(uTime, VALUE_VEC4(curTime, 0.0f, 0.0f, 0.0f));
	}

	// 2) camera view
	camera->UpdateViewProjection(0, TO_FLOAT(screenX), TO_FLOAT(screenY));

	// 3) physics
	{
		if (!xsettings.physPaused)
		{
			physics->StepSimulation(deltaTime);
			++physicsFrame;

			if (pauseNextFrame)
			{
				xsettings.physPaused = true;
				pauseNextFrame       = false;
			}
		}

		scene->SynchronizePhysics();
	}

	// 4) draw grid
	if (xsettings.gridDraw)
	{
		DebugDrawEncoder dde;

		dde.begin(0);
		dde.drawGrid(Axis::Y, { 0.0f, 0.0f, 0.0f }, xsettings.gridSize);
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
	std::shared_ptr<App> app      = nullptr; ///< main application
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

	virtual void init(int32_t argc, const char* const* argv, uint32_t width, uint32_t height) override
	{
		Args args(argc, argv);

		// 0) CLI
		auto cliFunc = [&]() -> int
		{
			CLI_BEGIN("Simple");

			// name, type, default, implicit, needApp, help
			// clang-format off
			CLI_OPTION(tests, int, 0, 1, 0, "Run tests");
			// clang-format on

			if (argc && argv) CLI11_PARSE(cli, argc, argv);
			CLI_NEEDAPP();

			if (tests)
			{
				const int res = AiTests(tests, argc, (char**)argv);
				if (!(tests & 4) && (res || (tests & 2))) return res;
			}
			return 0;
		};
		if (const int result = cliFunc()) return;

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
			app = std::make_shared<App>();
			app->Initialize(app);
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
