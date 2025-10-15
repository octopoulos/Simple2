// @version 2025-10-11
/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "stdafx.h"
#include <bx/bx.h>
#include <bx/file.h>
#include <bx/sort.h>
#include <bgfx/bgfx.h>

#include "common/config.h" // DEV_char
#include "ui/xsettings.h"  // xsettings

#include <time.h>

#if BX_PLATFORM_EMSCRIPTEN
#	include <emscripten.h>
#endif // BX_PLATFORM_EMSCRIPTEN

#include "entry_p.h"
#include "input.h"

extern "C" int32_t _main_(int32_t _argc, char** _argv);

namespace entry
{
static uint32_t s_debug = BGFX_DEBUG_NONE;
static uint32_t s_reset = BGFX_RESET_NONE;
static bool     s_exit  = false;

static bx::FileReaderI* s_fileReader = nullptr;
static bx::FileWriterI* s_fileWriter = nullptr;

extern bx::AllocatorI* getDefaultAllocator();
bx::AllocatorI*        g_allocator = getDefaultAllocator();

typedef bx::StringT<&g_allocator> String;

static String      s_currentDir;
static std::string entryName;

class FileReader : public bx::FileReader
{
	typedef bx::FileReader super;

public:
	virtual bool open(const bx::FilePath& _filePath, bx::Error* _err) override
	{
		String filePath(s_currentDir);
		filePath.append(_filePath);
		return super::open(filePath.getPtr(), _err);
	}
};

class FileWriter : public bx::FileWriter
{
	typedef bx::FileWriter super;

public:
	virtual bool open(const bx::FilePath& _filePath, bool _append, bx::Error* _err) override
	{
		String filePath(s_currentDir);
		filePath.append(_filePath);
		return super::open(filePath.getPtr(), _append, _err);
	}
};

void setCurrentDir(const char* _dir)
{
	s_currentDir.set(_dir);
}

#if ENTRY_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR
bx::AllocatorI* getDefaultAllocator()
{
	BX_PRAGMA_DIAGNOSTIC_PUSH();
	BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4459); // warning C4459: declaration of 's_allocator' hides global declaration
	BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wshadow");
	static bx::DefaultAllocator s_allocator;
	return &s_allocator;
	BX_PRAGMA_DIAGNOSTIC_POP();
}
#endif // ENTRY_CONFIG_IMPLEMENT_DEFAULT_ALLOCATOR

/// same order as SDL3
/// @see SDL_scancode.h
static const char* s_keyName[] = {
	"None",
	"None1",
	"None2",
	"None3",

	"KeyA", // 4
	"KeyB",
	"KeyC",
	"KeyD",
	"KeyE",
	"KeyF",
	"KeyG",
	"KeyH",
	"KeyI",
	"KeyJ",
	"KeyK",
	"KeyL",
	"KeyM",
	"KeyN",
	"KeyO",
	"KeyP",
	"KeyQ",
	"KeyR",
	"KeyS",
	"KeyT",
	"KeyU",
	"KeyV",
	"KeyW",
	"KeyX",
	"KeyY",
	"KeyZ",

	"Key1", // 30
	"Key2",
	"Key3",
	"Key4",
	"Key5",
	"Key6",
	"Key7",
	"Key8",
	"Key9",
	"Key0",

	"Return", // 40
	"Esc",
	"Backspace",
	"Tab",
	"Space",

	"Minus", // 45
	"Equals",
	"LeftBracket",
	"RightBracket",
	"Backslash",
	"NonUsHash",
	"Semicolon",
	"Quote",
	"Tilde",
	"Comma",
	"Period",
	"Slash",

	"CapsLock", // 57

	"F1", // 58
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",

	"Print", // 70
	"ScrollLock",
	"Pause",
	"Insert",
	"Home",
	"PageUp",
	"Delete",
	"End",
	"PageDown",
	"Right",
	"Left",
	"Down",
	"Up",

	"NumLockClear", // 83
	"NumPadDivide",
	"NumPadMultiply",
	"NumPadMinus",
	"NumPadPlus",
	"NumPadEnter",
	"NumPad1",
	"NumPad2",
	"NumPad3",
	"NumPad4",
	"NumPad5",
	"NumPad6",
	"NumPad7",
	"NumPad8",
	"NumPad9",
	"NumPad0",
	"NumPadPeriod",

	"NonUsBackslash", // 100
	"Application",
	"Power",
	"NumPadEquals",
	"Mute",
	"VolumeUp",
	"VolumeDown",
	"NumPadComma",

	"Clear", // 108 (SDL3: 156)

	"LeftCtrl", // 109 (SDL3: 224)
	"LeftShift",
	"LeftAlt",
	"LeftMeta",
	"RightCtrl",
	"RightShift",
	"RightAlt",
	"RightMeta",

	"MediaPlay", // 117 (SDL3: 262)
	"MediaPause",
	"MediaRecord",
	"MediaFastFoward",
	"MediaRewind",
	"MediaNext",
	"MediaPrevious",
	"MediaStop",
	"MediaEject",
	"MediaPlayPause",
	"MediaSelect",

	"App1", // 128 (SDL3: 283)
	"App2",

	"GamepadA", // 130 (???)
	"GamepadB",
	"GamepadX",
	"GamepadY",
	"GamepadThumbL",
	"GamepadThumbR",
	"GamepadShoulderL",
	"GamepadShoulderR",
	"GamepadUp",
	"GamepadDown",
	"GamepadLeft",
	"GamepadRight",
	"GamepadBack",
	"GamepadStart",
	"GamepadGuide",
};
static_assert(Key::Count == BX_COUNTOF(s_keyName));

