// entry_sdl3.cpp
// @author octopoulos
// @version 2025-07-30

#include "stdafx.h"
#include "entry_p.h"
#include "ui/xsettings.h"

#if ENTRY_CONFIG_USE_SDL3

#	if BX_PLATFORM_LINUX
#	elif BX_PLATFORM_WINDOWS
#		define SDL_MAIN_HANDLED
#	endif

#	include <SDL3/SDL.h>

#	include <bgfx/platform.h>
#	include <bx/handlealloc.h>
#	include <bx/os.h>
#	include <bx/readerwriter.h>
#	include <bx/thread.h>
#	include <tinystl/allocator.h>
#	include <tinystl/string.h>

namespace entry
{
///
static void* sdlNativeWindowHandle(SDL_Window* window)
{
	if (!window) return nullptr;
	const auto props = SDL_GetWindowProperties(window);

#	if BX_PLATFORM_ANDROID
	return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);

#	elif BX_PLATFORM_EMSCRIPTEN
	return (void*)"#canvas";

#	elif BX_PLATFORM_IOS
	return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);

#	elif BX_PLATFORM_LINUX
	const char* driver = SDL_GetCurrentVideoDriver();
	if (SDL_strcmp(driver, "wayland") == 0)
		return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
	else if (SDL_strcmp(driver, "x11") == 0)
		return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);

#	elif BX_PLATFORM_OSX
	return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);

#	elif BX_PLATFORM_WINDOWS
	return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

#	endif // BX_PLATFORM_*

	return nullptr;
}

static uint8_t translateKeyModifiers(uint16_t sdl)
{
	uint8_t modifiers = 0;
	// clang-format off
	modifiers |= sdl & SDL_KMOD_LALT   ? Modifier::LeftAlt    : 0;
	modifiers |= sdl & SDL_KMOD_RALT   ? Modifier::RightAlt   : 0;
	modifiers |= sdl & SDL_KMOD_LCTRL  ? Modifier::LeftCtrl   : 0;
	modifiers |= sdl & SDL_KMOD_RCTRL  ? Modifier::RightCtrl  : 0;
	modifiers |= sdl & SDL_KMOD_LSHIFT ? Modifier::LeftShift  : 0;
	modifiers |= sdl & SDL_KMOD_RSHIFT ? Modifier::RightShift : 0;
	modifiers |= sdl & SDL_KMOD_LGUI   ? Modifier::LeftMeta   : 0;
	modifiers |= sdl & SDL_KMOD_RGUI   ? Modifier::RightMeta  : 0;
	// clang-format on
	return modifiers;
}

static uint8_t translateKeyModifierPress(uint16_t key)
{
	uint8_t modifier;
	// clang-format off
	switch (key)
	{
	case SDL_SCANCODE_LALT  : modifier = Modifier::LeftAlt;    break;
	case SDL_SCANCODE_RALT  : modifier = Modifier::RightAlt;   break;
	case SDL_SCANCODE_LCTRL : modifier = Modifier::LeftCtrl;   break;
	case SDL_SCANCODE_RCTRL : modifier = Modifier::RightCtrl;  break;
	case SDL_SCANCODE_LSHIFT: modifier = Modifier::LeftShift;  break;
	case SDL_SCANCODE_RSHIFT: modifier = Modifier::RightShift; break;
	case SDL_SCANCODE_LGUI  : modifier = Modifier::LeftMeta;   break;
	case SDL_SCANCODE_RGUI  : modifier = Modifier::RightMeta;  break;
	// clang-format on
	default: modifier = 0; break;
	}

	return modifier;
}

static uint8_t s_translateKey[512];

static Key::Enum translateKey(SDL_Scancode _sdl)
{
	return (Key::Enum)s_translateKey[_sdl & 0x1ff];
}

static uint8_t s_translateGamepad[256];

static void initTranslateGamepad(uint8_t _sdl, Key::Enum _button)
{
	s_translateGamepad[_sdl] = _button;
}

static Key::Enum translateGamepad(uint8_t _sdl)
{
	return Key::Enum(s_translateGamepad[_sdl]);
}

static uint8_t s_translateGamepadAxis[256];

static void initTranslateGamepadAxis(uint8_t _sdl, GamepadAxis::Enum _axis)
{
	s_translateGamepadAxis[_sdl] = uint8_t(_axis);
}

static GamepadAxis::Enum translateGamepadAxis(uint8_t _sdl)
{
	return GamepadAxis::Enum(s_translateGamepadAxis[_sdl]);
}

struct GamepadSDL
{
	GamepadSDL()
	    : m_controller(nullptr)
	    , m_jid(INT32_MAX)
	{
		bx::memSet(m_value, 0, sizeof(m_value));

		// Deadzone values from xinput.h
		m_deadzone[GamepadAxis::LeftX] =
		    m_deadzone[GamepadAxis::LeftY] = 7849;
		m_deadzone[GamepadAxis::RightX] =
		    m_deadzone[GamepadAxis::RightY] = 8689;
		m_deadzone[GamepadAxis::LeftZ] =
		    m_deadzone[GamepadAxis::RightZ] = 30;
	}

	void create(const SDL_JoyDeviceEvent& _jev)
	{
		m_joystick             = SDL_OpenJoystick(_jev.which);
		SDL_Joystick* joystick = m_joystick;
		m_jid                  = SDL_GetJoystickID(joystick);
	}

