// app.cpp
// @author octopoulos
// @version 2025-07-06

#include "stdafx.h"
#include "app.h"
#include "common/bgfx_utils.h"
// #include "gui/imgui-bgfx.h"
#include "imgui/imgui.h"

#include "engine/ModelLoader.h"

#ifdef _WIN32
#	include <windows.h>
#endif
#if BX_PLATFORM_OSX
	extern "C" void* GetMacOSMetalLayer(SDL_Window* window);
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A key for the index triplet
struct IndexKey
{
	int pi, ni, ti;

	bool operator==(const IndexKey& other) const
	{
		return pi == other.pi && ni == other.ni && ti == other.ti;
	}
};

struct IndexKeyHash
{
	std::size_t operator()(const IndexKey& k) const
	{
		return (std::hash<int>()(k.pi) ^ (std::hash<int>()(k.ni) << 1)) ^ (std::hash<int>()(k.ti) << 2);
	}
};

int SDL_Fail()
{
	SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INIT
///////

App::App()
{
}

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

int App::Init()
{
	FindAppDirectory(true);

	if (const int result = InitBackend(); result < 0) return result;
	if (const int result = InitScene(); result < 0) return result;

	imguiCreate();
	return 1;
}

int App::InitBackend()
{
	// 1) SDL window
	{
		ui::Log("Initializing SDL");
		if (!SDL_Init(SDL_INIT_VIDEO)) return -1;

		window = SDL_CreateWindow("Hello bgfx + bullet + SDL3", screenWidth, screenHeight, SDL_WINDOW_RESIZABLE);
		if (!window) return -2;
	}

	// 2) Find native window
	bgfx::PlatformData       pd           = {};
	bgfx::RendererType::Enum rendererType = bgfx::RendererType::Count;
	std::filesystem::path    shaderDir    = "runtime/shaders";

// 	const auto props = SDL_GetWindowProperties(window);

// #if BX_PLATFORM_ANDROID
// #elif BX_PLATFORM_EMSCRIPTEN
// 	ui::Log("__EMSCRIPTEN__");
// 	shaderDir /= "essl";
// 	pd.context = SDL_GL_GetCurrentContext();
// 	pd.nwh     = (void*)"#canvas";
// #elif BX_PLATFORM_IOS
// #elif BX_PLATFORM_LINUX
// 	ui::Log("SDL_PLATFORM_LINUX");
// 	rendererType = bgfx::RendererType::OpenGL;
// 	shaderDir /= "glsl";
// 	if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0)
// 	{
// 		pd.ndt = (Display*)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
// 		pd.nwh = (Window)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
// 	}
// 	else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0)
// 	{
// 		pd.ndt = (struct wl_display*)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
// 		pd.nwh = (struct wl_surface*)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
// 	}
// #elif BX_PLATFORM_OSX
// 	ui::Log("SDL_PLATFORM_MACOS");
// 	rendererType = bgfx::RendererType::Metal;
// 	shaderDir /= "metal";
// 	// pd.nwh = GetMacOSMetalLayer(window);
// #elif BX_PLATFORM_WINDOWS
// 	ui::Log("SDL_PLATFORM_WIN32");
// 	rendererType = bgfx::RendererType::Vulkan;
// 	shaderDir /= "spirv";
// 	pd.nwh = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
// #endif // BX_PLATFORM_*

	// 3) Bgfx initialization
	{
		bgfx::Init init;
		init.platformData      = pd;
		init.resolution.height = screenHeight;
		init.resolution.reset  = BGFX_RESET_VSYNC;
		init.resolution.width  = screenWidth;
		init.type              = rendererType;

		ui::Log("Initializing bgfx");
		if (!bgfx::init(init))
		{
			ui::LogError("bgfx::init failed");
			SDL_DestroyWindow(window);
			SDL_Quit();
			return -4;
		}
		ui::Log("bgfx initialized successfully");
		bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);
	}

	// entry::StartEntry();
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
			screenWidth  = event.window.data1;
			screenHeight = event.window.data2;
			// ResizeWindow(event.window.data1, event.window.data2);
			break;
		}
	}
	Render();
}