const char* getName(Key::Enum _key)
{
	BX_ASSERT(_key < Key::Count, "Invalid key %d.", _key);
	return s_keyName[_key];
}

bool setOrToggle(uint32_t& _flags, const char* _name, uint32_t _bit, int _first, int _argc, char const* const* _argv)
{
	if (0 == bx::strCmp(_argv[_first], _name))
	{
		int arg = _first + 1;
		if (_argc > arg)
		{
			_flags &= ~_bit;

			bool set = false;
			bx::fromString(&set, _argv[arg]);

			_flags |= set ? _bit : 0;
		}
		else
		{
			_flags ^= _bit;
		}

		return true;
	}

	return false;
}

#if BX_PLATFORM_EMSCRIPTEN
static AppI* s_app;

static void updateApp()
{
	s_app->update();
}
#endif // BX_PLATFORM_EMSCRIPTEN

static AppI*    s_currentApp = nullptr;
static AppI*    s_apps       = nullptr;
static uint32_t s_numApps    = 0;

static char s_restartArgs[1024] = { '\0' };

static AppI* getCurrentApp(AppI* _set = nullptr)
{
	if (_set)
		s_currentApp = _set;
	else if (!s_currentApp)
		s_currentApp = getFirstApp();

	return s_currentApp;
}

static AppI* getNextWrap(AppI* _app)
{
	AppI* next = _app->getNext();
	if (next) return next;
	return getFirstApp();
}

struct AppInternal
{
	AppI*       m_next;
	const char* m_name;
	const char* m_description;
	const char* m_url;
};

static ptrdiff_t s_offset = 0;

AppI::AppI(const char* _name, const char* _description, const char* _url)
{
	static_assert(sizeof(AppInternal) <= sizeof(m_internal));
	s_offset = BX_OFFSETOF(AppI, m_internal);

	AppInternal* ai = (AppInternal*)m_internal;

	ai->m_name        = _name;
	ai->m_description = _description;
	ai->m_url         = _url;
	ai->m_next        = s_apps;

	s_apps = this;
	s_numApps++;
}

AppI::~AppI()
{
	for (AppI *prev = nullptr, *app = s_apps, *next = app->getNext(); app; prev = app, app = next, next = app->getNext())
	{
		if (app == this)
		{
			if (prev)
			{
				AppInternal* ai = bx::addressOf<AppInternal>(prev, s_offset);
				ai->m_next      = next;
			}
			else
			{
				s_apps = next;
			}

			--s_numApps;

			break;
		}
	}
}