	void create(const SDL_GamepadDeviceEvent& _cev)
	{
		m_controller           = SDL_OpenGamepad(_cev.which);
		SDL_Joystick* joystick = SDL_GetGamepadJoystick(m_controller);
		m_jid                  = SDL_GetJoystickID(joystick);
	}

	void update(EventQueue& _eventQueue, WindowHandle _handle, GamepadHandle _gamepad, GamepadAxis::Enum _axis, int32_t _value)
	{
		if (filter(_axis, &_value))
			_eventQueue.postAxisEvent(_handle, _gamepad, _axis, _value);
	}

	void destroy()
	{
		if (m_controller)
		{
			SDL_CloseGamepad(m_controller);
			m_controller = nullptr;
		}
		if (m_joystick)
		{
			SDL_CloseJoystick(m_joystick);
			m_joystick = nullptr;
		}

		m_jid = INT32_MAX;
	}

	bool filter(GamepadAxis::Enum _axis, int32_t* _value)
	{
		const int32_t old      = m_value[_axis];
		const int32_t deadzone = m_deadzone[_axis];
		int32_t       value    = *_value;
		value                  = value > deadzone || value < -deadzone ? value : 0;
		m_value[_axis]         = value;
		*_value                = value;
		return old != value;
	}

	int32_t m_value[GamepadAxis::Count];
	int32_t m_deadzone[GamepadAxis::Count];

	SDL_Joystick*  m_joystick;
	SDL_Gamepad*   m_controller;
	// SDL_Haptic*  m_haptic;
	SDL_JoystickID m_jid;
};

struct MainThreadEntry
{
	int    m_argc;
	char** m_argv;

	static int32_t threadFunc(bx::Thread* _thread, void* _userData);
};

struct Msg
{
	Msg()
	    : m_x(0)
	    , m_y(0)
	    , m_width(0)
	    , m_height(0)
	    , m_flags(0)
	    , m_flagsEnabled(false)
	{
	}

	int32_t         m_x;
	int32_t         m_y;
	uint32_t        m_width;
	uint32_t        m_height;
	uint32_t        m_flags;
	tinystl::string m_title;
	bool            m_flagsEnabled;
};

static uint32_t s_userEventStart;

enum SDL_USER_WINDOW
{
	SDL_USER_WINDOW_CREATE,
	SDL_USER_WINDOW_DESTROY,
	SDL_USER_WINDOW_SET_TITLE,
	SDL_USER_WINDOW_SET_FLAGS,
	SDL_USER_WINDOW_SET_POS,
	SDL_USER_WINDOW_SET_SIZE,
	SDL_USER_WINDOW_TOGGLE_FRAME,
	SDL_USER_WINDOW_TOGGLE_FULL_SCREEN,
	SDL_USER_WINDOW_MOUSE_LOCK,
};

static void sdlPostEvent(SDL_USER_WINDOW _type, WindowHandle _handle, Msg* _msg = nullptr, uint32_t _code = 0)
{
	SDL_Event      event;
	SDL_UserEvent& uev = event.user;
	uev.type           = s_userEventStart + _type;

	union
	{
		void*        p;
		WindowHandle h;
	} cast;

	cast.h    = _handle;
	uev.data1 = cast.p;

	uev.data2 = _msg;
	uev.code  = _code;
	SDL_PushEvent(&event);
}

static WindowHandle getWindowHandle(const SDL_UserEvent& _uev)
{
	union
	{
		void*        p;
		WindowHandle h;
	} cast;

	cast.p = _uev.data1;
	return cast.h;
}