void App::Render()
{
	imguiBeginFrame(mousePos[0], mousePos[1], mouseButton, mouseScroll, uint16_t(screenWidth), uint16_t(screenHeight));
	showExampleDialog(nullptr);
	imguiEndFrame();

	bgfx::setViewRect(0, 0, 0, screenWidth, screenHeight);
	bgfx::touch(0);

	// ui::Log("Rendering frame");
	if (useGlm)
	{
		glm::vec3 eye(0.0f, 0.0f, -5.0f);
		glm::vec3 at(0.0f, 0.0f, 0.0f);
		glm::vec3 up(0.0f, 1.0f, 0.0f);
		glm::mat4 view = glm::lookAt(eye, at, up);
		glm::mat4 proj = glm::perspective(glm::radians(60.0f), static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.0f);

		if (!bgfx::getCaps()->originBottomLeft) proj[1][1] *= -1.0f;
		if (bgfx::getCaps()->homogeneousDepth)
		{
			glm::mat4 clipZRemap = glm::mat4(
			    1, 0, 0, 0,
			    0, 1, 0, 0,
			    0, 0, 0.5f, 0.5f,
			    0, 0, 0, 1);
			proj = clipZRemap * proj;
		}
		bgfx::setViewTransform(0, glm::value_ptr(view), glm::value_ptr(proj));
	}
	else
	{
		const bx::Vec3 eye = { 0, 0, -5 };
		const bx::Vec3 at  = { 0, 0, 0 };

		float view[16];
		bx::mtxLookAt(view, eye, at);

		float proj[16];
		bx::mtxProj(proj, 60.0f, float(screenWidth) / screenHeight, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
		bgfx::setViewTransform(0, view, proj);
	}
	bgfx::setViewRect(0, 0, 0, screenWidth, screenHeight);

	const float time      = TO_FLOAT((bx::getHPCounter() - startTime) / TO_DOUBLE(bx::getHPFrequency()));
	const float deltaTime = time - lastTime;

	physicsWorld->dynamicsWorld->stepSimulation(deltaTime, 10, 1.0f / 60.0f);
	lastTime = time;

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
	scene.RenderScene(0, screenWidth, screenHeight);

	//{
	//	float mtx[16];
	//	bx::mtxRotateXY(mtx, 0.0f, deltaTime * 0.37f);

	//	const auto obj = scene.children[0];
	//	obj->Render(0);
	//	//meshSubmit(obj->mesh, 0, obj->program, mtx);
	//}

	const uint32_t frame = bgfx::frame();
	//ui::Log("frame={} time={} deltaTime={}", frame, time, deltaTime);
}

void App::Run()
{
#ifdef __EMSCRIPTEN__
	ui::Log("Starting main loop");
	// clang-format off
	emscripten_set_main_loop_arg([](void* arg) {
		static_cast<App*>(arg)->MainLoop();
	}, this, 0, 1);
	// clang-format on
#else
	while (!quit) MainLoop();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SCENE
////////

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
	//if (auto object = loader.LoadModel("kenney_car-kit-obj/race-future"))
	//{
	//	object->program = shaderManager.LoadProgram("vs_model", "fs_model");
	//	bx::mtxRotateXYZ(object->transform, -0.3f, 2.5f, 0.0f);
	//	scene.AddNamedChild("car", object);
	//}
	//if (auto object = loader.LoadModel("building-n"))
	//{
	//	object->program = shaderManager.LoadProgram("vs_model", "fs_model");
	//	bx::mtxSRT(
	//	    object->transform,
	//	    1.0f, 1.0f, 1.0f, // scale
	//	    0.0f, 1.5f, 0.0f, // rotation
	//	    3.0f, -1.0f, 0.0f // translation
	//	);
	//	scene.AddNamedChild("building", object);
	//}
	//if (auto object = loader.LoadModel("bunny"))
	//{
	//	object->program = shaderManager.LoadProgram("vs_mesh", "fs_mesh");
	//	bx::mtxSRT(
	//	    object->transform,
	//	    1.0f, 1.0f, 1.0f, // scale
	//	    0.0f, 1.5f, 0.0f, // rotation
	//	    -3.0f, 0.0f, 0.0f // translation
	//	);
	//	scene.AddNamedChild("bunny", object);
	//}
	ui::Log("Scene.children={}", scene.children.size());

	physicsWorld = std::make_unique<PhysicsWorld>();
	physicsWorld->ResetCube();

	startTime = bx::getHPCounter();
	lastUs    = NowUs();
	return 1;
}

class ExampleMesh : public entry::AppI
{
private:
	uint32_t            m_debug       = 0;
	uint32_t            m_height      = 480;
	Mesh2*              m_mesh        = nullptr;
	entry::MouseState   m_mouseState  = {};
	bgfx::ProgramHandle m_program     = {};
	uint32_t            m_reset       = 0;
	bgfx::UniformHandle u_time        = {};
	int64_t             m_timeOffset  = 0;
	uint32_t            m_width       = 640;
	sMesh               myMesh        = nullptr;
	ShaderManager       shaderManager = {};

public:
	ExampleMesh(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
	{
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		FindAppDirectory(true);

		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_NONE;
		m_reset  = BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.type     = args.m_type;
		init.vendorId = args.m_pciId;
		init.platformData.nwh  = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
		init.platformData.ndt  = entry::getNativeDisplayHandle();
		init.platformData.type = entry::getNativeWindowHandleType();
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0 , BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

		u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);

		// Create program from shaders.
		m_program = shaderManager.LoadProgram("vs_model", "fs_model");

		// m_mesh = meshLoad("runtime/models/building-n.bin");

		ModelLoader loader;
		m_mesh = loader.LoadModel2("Donut");
		myMesh = loader.LoadModel("building-n");

		m_timeOffset = bx::getHPCounter();

		imguiCreate();
	}

	int shutdown() override
	{
		imguiDestroy();

		if (myMesh) myMesh->Destroy();

		meshUnload(m_mesh);

		// Cleanup.
		// bgfx::destroy(m_program);

		shaderManager.Destroy();

		bgfx::destroy(u_time);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
			imguiBeginFrame(m_mouseState.m_mx
				,  m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				,  m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
				);

			showExampleDialog(this);

			imguiEndFrame();

			// Set view 0 default viewport.
			bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

			float time = (float)( (bx::getHPCounter()-m_timeOffset)/double(bx::getHPFrequency() ) );
			bgfx::setUniform(u_time, &time);

			const bx::Vec3 at  = { 0.0f, 1.0f,  0.0f };
			const bx::Vec3 eye = { 0.0f, 1.0f, -2.5f };

			// Set view and projection matrix for view 0.
			{
				float view[16];
				bx::mtxLookAt(view, eye, at);

				float proj[16];
				bx::mtxProj(proj, 60.0f, float(m_width)/float(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
				bgfx::setViewTransform(0, view, proj);

				// Set view 0 default viewport.
				bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );
			}

			float mtx[16];
			bx::mtxRotateXY(mtx, 0.0f, time*0.37f);

			if (m_mesh)
				meshSubmit(m_mesh, 0, m_program, mtx);

			if (myMesh)
				myMesh->Submit(0, m_program, mtx, BGFX_STATE_MASK);
			else
				ui::Log("!!!myMesh");
			// myMesh->Render(0);

			// Advance to next frame. Rendering thread will be kicked to process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}
};

ENTRY_IMPLEMENT_MAIN(
	  ExampleMesh
	, "04-mesh"
	, "Loading meshes."
	, "https://shark-it.be"
	);