const char* AppI::getName() const
{
	AppInternal* ai = (AppInternal*)m_internal;
	return ai->m_name;
}

const char* AppI::getDescription() const
{
	AppInternal* ai = (AppInternal*)m_internal;
	return ai->m_description;
}

const char* AppI::getUrl() const
{
	AppInternal* ai = (AppInternal*)m_internal;
	return ai->m_url;
}

AppI* AppI::getNext()
{
	AppInternal* ai = (AppInternal*)m_internal;
	return ai->m_next;
}

AppI* getFirstApp()
{
	return s_apps;
}

uint32_t getNumApps()
{
	return s_numApps;
}

int runApp(AppI* _app, int _argc, const char* const* _argv)
{
	ui::Log("entry/runApp: %dx%d", xsettings.windowSize[0], xsettings.windowSize[1]);
	setWindowSize(kDefaultWindowHandle, xsettings.windowSize[0], xsettings.windowSize[1]);

	if (_app->init(_argc, _argv, xsettings.windowSize[0], xsettings.windowSize[1]))
	{
		bgfx::frame();

#if BX_PLATFORM_EMSCRIPTEN
		s_app = _app;
		emscripten_set_main_loop(&updateApp, -1, 1);
#else
		while (_app->update())
			if (0 != bx::strLen(s_restartArgs))
				break;
#endif // BX_PLATFORM_EMSCRIPTEN
	}

	return _app->shutdown();
}

void EntryBegin(const char* name)
{
	ui::Log("EntryBegin: %s debug=%d", name, BX_CONFIG_DEBUG);
	entryName = name;

	FindAppDirectory(true);
	InitGameSettings();
	LoadGameSettings();
	LoadGameSettings(xsettings.gameId);
}

void EntryEnd()
{
	ui::Log("EntryEnd");
	SaveGameSettings();
}

void ExitApp()
{
	ui::Log("ExitApp: %d", s_exit);
	s_exit = true;
}

const char* GetEntryName() { return entryName.c_str(); }

void RestartApp(int dir, const char* name)
{
	switch (dir)
	{
	case -1:
	{
		AppI* prev = getCurrentApp();
		for (AppI* app = getNextWrap(prev); app != getCurrentApp(); app = getNextWrap(app))
			prev = app;

		bx::strCopy(s_restartArgs, BX_COUNTOF(s_restartArgs), prev->getName());
		break;
	}
	case 0:
		bx::strCopy(s_restartArgs, BX_COUNTOF(s_restartArgs), getCurrentApp()->getName());
		break;
	case 1:
	{
		AppI* next = getNextWrap(getCurrentApp());
		bx::strCopy(s_restartArgs, BX_COUNTOF(s_restartArgs), next->getName());
		break;
	}
	default:
		for (AppI* app = getFirstApp(); app; app = app->getNext())
		{
			if (0 == bx::strCmp(name, app->getName()))
			{
				bx::strCopy(s_restartArgs, BX_COUNTOF(s_restartArgs), app->getName());
				break;
			}
		}
		break;
	}

	ui::Log("restartArgs=%s", s_restartArgs);
	ExitApp();
}

static int32_t sortApp(const void* _lhs, const void* _rhs)
{
	const AppI* lhs = *(const AppI**)_lhs;
	const AppI* rhs = *(const AppI**)_rhs;

	return bx::strCmpI(lhs->getName(), rhs->getName());
}

static void sortApps()
{
	if (2 > s_numApps)
		return;

	AppI** apps = (AppI**)bx::alloc(g_allocator, s_numApps * sizeof(AppI*));

	uint32_t ii = 0;
	for (AppI* app = getFirstApp(); app; app = app->getNext())
		apps[ii++] = app;
	bx::quickSort(apps, s_numApps, sizeof(AppI*), sortApp);

	s_apps = apps[0];
	for (ii = 1; ii < s_numApps; ++ii)
	{
		AppI* app = apps[ii - 1];

		AppInternal* ai = bx::addressOf<AppInternal>(app, s_offset);
		ai->m_next      = apps[ii];
	}

	{
		AppInternal* ai = bx::addressOf<AppInternal>(apps[s_numApps - 1], s_offset);
		ai->m_next      = nullptr;
	}

	bx::free(g_allocator, apps);
}