struct Context
{
	Context()
	{
		bx::memSet(s_translateKey, 0, sizeof(s_translateKey));

		// https://wiki.libsdl.org/SDL3/SDL_Scancode
		TRANSLATE_KEY(SDL_SCANCODE_A, Key::KeyA);
		TRANSLATE_KEY(SDL_SCANCODE_B, Key::KeyB);
		TRANSLATE_KEY(SDL_SCANCODE_C, Key::KeyC);
		TRANSLATE_KEY(SDL_SCANCODE_D, Key::KeyD);
		TRANSLATE_KEY(SDL_SCANCODE_E, Key::KeyE);
		TRANSLATE_KEY(SDL_SCANCODE_F, Key::KeyF);
		TRANSLATE_KEY(SDL_SCANCODE_G, Key::KeyG);
		TRANSLATE_KEY(SDL_SCANCODE_H, Key::KeyH);
		TRANSLATE_KEY(SDL_SCANCODE_I, Key::KeyI);
		TRANSLATE_KEY(SDL_SCANCODE_J, Key::KeyJ);
		TRANSLATE_KEY(SDL_SCANCODE_K, Key::KeyK);
		TRANSLATE_KEY(SDL_SCANCODE_L, Key::KeyL);
		TRANSLATE_KEY(SDL_SCANCODE_M, Key::KeyM);
		TRANSLATE_KEY(SDL_SCANCODE_N, Key::KeyN);
		TRANSLATE_KEY(SDL_SCANCODE_O, Key::KeyO);
		TRANSLATE_KEY(SDL_SCANCODE_P, Key::KeyP);
		TRANSLATE_KEY(SDL_SCANCODE_Q, Key::KeyQ);
		TRANSLATE_KEY(SDL_SCANCODE_R, Key::KeyR);
		TRANSLATE_KEY(SDL_SCANCODE_S, Key::KeyS);
		TRANSLATE_KEY(SDL_SCANCODE_T, Key::KeyT);
		TRANSLATE_KEY(SDL_SCANCODE_U, Key::KeyU);
		TRANSLATE_KEY(SDL_SCANCODE_V, Key::KeyV);
		TRANSLATE_KEY(SDL_SCANCODE_W, Key::KeyW);
		TRANSLATE_KEY(SDL_SCANCODE_X, Key::KeyX);
		TRANSLATE_KEY(SDL_SCANCODE_Y, Key::KeyY);
		TRANSLATE_KEY(SDL_SCANCODE_Z, Key::KeyZ);

		TRANSLATE_KEY(SDL_SCANCODE_1, Key::Key1);
		TRANSLATE_KEY(SDL_SCANCODE_2, Key::Key2);
		TRANSLATE_KEY(SDL_SCANCODE_3, Key::Key3);
		TRANSLATE_KEY(SDL_SCANCODE_4, Key::Key4);
		TRANSLATE_KEY(SDL_SCANCODE_5, Key::Key5);
		TRANSLATE_KEY(SDL_SCANCODE_6, Key::Key6);
		TRANSLATE_KEY(SDL_SCANCODE_7, Key::Key7);
		TRANSLATE_KEY(SDL_SCANCODE_8, Key::Key8);
		TRANSLATE_KEY(SDL_SCANCODE_9, Key::Key9);
		TRANSLATE_KEY(SDL_SCANCODE_0, Key::Key0);

		TRANSLATE_KEY(SDL_SCANCODE_RETURN, Key::Return);
		TRANSLATE_KEY(SDL_SCANCODE_ESCAPE, Key::Esc);
		TRANSLATE_KEY(SDL_SCANCODE_BACKSPACE, Key::Backspace);
		TRANSLATE_KEY(SDL_SCANCODE_TAB, Key::Tab);
		TRANSLATE_KEY(SDL_SCANCODE_SPACE, Key::Space);

		TRANSLATE_KEY(SDL_SCANCODE_MINUS, Key::Minus);
		TRANSLATE_KEY(SDL_SCANCODE_EQUALS, Key::Equals);
		TRANSLATE_KEY(SDL_SCANCODE_LEFTBRACKET, Key::LeftBracket);
		TRANSLATE_KEY(SDL_SCANCODE_RIGHTBRACKET, Key::RightBracket);
		TRANSLATE_KEY(SDL_SCANCODE_BACKSLASH, Key::Backslash);
		TRANSLATE_KEY(SDL_SCANCODE_NONUSHASH, Key::NonUsHash);
		TRANSLATE_KEY(SDL_SCANCODE_SEMICOLON, Key::Semicolon);
		TRANSLATE_KEY(SDL_SCANCODE_APOSTROPHE, Key::Quote);
		TRANSLATE_KEY(SDL_SCANCODE_GRAVE, Key::Tilde);
		TRANSLATE_KEY(SDL_SCANCODE_COMMA, Key::Comma);
		TRANSLATE_KEY(SDL_SCANCODE_PERIOD, Key::Period);
		TRANSLATE_KEY(SDL_SCANCODE_SLASH, Key::Slash);

		TRANSLATE_KEY(SDL_SCANCODE_CAPSLOCK, Key::CapsLock);

		TRANSLATE_KEY(SDL_SCANCODE_F1, Key::F1);
		TRANSLATE_KEY(SDL_SCANCODE_F2, Key::F2);
		TRANSLATE_KEY(SDL_SCANCODE_F3, Key::F3);
		TRANSLATE_KEY(SDL_SCANCODE_F4, Key::F4);
		TRANSLATE_KEY(SDL_SCANCODE_F5, Key::F5);
		TRANSLATE_KEY(SDL_SCANCODE_F6, Key::F6);
		TRANSLATE_KEY(SDL_SCANCODE_F7, Key::F7);
		TRANSLATE_KEY(SDL_SCANCODE_F8, Key::F8);
		TRANSLATE_KEY(SDL_SCANCODE_F9, Key::F9);
		TRANSLATE_KEY(SDL_SCANCODE_F10, Key::F10);
		TRANSLATE_KEY(SDL_SCANCODE_F11, Key::F11);
		TRANSLATE_KEY(SDL_SCANCODE_F12, Key::F12);

		TRANSLATE_KEY(SDL_SCANCODE_PRINTSCREEN, Key::Print);
		TRANSLATE_KEY(SDL_SCANCODE_SCROLLLOCK, Key::ScrollLock);
		TRANSLATE_KEY(SDL_SCANCODE_PAUSE, Key::Pause);
		TRANSLATE_KEY(SDL_SCANCODE_INSERT, Key::Insert);
		TRANSLATE_KEY(SDL_SCANCODE_HOME, Key::Home);
		TRANSLATE_KEY(SDL_SCANCODE_PAGEUP, Key::PageUp);
		TRANSLATE_KEY(SDL_SCANCODE_DELETE, Key::Delete);
		TRANSLATE_KEY(SDL_SCANCODE_END, Key::End);
		TRANSLATE_KEY(SDL_SCANCODE_PAGEDOWN, Key::PageDown);
		TRANSLATE_KEY(SDL_SCANCODE_RIGHT, Key::Right);
		TRANSLATE_KEY(SDL_SCANCODE_LEFT, Key::Left);
		TRANSLATE_KEY(SDL_SCANCODE_DOWN, Key::Down);
		TRANSLATE_KEY(SDL_SCANCODE_UP, Key::Up);

		TRANSLATE_KEY(SDL_SCANCODE_NUMLOCKCLEAR, Key::NumLockClear);
		TRANSLATE_KEY(SDL_SCANCODE_KP_DIVIDE, Key::NumPadDivide);
		TRANSLATE_KEY(SDL_SCANCODE_KP_MULTIPLY, Key::NumPadMultiply);
		TRANSLATE_KEY(SDL_SCANCODE_KP_MINUS, Key::NumPadMinus);
		TRANSLATE_KEY(SDL_SCANCODE_KP_PLUS, Key::NumPadPlus);
		TRANSLATE_KEY(SDL_SCANCODE_KP_ENTER, Key::NumPadEnter);
		TRANSLATE_KEY(SDL_SCANCODE_KP_1, Key::NumPad1);
		TRANSLATE_KEY(SDL_SCANCODE_KP_2, Key::NumPad2);
		TRANSLATE_KEY(SDL_SCANCODE_KP_3, Key::NumPad3);
		TRANSLATE_KEY(SDL_SCANCODE_KP_4, Key::NumPad4);
		TRANSLATE_KEY(SDL_SCANCODE_KP_5, Key::NumPad5);
		TRANSLATE_KEY(SDL_SCANCODE_KP_6, Key::NumPad6);
		TRANSLATE_KEY(SDL_SCANCODE_KP_7, Key::NumPad7);
		TRANSLATE_KEY(SDL_SCANCODE_KP_8, Key::NumPad8);
		TRANSLATE_KEY(SDL_SCANCODE_KP_9, Key::NumPad9);
		TRANSLATE_KEY(SDL_SCANCODE_KP_0, Key::NumPad0);
		TRANSLATE_KEY(SDL_SCANCODE_KP_PERIOD, Key::NumPadPeriod);

		TRANSLATE_KEY(SDL_SCANCODE_NONUSBACKSLASH, Key::NonUsBackslash);
		TRANSLATE_KEY(SDL_SCANCODE_APPLICATION, Key::Application);
		TRANSLATE_KEY(SDL_SCANCODE_POWER, Key::Power);
		TRANSLATE_KEY(SDL_SCANCODE_KP_EQUALS, Key::NumPadEquals);
		TRANSLATE_KEY(SDL_SCANCODE_MUTE, Key::Mute);
		TRANSLATE_KEY(SDL_SCANCODE_VOLUMEUP, Key::VolumeUp);
		TRANSLATE_KEY(SDL_SCANCODE_VOLUMEDOWN, Key::VolumeDown);
		TRANSLATE_KEY(SDL_SCANCODE_KP_COMMA, Key::NumPadComma);

		TRANSLATE_KEY(SDL_SCANCODE_CLEAR, Key::Clear);

		TRANSLATE_KEY(SDL_SCANCODE_LCTRL, Key::LeftCtrl);
		TRANSLATE_KEY(SDL_SCANCODE_LSHIFT, Key::LeftShift);
		TRANSLATE_KEY(SDL_SCANCODE_LALT, Key::LeftAlt);
		TRANSLATE_KEY(SDL_SCANCODE_LGUI, Key::LeftMeta);
		TRANSLATE_KEY(SDL_SCANCODE_RCTRL, Key::RightCtrl);
		TRANSLATE_KEY(SDL_SCANCODE_RSHIFT, Key::RightShift);
		TRANSLATE_KEY(SDL_SCANCODE_RALT, Key::RightAlt);
		TRANSLATE_KEY(SDL_SCANCODE_RGUI, Key::RightMeta);

		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_PLAY, Key::MediaPlay);
		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_PAUSE, Key::MediaPause);
		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_RECORD, Key::MediaRecord);
		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_FAST_FORWARD, Key::MediaFastFoward);
		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_REWIND, Key::MediaRewind);
		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_NEXT_TRACK, Key::MediaNext);
		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_PREVIOUS_TRACK, Key::MediaPrevious);
		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_STOP, Key::MediaStop);
		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_EJECT, Key::MediaEject);
		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_PLAY_PAUSE, Key::MediaPlayPause);
		TRANSLATE_KEY(SDL_SCANCODE_MEDIA_SELECT, Key::MediaSelect);

		//TRANSLATE_KEY(SDL_SCANCODE_APP1, Key::App1);
		//TRANSLATE_KEY(SDL_SCANCODE_APP2, Key::App2);

		bx::memSet(s_translateGamepad, uint8_t(Key::Count), sizeof(s_translateGamepad));
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_SOUTH, Key::GamepadA);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_EAST, Key::GamepadB);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_WEST, Key::GamepadX);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_NORTH, Key::GamepadY);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_LEFT_STICK, Key::GamepadThumbL);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_RIGHT_STICK, Key::GamepadThumbR);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, Key::GamepadShoulderL);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, Key::GamepadShoulderR);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_DPAD_UP, Key::GamepadUp);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_DPAD_DOWN, Key::GamepadDown);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_DPAD_LEFT, Key::GamepadLeft);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Key::GamepadRight);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_BACK, Key::GamepadBack);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_START, Key::GamepadStart);
		initTranslateGamepad(SDL_GAMEPAD_BUTTON_GUIDE, Key::GamepadGuide);

		bx::memSet(s_translateGamepadAxis, uint8_t(GamepadAxis::Count), sizeof(s_translateGamepadAxis));
		initTranslateGamepadAxis(SDL_GAMEPAD_AXIS_LEFTX, GamepadAxis::LeftX);
		initTranslateGamepadAxis(SDL_GAMEPAD_AXIS_LEFTY, GamepadAxis::LeftY);
		initTranslateGamepadAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, GamepadAxis::LeftZ);
		initTranslateGamepadAxis(SDL_GAMEPAD_AXIS_RIGHTX, GamepadAxis::RightX);
		initTranslateGamepadAxis(SDL_GAMEPAD_AXIS_RIGHTY, GamepadAxis::RightY);
		initTranslateGamepadAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, GamepadAxis::RightZ);
	}

	int run(int _argc, char** _argv)
	{
		EntryBegin("SDL3");
		ui::Log("SDL3/run: {}x{}", xsettings.windowSize[0], xsettings.windowSize[1]);

		m_mte.m_argc = _argc;
		m_mte.m_argv = _argv;

		SDL_Init(SDL_INIT_GAMEPAD);

		m_windowAlloc.alloc();
		m_window[0] = SDL_CreateWindow("Loading ...", xsettings.windowSize[0], xsettings.windowSize[1], SDL_WINDOW_RESIZABLE);

		m_flags[0] = 0
		    | ENTRY_WINDOW_FLAG_ASPECT_RATIO
		    | ENTRY_WINDOW_FLAG_FRAME;

		s_userEventStart = SDL_RegisterEvents(7);

		bgfx::renderFrame();

		m_thread.init(MainThreadEntry::threadFunc, &m_mte);

		// Force window resolution...
		WindowHandle defaultWindow = { 0 };
		entry::setWindowSize(defaultWindow, xsettings.windowSize[0], xsettings.windowSize[1]);

		SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, true);

		bx::FileReaderI* reader = nullptr;
		while (!reader)
		{
			reader = getFileReader();
			bx::sleep(100);
		}

		if (bx::open(reader, "gamecontrollerdb.txt"))
		{
			bx::AllocatorI* allocator = getAllocator();
			uint32_t        size      = (uint32_t)bx::getSize(reader);
			void*           data      = bx::alloc(allocator, size + 1);
			bx::read(reader, data, size, bx::ErrorAssert {});
			bx::close(reader);
			((char*)data)[size] = '\0';

			if (SDL_AddGamepadMapping((char*)data) < 0)
				DBG("SDL game controller add mapping failed: %s", SDL_GetError());

			bx::free(allocator, data);
		}

		bool      exit = false;
		SDL_Event event;
		while (!exit)
		{
			bgfx::renderFrame();

			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				case SDL_EVENT_QUIT:
					m_eventQueue.postExitEvent();
					exit = true;
					break;

				case SDL_EVENT_MOUSE_MOTION:
				{
					const SDL_MouseMotionEvent& mev = event.motion;
					m_mx                            = mev.x;
					m_my                            = mev.y;

					WindowHandle handle = findHandle(mev.windowID);
					if (isValid(handle))
						m_eventQueue.postMouseEvent(handle, m_mx, m_my, m_mz, true, mev.xrel * 2, mev.yrel * 2);
				}
				break;

				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				case SDL_EVENT_MOUSE_BUTTON_UP:
				{
					const SDL_MouseButtonEvent& mev    = event.button;
					WindowHandle                handle = findHandle(mev.windowID);
					if (isValid(handle))
					{
						MouseButton::Enum button;
						switch (mev.button)
						{
						default:
						case SDL_BUTTON_LEFT: button = MouseButton::Left; break;
						case SDL_BUTTON_MIDDLE: button = MouseButton::Middle; break;
						case SDL_BUTTON_RIGHT: button = MouseButton::Right; break;
						}

						m_eventQueue.postMouseEvent(handle, mev.x, mev.y, m_mz, button, mev.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
					}
				}
				break;

				case SDL_EVENT_MOUSE_WHEEL:
				{
					const SDL_MouseWheelEvent& mev = event.wheel;
					m_mz += mev.y;

					WindowHandle handle = findHandle(mev.windowID);
					if (isValid(handle))
						m_eventQueue.postMouseEvent(handle, m_mx, m_my, m_mz, false, 0, 0);
				}
				break;

				case SDL_EVENT_TEXT_INPUT:
				{
					const SDL_TextInputEvent& tev    = event.text;
					WindowHandle              handle = findHandle(tev.windowID);
					if (isValid(handle))
						m_eventQueue.postCharEvent(handle, 1, (const uint8_t*)tev.text);
				}
				break;

				case SDL_EVENT_KEY_DOWN:
				{
					const SDL_KeyboardEvent& kev    = event.key;
					WindowHandle             handle = findHandle(kev.windowID);
					if (isValid(handle))
					{
						uint8_t   modifiers = translateKeyModifiers(kev.mod);
						Key::Enum key       = translateKey(kev.scancode);
						if (!key) ui::Log("SDL3: Unknown code: {} {}", TO_INT(kev.scancode), SDL_GetScancodeName(kev.scancode));

#	if 0
						DBG("SDL scancode %d, key %d, name %s, key name %s"
							, kev.scancode
							, key
							, SDL_GetScancodeName(kev.scancode)
							, SDL_GetKeyName(kev.scancode)
							);
#	endif // 0

						/// If you only press (e.g.) 'shift' and nothing else, then key == 'shift', modifier == 0.
						/// Further along, pressing 'shift' + 'ctrl' would be: key == 'shift', modifier == 'ctrl.
						if (0 == key && 0 == modifiers)
							modifiers = translateKeyModifierPress(kev.scancode);

						if (Key::Esc == key)
						{
							uint8_t pressedChar[4];
							pressedChar[0] = 0x1b;
							m_eventQueue.postCharEvent(handle, 1, pressedChar);
						}
						else if (Key::Return == key)
						{
							uint8_t pressedChar[4];
							pressedChar[0] = 0x0d;
							m_eventQueue.postCharEvent(handle, 1, pressedChar);
						}
						else if (Key::Backspace == key)
						{
							uint8_t pressedChar[4];
							pressedChar[0] = 0x08;
							m_eventQueue.postCharEvent(handle, 1, pressedChar);
						}

						m_eventQueue.postKeyEvent(handle, key, modifiers, kev.down);
					}
				}
				break;

				case SDL_EVENT_KEY_UP:
				{
					const SDL_KeyboardEvent& kev    = event.key;
					WindowHandle             handle = findHandle(kev.windowID);
					if (isValid(handle))
					{
						uint8_t   modifiers = translateKeyModifiers(kev.mod);
						Key::Enum key       = translateKey(kev.scancode);
						m_eventQueue.postKeyEvent(handle, key, modifiers, kev.down);
					}
				}
				break;

				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				case SDL_EVENT_WINDOW_METAL_VIEW_RESIZED:
				{
					const auto&  wev    = event.window;
					WindowHandle handle = findHandle(wev.windowID);
					uint32_t     width  = wev.data1;
					uint32_t     height = wev.data2;
					if (width != xsettings.windowSize[0] || height != xsettings.windowSize[1])
						m_eventQueue.postSizeEvent(handle, width, height);
				}
				break;

				case SDL_EVENT_WINDOW_SHOWN:
				case SDL_EVENT_WINDOW_HIDDEN:
				case SDL_EVENT_WINDOW_EXPOSED:
				case SDL_EVENT_WINDOW_MOVED:
				case SDL_EVENT_WINDOW_MINIMIZED:
				case SDL_EVENT_WINDOW_MAXIMIZED:
				case SDL_EVENT_WINDOW_RESTORED:
				case SDL_EVENT_WINDOW_MOUSE_ENTER:
				case SDL_EVENT_WINDOW_MOUSE_LEAVE:
				case SDL_EVENT_WINDOW_FOCUS_GAINED:
				case SDL_EVENT_WINDOW_FOCUS_LOST:
					break;

				case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				{
					const auto&  wev    = event.window;
					WindowHandle handle = findHandle(wev.windowID);
					if (0 == handle.idx)
					{
						m_eventQueue.postExitEvent();
						exit = true;
					}
				}
				break;

				// Ignore Joystick events. Example's Gamepad concept mirrors SDL Game Controller.
				// Game Controllers are higher level wrapper around Joystick and both events come through.
				// Respond to only the controller events. Controller events are properly remapped.
				case SDL_EVENT_JOYSTICK_AXIS_MOTION:
				case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
				case SDL_EVENT_JOYSTICK_BUTTON_UP:
				case SDL_EVENT_JOYSTICK_ADDED:
				case SDL_EVENT_JOYSTICK_REMOVED:
					break;

				case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				{
					const SDL_GamepadAxisEvent& aev    = event.gaxis;
					GamepadHandle               handle = findGamepad(aev.which);
					if (isValid(handle))
					{
						GamepadAxis::Enum axis = translateGamepadAxis(aev.axis);
						m_gamepad[handle.idx].update(m_eventQueue, defaultWindow, handle, axis, aev.value);
					}
				}
				break;

				case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				case SDL_EVENT_GAMEPAD_BUTTON_UP:
				{
					const SDL_GamepadButtonEvent& bev    = event.gbutton;
					GamepadHandle                 handle = findGamepad(bev.which);
					if (isValid(handle))
					{
						Key::Enum key = translateGamepad(bev.button);
						if (Key::Count != key)
							m_eventQueue.postKeyEvent(defaultWindow, key, 0, event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
					}
				}
				break;

				case SDL_EVENT_GAMEPAD_ADDED:
				{
					GamepadHandle handle = { m_gamepadAlloc.alloc() };
					if (isValid(handle))
					{
						const SDL_GamepadDeviceEvent& cev = event.gdevice;
						m_gamepad[handle.idx].create(cev);
						m_eventQueue.postGamepadEvent(defaultWindow, handle, true);
					}
				}
				break;

				case SDL_EVENT_GAMEPAD_REMAPPED:
				{
				}
				break;

				case SDL_EVENT_GAMEPAD_REMOVED:
				{
					const SDL_GamepadDeviceEvent& cev    = event.gdevice;
					GamepadHandle                 handle = findGamepad(cev.which);
					if (isValid(handle))
					{
						m_gamepad[handle.idx].destroy();
						m_gamepadAlloc.free(handle.idx);
						m_eventQueue.postGamepadEvent(defaultWindow, handle, false);
					}
				}
				break;

				case SDL_EVENT_DROP_FILE:
				{
					const SDL_DropEvent& dev    = event.drop;
					WindowHandle         handle = defaultWindow; // findHandle(dev.windowID);
					if (isValid(handle))
					{
						m_eventQueue.postDropFileEvent(handle, dev.data);
						SDL_free((void*)dev.data);
					}
				}
				break;

				default:
				{
					const SDL_UserEvent& uev = event.user;
					switch (uev.type - s_userEventStart)
					{
					case SDL_USER_WINDOW_CREATE:
					{
						WindowHandle handle = getWindowHandle(uev);
						Msg*         msg    = (Msg*)uev.data2;

						m_window[handle.idx] = SDL_CreateWindow(msg->m_title.c_str(), msg->m_width, msg->m_height, SDL_WINDOW_RESIZABLE);

						m_flags[handle.idx] = msg->m_flags;

						void* nwh = sdlNativeWindowHandle(m_window[handle.idx]);
						if (nwh)
						{
							m_eventQueue.postSizeEvent(handle, msg->m_width, msg->m_height);
							m_eventQueue.postWindowEvent(handle, nwh);
						}

						delete msg;
					}
					break;

					case SDL_USER_WINDOW_DESTROY:
					{
						WindowHandle handle = getWindowHandle(uev);
						if (isValid(handle))
						{
							m_eventQueue.postWindowEvent(handle);
							SDL_DestroyWindow(m_window[handle.idx]);
							m_window[handle.idx] = nullptr;
						}
					}
					break;

					case SDL_USER_WINDOW_SET_TITLE:
					{
						WindowHandle handle = getWindowHandle(uev);
						Msg*         msg    = (Msg*)uev.data2;
						if (isValid(handle))
							SDL_SetWindowTitle(m_window[handle.idx], msg->m_title.c_str());
						delete msg;
					}
					break;

					case SDL_USER_WINDOW_SET_FLAGS:
					{
						WindowHandle handle = getWindowHandle(uev);
						Msg*         msg    = (Msg*)uev.data2;

						if (msg->m_flagsEnabled)
							m_flags[handle.idx] |= msg->m_flags;
						else
							m_flags[handle.idx] &= ~msg->m_flags;

						delete msg;
					}
					break;

					case SDL_USER_WINDOW_SET_POS:
					{
						WindowHandle handle = getWindowHandle(uev);
						Msg*         msg    = (Msg*)uev.data2;
						SDL_SetWindowPosition(m_window[handle.idx], msg->m_x, msg->m_y);
						delete msg;
					}
					break;

					case SDL_USER_WINDOW_SET_SIZE:
					{
						WindowHandle handle = getWindowHandle(uev);
						Msg*         msg    = (Msg*)uev.data2;
						if (isValid(handle))
							SDL_SetWindowSize(m_window[handle.idx], msg->m_width, msg->m_height);
						delete msg;
					}
					break;

					case SDL_USER_WINDOW_TOGGLE_FRAME:
					{
						WindowHandle handle = getWindowHandle(uev);
						if (isValid(handle))
						{
							m_flags[handle.idx] ^= ENTRY_WINDOW_FLAG_FRAME;
							SDL_SetWindowBordered(m_window[handle.idx], !!(m_flags[handle.idx] & ENTRY_WINDOW_FLAG_FRAME));
						}
					}
					break;

					case SDL_USER_WINDOW_TOGGLE_FULL_SCREEN:
					{
						WindowHandle handle = getWindowHandle(uev);
						m_fullscreen        = !m_fullscreen;
						SDL_SetWindowFullscreen(m_window[handle.idx], m_fullscreen);
					}
					break;

					case SDL_USER_WINDOW_MOUSE_LOCK:
					{
						WindowHandle handle = getWindowHandle(uev);
						if (isValid(handle))
						{
							m_eventQueue.postWindowEvent(handle);
							SDL_SetWindowRelativeMouseMode(m_window[handle.idx], !!uev.code);
						}
					}
					break;

					default: break;
					}
				}
				break;
				}
			}
		}

		while (bgfx::RenderFrame::NoContext != bgfx::renderFrame()) {};
		m_thread.shutdown();

		SDL_DestroyWindow(m_window[0]);
		SDL_Quit();

		EntryEnd();
		return m_thread.getExitCode();
	}

	WindowHandle findHandle(uint32_t _windowId)
	{
		SDL_Window* window = SDL_GetWindowFromID(_windowId);
		return findHandle(window);
	}

	WindowHandle findHandle(SDL_Window* _window)
	{
		bx::MutexScope scope(m_lock);
		for (uint32_t ii = 0, num = m_windowAlloc.getNumHandles(); ii < num; ++ii)
		{
			uint16_t idx = m_windowAlloc.getHandleAt(ii);
			if (_window == m_window[idx])
			{
				WindowHandle handle = { idx };
				return handle;
			}
		}

		WindowHandle invalid = { UINT16_MAX };
		return invalid;
	}

	GamepadHandle findGamepad(SDL_JoystickID _jid)
	{
		for (uint32_t ii = 0, num = m_gamepadAlloc.getNumHandles(); ii < num; ++ii)
		{
			uint16_t idx = m_gamepadAlloc.getHandleAt(ii);
			if (_jid == m_gamepad[idx].m_jid)
			{
				GamepadHandle handle = { idx };
				return handle;
			}
		}

		GamepadHandle invalid = { UINT16_MAX };
		return invalid;
	}

	MainThreadEntry m_mte;
	bx::Thread      m_thread;

	EventQueue m_eventQueue;
	bx::Mutex  m_lock;

	bx::HandleAllocT<ENTRY_CONFIG_MAX_WINDOWS> m_windowAlloc;
	SDL_Window*                                m_window[ENTRY_CONFIG_MAX_WINDOWS];
	uint32_t                                   m_flags[ENTRY_CONFIG_MAX_WINDOWS];

	bx::HandleAllocT<ENTRY_CONFIG_MAX_GAMEPADS> m_gamepadAlloc;
	GamepadSDL                                  m_gamepad[ENTRY_CONFIG_MAX_GAMEPADS];

	bool    m_fullscreen = false;
	bool    m_mouseLock  = false;
	int32_t m_mx         = 0;
	int32_t m_my         = 0;
	int32_t m_mz         = 0;
};