int main(int _argc, const char* const* _argv)
{
	ui::Log("entry/main");
	// DBG(BX_COMPILER_NAME " / " BX_CPU_NAME " / " BX_ARCH_NAME " / " BX_PLATFORM_NAME);

	s_fileReader = BX_NEW(g_allocator, FileReader);
	s_fileWriter = BX_NEW(g_allocator, FileWriter);

	inputInit();

	bx::FilePath fp(_argv[0]);
	char         title[bx::kMaxFilePath];
	bx::strCopy(title, BX_COUNTOF(title), fp.getBaseName());

	entry::setWindowTitle(kDefaultWindowHandle, title);
	setWindowSize(kDefaultWindowHandle, xsettings.windowSize[0], xsettings.windowSize[1]);

	sortApps();

	const char* find = "";
	if (1 < _argc)
		find = _argv[_argc - 1];

restart:
	AppI* selected = nullptr;
	s_exit         = false;

	for (AppI* app = getFirstApp(); app; app = app->getNext())
	{
		if (!selected && !bx::strFindI(app->getName(), find).isEmpty())
		{
			selected = app;
			break;
		}
	}

	int32_t result   = bx::kExitSuccess;
	s_restartArgs[0] = '\0';
	if (!s_numApps)
		result = ::_main_(_argc, (char**)_argv);
	else
		result = runApp(getCurrentApp(selected), _argc, _argv);

	if (0 != bx::strLen(s_restartArgs))
	{
		find = s_restartArgs;
		goto restart;
	}

	setCurrentDir("");

	inputShutdown();

	bx::deleteObject(g_allocator, s_fileReader);
	s_fileReader = nullptr;
	bx::deleteObject(g_allocator, s_fileWriter);
	s_fileWriter = nullptr;

	return result;
}

WindowState s_window[ENTRY_CONFIG_MAX_WINDOWS];

bool processEvents(uint32_t& _width, uint32_t& _height, uint32_t& _debug, uint32_t& _reset, MouseState* _mouse)
{
	bool needReset = s_reset != _reset;

	if (s_debug != _debug) bgfx::setDebug(_debug);
	s_debug = _debug;
	s_reset = _reset;

	WindowHandle handle = { UINT16_MAX };

	const bool mouseLock = GetGlobalInput().mouseLock;

	const Event* ev;
	do
	{
		struct SE
		{
			const Event* m_ev;

			SE()
			    : m_ev(poll())
			{
			}

			~SE()
			{
				if (m_ev) release(m_ev);
			}
		} scopeEvent;

		ev = scopeEvent.m_ev;

		if (ev)
		{
			switch (ev->m_type)
			{
			case Event::Axis:
			{
				const AxisEvent* axis = static_cast<const AxisEvent*>(ev);
				inputSetGamepadAxis(axis->m_gamepad, axis->m_axis, axis->m_value);
			}
			break;

			case Event::Char:
			{
				const CharEvent* chev = static_cast<const CharEvent*>(ev);
				GetGlobalInput().PushChar(chev->m_len, chev->m_char);
				if (DEV_char) ui::Log("char=%d %d %d %d %d", chev->m_len, chev->m_char[0], chev->m_char[1], chev->m_char[2], chev->m_char[3]);
			}
			break;

			case Event::Exit: return true;

			case Event::Gamepad:
			{
				//const GamepadEvent* gev = static_cast<const GamepadEvent*>(ev);
				//DBG("gamepad %d, %d", gev->m_gamepad.idx, gev->m_connected);
			}
			break;

			case Event::Mouse:
			{
				const MouseEvent* mouse = static_cast<const MouseEvent*>(ev);
				handle                  = mouse->m_handle;

				GetGlobalInput().MouseMove(mouse->m_mx, mouse->m_my, mouse->m_mz, mouse->hasDelta, mouse->m_dx, mouse->m_dy, mouse->finger);
				if (!mouse->m_move)
					GetGlobalInput().MouseButton(mouse->m_button, mouse->m_down);

				if (_mouse && !mouseLock)
				{
					_mouse->m_mx = mouse->m_mx;
					_mouse->m_my = mouse->m_my;
					_mouse->m_mz = mouse->m_mz;
					if (!mouse->m_move) _mouse->m_buttons[mouse->m_button] = mouse->m_down;
				}
			}
			break;

			case Event::Key:
			{
				const KeyEvent* key = static_cast<const KeyEvent*>(ev);
				handle              = key->m_handle;

				GetGlobalInput().KeyDownUp(TO_INT(key->m_key), key->m_down);
			}
			break;

			case Event::Size:
			{
				const SizeEvent* size   = static_cast<const SizeEvent*>(ev);
				const int        height = (bx::max(32, size->m_height) + 7) & ~15;
				const int        width  = (bx::max(32, size->m_width) + 3) & ~7;

				WindowState& win = s_window[0];
				win.m_handle     = size->m_handle;
				win.m_width      = width;
				win.m_height     = height;

				xsettings.windowSize[0] = width / xsettings.dpr;
				xsettings.windowSize[1] = height / xsettings.dpr;

				handle  = size->m_handle;
				_width  = width;
				_height = height;
				BX_TRACE("Window resize event: %d: %dx%d dpr=%d", handle, _width, _height, xsettings.dpr);
				ui::Log("processEvents/Event::Size: %p: %dx%d", (void*)&handle, _width, _height);
				needReset = true;
			}
			break;

			case Event::Window: break;
			case Event::Suspend: break;

			case Event::DropFile:
			{
				const DropFileEvent* drop = static_cast<const DropFileEvent*>(ev);
				DBG("%s", drop->m_filePath.getCPtr());
			}
			break;

			default: break;
			}
		}
	}
	while (ev);

	needReset |= _reset != s_reset;

	if (handle.idx == 0 && needReset)
	{
		_reset = s_reset;
		BX_TRACE("bgfx::reset(%d, %d, 0x%x)", _width, _height, _reset)
		bgfx::reset(_width, _height, _reset);
		GetGlobalInput().SetResolution(uint16_t(_width), uint16_t(_height), 0);
	}

	_debug = s_debug;
	return s_exit;
}