static Context s_ctx;

const Event* poll()
{
	return s_ctx.m_eventQueue.poll();
}

const Event* poll(WindowHandle _handle)
{
	return s_ctx.m_eventQueue.poll(_handle);
}

void release(const Event* _event)
{
	s_ctx.m_eventQueue.release(_event);
}

WindowHandle createWindow(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height, uint32_t _flags, const char* _title)
{
	bx::MutexScope scope(s_ctx.m_lock);
	WindowHandle   handle = { s_ctx.m_windowAlloc.alloc() };

	if (UINT16_MAX != handle.idx)
	{
		Msg* msg      = new Msg;
		msg->m_x      = _x;
		msg->m_y      = _y;
		msg->m_width  = _width;
		msg->m_height = _height;
		msg->m_title  = _title;
		msg->m_flags  = _flags;

		sdlPostEvent(SDL_USER_WINDOW_CREATE, handle, msg);
	}

	return handle;
}

void destroyWindow(WindowHandle _handle)
{
	if (UINT16_MAX != _handle.idx)
	{
		sdlPostEvent(SDL_USER_WINDOW_DESTROY, _handle);

		bx::MutexScope scope(s_ctx.m_lock);
		s_ctx.m_windowAlloc.free(_handle.idx);
	}
}

void setWindowPos(WindowHandle _handle, int32_t _x, int32_t _y)
{
	Msg* msg = new Msg;
	msg->m_x = _x;
	msg->m_y = _y;

	sdlPostEvent(SDL_USER_WINDOW_SET_POS, _handle, msg);
}

void setWindowSize(WindowHandle _handle, uint32_t _width, uint32_t _height)
{
	// Function to set the window size programmatically from the examples/tools.
	Msg* msg      = new Msg;
	msg->m_width  = _width;
	msg->m_height = _height;

	sdlPostEvent(SDL_USER_WINDOW_SET_SIZE, _handle, msg);
}

void setWindowTitle(WindowHandle _handle, const char* _title)
{
	Msg* msg     = new Msg;
	msg->m_title = _title;

	sdlPostEvent(SDL_USER_WINDOW_SET_TITLE, _handle, msg);
}

void setWindowFlags(WindowHandle _handle, uint32_t _flags, bool _enabled)
{
	Msg* msg            = new Msg;
	msg->m_flags        = _flags;
	msg->m_flagsEnabled = _enabled;
	sdlPostEvent(SDL_USER_WINDOW_SET_FLAGS, _handle, msg);
}

void toggleFullscreen(WindowHandle _handle)
{
	sdlPostEvent(SDL_USER_WINDOW_TOGGLE_FULL_SCREEN, _handle);
}

void setMouseLock(WindowHandle _handle, bool _lock)
{
	sdlPostEvent(SDL_USER_WINDOW_MOUSE_LOCK, _handle, nullptr, _lock);
}