bool processWindowEvents(WindowState& _state, uint32_t& _debug, uint32_t& _reset)
{
	bool needReset = s_reset != _reset;

	s_debug = _debug;
	s_reset = _reset;

	WindowHandle handle = { UINT16_MAX };

	const bool mouseLock     = GetGlobalInput().mouseLock;
	bool       clearDropFile = true;

	const Event* ev;
	do
	{
		struct SE
		{
			SE(WindowHandle _handle)
			    : m_ev(poll(_handle))
			{
			}

			~SE()
			{
				if (m_ev) release(m_ev);
			}

			const Event* m_ev;

		} scopeEvent(handle);

		ev = scopeEvent.m_ev;

		if (ev)
		{
			handle           = ev->m_handle;
			WindowState& win = s_window[handle.idx];

			switch (ev->m_type)
			{
			case Event::Axis:
			{
				const AxisEvent* axis = static_cast<const AxisEvent*>(ev);
				inputSetGamepadAxis(axis->m_gamepad, axis->m_axis, axis->m_value);
			}
			break;

			case Event::Char:
			{
				const CharEvent* chev = static_cast<const CharEvent*>(ev);
				win.m_handle          = chev->m_handle;
				GetGlobalInput().PushChar(chev->m_len, chev->m_char);
			}
			break;

			case Event::Exit: return true;

			case Event::Gamepad:
			{
				const GamepadEvent* gev = static_cast<const GamepadEvent*>(ev);
				DBG("gamepad %d, %d", gev->m_gamepad.idx, gev->m_connected);
			}
			break;

			case Event::Mouse:
			{
				const MouseEvent* mouse = static_cast<const MouseEvent*>(ev);
				win.m_handle            = mouse->m_handle;

				if (mouse->m_move)
					GetGlobalInput().MouseMove(mouse->m_mx, mouse->m_my, mouse->m_mz, mouse->hasDelta, mouse->m_dx, mouse->m_dy, mouse->finger);
				else
					GetGlobalInput().MouseButton(mouse->m_button, mouse->m_down);

				if (!mouseLock)
				{
					if (mouse->m_move)
					{
						win.m_mouse.m_mx = mouse->m_mx;
						win.m_mouse.m_my = mouse->m_my;
						win.m_mouse.m_mz = mouse->m_mz;
					}
					else
					{
						win.m_mouse.m_buttons[mouse->m_button] = mouse->m_down;
					}
				}
			}
			break;

			case Event::Key:
			{
				const KeyEvent* key = static_cast<const KeyEvent*>(ev);
				win.m_handle        = key->m_handle;

				GetGlobalInput().KeyDownUp(TO_INT(key->m_key), key->m_down);
			}
			break;

			case Event::Size:
			{
				const SizeEvent* size = static_cast<const SizeEvent*>(ev);
				win.m_handle          = size->m_handle;
				win.m_width           = size->m_width;
				win.m_height          = size->m_height;

				needReset = win.m_handle.idx == 0 ? true : needReset;
			}
			break;

			case Event::Window:
			{
				const WindowEvent* window = static_cast<const WindowEvent*>(ev);
				win.m_handle              = window->m_handle;
				win.m_nwh                 = window->m_nwh;
				ev                        = nullptr;
			}
			break;

			case Event::Suspend: break;

			case Event::DropFile:
			{
				const DropFileEvent* drop = static_cast<const DropFileEvent*>(ev);
				win.m_dropFile            = drop->m_filePath;
				clearDropFile             = false;
			}
			break;

			default: break;
			}
		}
	}
	while (ev);

	if (isValid(handle))
	{
		WindowState& win = s_window[handle.idx];
		if (clearDropFile)
			win.m_dropFile.clear();

		_state = win;

		if (handle.idx == 0)
			GetGlobalInput().SetResolution(uint16_t(win.m_width), uint16_t(win.m_height), 0);
	}

	needReset |= _reset != s_reset;

	if (needReset)
	{
		_reset = s_reset;
		BX_TRACE("bgfx::reset(%d, %d, 0x%x)", s_window[0].m_width, s_window[0].m_height, _reset)
		bgfx::reset(s_window[0].m_width, s_window[0].m_height, _reset);
		GetGlobalInput().SetResolution(uint16_t(s_window[0].m_width), uint16_t(s_window[0].m_height), 0);
	}

	_debug = s_debug;
	return s_exit;
}

bx::FileReaderI* getFileReader()
{
	return s_fileReader;
}

bx::FileWriterI* getFileWriter()
{
	return s_fileWriter;
}

bx::AllocatorI* getAllocator()
{
	if (!g_allocator) g_allocator = getDefaultAllocator();
	return g_allocator;
}

void* TinyStlAllocator::static_allocate(size_t _bytes)
{
	return bx::alloc(getAllocator(), _bytes);
}

void TinyStlAllocator::static_deallocate(void* _ptr, size_t /*_bytes*/)
{
	if (_ptr) bx::free(getAllocator(), _ptr);
}

} // namespace entry

extern "C" bool entry_process_events(uint32_t* _width, uint32_t* _height, uint32_t* _debug, uint32_t* _reset)
{
	return entry::processEvents(*_width, *_height, *_debug, *_reset, nullptr);
}

extern "C" void* entry_get_default_native_window_handle()
{
	return entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
}

extern "C" void* entry_get_native_display_handle()
{
	return entry::getNativeDisplayHandle();
}

extern "C" bgfx::NativeWindowHandleType::Enum entry_get_native_window_handle_type()
{
	return entry::getNativeWindowHandleType();
}