void* getNativeWindowHandle(WindowHandle _handle)
{
	return sdlNativeWindowHandle(s_ctx.m_window[_handle.idx]);
}

void* getNativeDisplayHandle()
{
	SDL_Window* window = s_ctx.m_window[0];
	if (!window) return nullptr;

#	if BX_PLATFORM_LINUX
	const char* driver = SDL_GetCurrentVideoDriver();

	if (SDL_strcmp(driver, "wayland") == 0)
		return SDL_GetWindowProperty(window, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
	else if (SDL_strcmp(driver, "x11") == 0)
		return SDL_GetWindowProperty(window, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
#	endif

	return nullptr;
}

bgfx::NativeWindowHandleType::Enum getNativeWindowHandleType()
{
#	if BX_PLATFORM_LINUX
	const char* driver = SDL_GetCurrentVideoDriver();
	if (SDL_strcmp(driver, "wayland") == 0)
		return bgfx::NativeWindowHandleType::Wayland;
#	endif // BX_PLATFORM_*

	return bgfx::NativeWindowHandleType::Default;
}

int32_t MainThreadEntry::threadFunc(bx::Thread* _thread, void* _userData)
{
	BX_UNUSED(_thread);

	MainThreadEntry* self   = (MainThreadEntry*)_userData;
	int32_t          result = main(self->m_argc, self->m_argv);

	SDL_Event      event;
	SDL_QuitEvent& qev = event.quit;
	qev.type           = SDL_EVENT_QUIT;
	SDL_PushEvent(&event);
	return result;
}

} // namespace entry

int main(int _argc, char** _argv)
{
	return entry::s_ctx.run(_argc, _argv);
}

#endif // ENTRY_CONFIG_USE_SDL3
