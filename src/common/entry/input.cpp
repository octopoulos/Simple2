// @version 2025-09-11
/*
 * Copyright 2010-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "stdafx.h"
#include "input.h"
#include "cmd.h"
//
#include "entry_p.h"
#include "ui/xsettings.h"

#include <bx/ringbuffer.h>

struct InputMouse
{
	InputMouse()
	    : m_width(1280)
	    , m_height(720)
	    , m_wheelDelta(120)
	    , m_lock(false)
	{
	}

	void reset()
	{
		if (m_lock)
		{
			m_norm[0] = 0.0f;
			m_norm[1] = 0.0f;
			m_norm[2] = 0.0f;
		}

		bx::memSet(m_buttons, 0, sizeof(m_buttons));
	}

	void setResolution(uint16_t _width, uint16_t _height)
	{
		m_width  = _width;
		m_height = _height;
	}

	void setPos(int32_t _mx, int32_t _my, int32_t _mz)
	{
		m_absolute[0] = _mx;
		m_absolute[1] = _my;
		m_absolute[2] = _mz;
		m_norm[0]     = float(_mx) / float(m_width);
		m_norm[1]     = float(_my) / float(m_height);
		m_norm[2]     = float(_mz) / float(m_wheelDelta);
	}

	void setButtonState(entry::MouseButton::Enum _button, uint8_t _state)
	{
		m_buttons[_button] = _state;
	}

	int32_t  m_absolute[3];
	float    m_norm[3];
	int32_t  m_wheel;
	uint8_t  m_buttons[entry::MouseButton::Count];
	uint16_t m_width;
	uint16_t m_height;
	uint16_t m_wheelDelta;
	bool     m_lock;
};

struct InputKeyboard
{
	InputKeyboard()
	    : m_ring(BX_COUNTOF(m_char) - 4)
	{
	}

	void reset()
	{
		bx::memSet(m_key, 0, sizeof(m_key));
		bx::memSet(m_once, 0xff, sizeof(m_once));
	}

	static uint32_t encodeKeyState(uint8_t _modifiers, bool _down)
	{
		uint32_t state = 0;
		state |= uint32_t(_down ? _modifiers : 0) << 16;
		state |= uint32_t(_down) << 8;
		return state;
	}

	static bool decodeKeyState(uint32_t _state, uint8_t& _modifiers)
	{
		_modifiers = (_state >> 16) & 0xff;
		return 0 != ((_state >> 8) & 0xff);
	}

	void setKeyState(entry::Key::Enum _key, uint8_t _modifiers, bool _down)
	{
		m_key[_key]  = encodeKeyState(_modifiers, _down);
		m_once[_key] = false;
	}

	bool getKeyState(entry::Key::Enum _key, uint8_t* _modifiers)
	{
		uint8_t modifiers;
		_modifiers = !_modifiers ? &modifiers : _modifiers;

		return decodeKeyState(m_key[_key], *_modifiers);
	}

	uint8_t getModifiersState()
	{
		uint8_t modifiers = 0;
		for (uint32_t ii = 0; ii < entry::Key::Count; ++ii)
			modifiers |= (m_key[ii] >> 16) & 0xff;
		return modifiers;
	}

	void pushChar(uint8_t _len, const uint8_t _char[4])
	{
		for (uint32_t len = m_ring.reserve(4); len < _len; len = m_ring.reserve(4))
			popChar();

		bx::memCopy(&m_char[m_ring.m_current], _char, 4);
		m_ring.commit(4);
	}

	const uint8_t* popChar()
	{
		if (0 < m_ring.available())
		{
			uint8_t* utf8 = &m_char[m_ring.m_read];
			m_ring.consume(4);
			return utf8;
		}

		return nullptr;
	}

	void charFlush()
	{
		m_ring.m_current = 0;
		m_ring.m_write   = 0;
		m_ring.m_read    = 0;
	}

	uint32_t m_key[256];
	bool     m_once[256];

	bx::RingBufferControl m_ring;
	uint8_t               m_char[256];
};

struct Gamepad
{
	Gamepad()
	{
		reset();
	}

	void reset()
	{
		bx::memSet(m_axis, 0, sizeof(m_axis));
	}

	void setAxis(entry::GamepadAxis::Enum _axis, int32_t _value)
	{
		m_axis[_axis] = _value;
	}

	int32_t getAxis(entry::GamepadAxis::Enum _axis)
	{
		return m_axis[_axis];
	}

	int32_t m_axis[entry::GamepadAxis::Count];
};

struct Input
{
	Input()
	{
		reset();
	}

	~Input()
	{
	}

	void addBindings(const char* _name, const InputBinding* _bindings)
	{
		m_inputBindingsMap.insert(std::make_pair(std::string(_name), _bindings));
	}

	void removeBindings(const char* _name)
	{
		const auto& it = m_inputBindingsMap.find(std::string(_name));
		if (it != m_inputBindingsMap.end())
			m_inputBindingsMap.erase(it);
	}

	void process(const InputBinding* _bindings)
	{
		for (const InputBinding* binding = _bindings; binding->m_key != entry::Key::None; ++binding)
		{
			uint8_t modifiers;
			bool    down = InputKeyboard::decodeKeyState(m_keyboard.m_key[binding->m_key], modifiers);

			if (binding->m_flags == 1)
			{
				if (down)
				{
					if (modifiers == binding->m_modifiers && !m_keyboard.m_once[binding->m_key])
					{
						if (!binding->m_fn)
							cmdExec((const char*)binding->m_userData);
						else
							binding->m_fn(binding->m_userData);
						m_keyboard.m_once[binding->m_key] = true;
					}
				}
				else
				{
					m_keyboard.m_once[binding->m_key] = false;
				}
			}
			else if (down && modifiers == binding->m_modifiers)
			{
				if (!binding->m_fn)
					cmdExec((const char*)binding->m_userData);
				else
					binding->m_fn(binding->m_userData);
			}
		}
	}

	void process()
	{
		for (const auto& [key, binding] : m_inputBindingsMap)
			process(binding);
	}

	void reset()
	{
		m_mouse.reset();
		m_keyboard.reset();
		for (uint32_t ii = 0; ii < BX_COUNTOF(m_gamepad); ++ii)
			m_gamepad[ii].reset();

		// !NEW
		GetGlobalInput().Reset();
	}

	UMAP_STR<const InputBinding*> m_inputBindingsMap;
	InputKeyboard                 m_keyboard;
	InputMouse                    m_mouse;
	Gamepad                       m_gamepad[ENTRY_CONFIG_MAX_GAMEPADS];
};

static Input* s_input;

void inputInit()
{
	s_input = BX_NEW(entry::getAllocator(), Input);
}

void inputShutdown()
{
	bx::deleteObject(entry::getAllocator(), s_input);
}

void inputAddBindings(const char* _name, const InputBinding* _bindings)
{
	s_input->addBindings(_name, _bindings);
}

void inputRemoveBindings(const char* _name)
{
	s_input->removeBindings(_name);
}

void inputProcess()
{
	s_input->process();
}

void inputSetMouseResolution(uint16_t _width, uint16_t _height)
{
	s_input->m_mouse.setResolution(_width, _height);
	// !NEW
	GetGlobalInput().SetResolution(_width, _height, s_input->m_mouse.m_wheelDelta);
}

void inputSetKeyState(entry::Key::Enum _key, uint8_t _modifiers, bool _down)
{
	s_input->m_keyboard.setKeyState(_key, _modifiers, _down);
	// !NEW
	GetGlobalInput().KeyDownUp(TO_INT(_key), _down);
}

bool inputGetKeyState(entry::Key::Enum _key, uint8_t* _modifiers)
{
	return s_input->m_keyboard.getKeyState(_key, _modifiers);
}

uint8_t inputGetModifiersState()
{
	return s_input->m_keyboard.getModifiersState();
}

void inputChar(uint8_t _len, const uint8_t _char[4])
{
	s_input->m_keyboard.pushChar(_len, _char);
}

const uint8_t* inputGetChar()
{
	return s_input->m_keyboard.popChar();
}

void inputCharFlush()
{
	s_input->m_keyboard.charFlush();
}

void inputSetMousePos(int32_t _mx, int32_t _my, int32_t _mz, bool hasDelta, int32_t _dx, int32_t _dy)
{
	s_input->m_mouse.setPos(_mx, _my, _mz);
	// !NEW
	GetGlobalInput().MouseMove(_mx, _my, _mz, hasDelta, _dx, _dy);
}

void inputSetMouseButtonState(entry::MouseButton::Enum _button, uint8_t _state)
{
	s_input->m_mouse.setButtonState(_button, _state);
	// !NEW
	GetGlobalInput().MouseButton(_button, _state);
}

void inputGetMouse(float _mouse[3])
{
	_mouse[0] = s_input->m_mouse.m_norm[0];
	_mouse[1] = s_input->m_mouse.m_norm[1];
	_mouse[2] = s_input->m_mouse.m_norm[2];

	s_input->m_mouse.m_norm[0] = 0.0f;
	s_input->m_mouse.m_norm[1] = 0.0f;
	s_input->m_mouse.m_norm[2] = 0.0f;
}

bool inputIsMouseLocked()
{
	return s_input->m_mouse.m_lock;
}

void inputSetMouseLock(bool _lock)
{
	if (s_input->m_mouse.m_lock != _lock)
	{
		s_input->m_mouse.m_lock = _lock;

		entry::setMouseLock(entry::kDefaultWindowHandle, _lock);
		if (_lock)
		{
			s_input->m_mouse.m_norm[0] = 0.0f;
			s_input->m_mouse.m_norm[1] = 0.0f;
			s_input->m_mouse.m_norm[2] = 0.0f;
		}
	}

	// !NEW
	GetGlobalInput().MouseLock(_lock);
}

void inputSetGamepadAxis(entry::GamepadHandle _handle, entry::GamepadAxis::Enum _axis, int32_t _value)
{
	s_input->m_gamepad[_handle.idx].setAxis(_axis, _value);
}

int32_t inputGetGamepadAxis(entry::GamepadHandle _handle, entry::GamepadAxis::Enum _axis)
{
	return s_input->m_gamepad[_handle.idx].getAxis(_axis);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GlobalInput
//////////////

/// same order as SDL3
/// @see SDL_scancode.h
// clang-format off
static int ASCII_KEYS[100][2] = {
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },

	{ 'a', 'A' },  // 4
	{ 'b', 'B' },
	{ 'c', 'C' },
	{ 'd', 'D' },
	{ 'e', 'E' },
	{ 'f', 'F' },
	{ 'g', 'G' },
	{ 'h', 'H' },
	{ 'i', 'I' },
	{ 'j', 'J' },
	{ 'k', 'K' },
	{ 'l', 'L' },
	{ 'm', 'M' },
	{ 'n', 'N' },
	{ 'o', 'O' },
	{ 'p', 'P' },
	{ 'q', 'Q' },
	{ 'r', 'R' },
	{ 's', 'S' },
	{ 't', 'T' },
	{ 'u', 'U' },
	{ 'v', 'V' },
	{ 'w', 'W' },
	{ 'x', 'X' },
	{ 'y', 'Y' },
	{ 'z', 'Z' },

	{ '1', '!' }, // 30
	{ '2', '@' },
	{ '3', '#' },
	{ '4', '$' },
	{ '5', '%' },
	{ '6', '^' },
	{ '7', '&' },
	{ '8', '*' },
	{ '9', '(' },
	{ '0', ')' },

	{ '\n', '\n' }, // 40
	{ 27, 27 },
	{ 8, 8 },
	{ '\t', '\t' },
	{ ' ', ' ' },

	{ '-', '_' }, // 45
	{ '=', '+' },
	{ '[', '{' },
	{ ']', '}' },
	{ '\\', '|' },
	{ '#', '#' },
	{ ';', ':' },
	{ '\'', '"' },
	{ '~', '~' },
	{ ',', '<' },
	{ '.', '>' },
	{ '/', '?' },

	{ -1, -1 }, // 57

	{ -1, -1 }, // 58
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },

	{ -1, -1 }, // 70
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },
	{ -1, -1 },

	{ -1, -1 }, // 83
	{ '/', '/' },
	{ '*', '*' },
	{ '-', '-' },
	{ '+', '+' },
	{ '\n', '\n' },
	{ '1', '1' },
	{ '2', '2' },
	{ '3', '3' },
	{ '4', '4' },
	{ '5', '5' },
	{ '6', '6' },
	{ '7', '7' },
	{ '8', '8' },
	{ '9', '9' },
	{ '0', '0' },
	{ '.', '.' },
};
// clang-format on

static UMAP_INT_INT MODIFIER_KEYS = {
	{ entry::Key::LeftAlt,    Modifier_Alt   },
	{ entry::Key::LeftCtrl,   Modifier_Ctrl  },
	{ entry::Key::LeftMeta,   Modifier_Meta  },
	{ entry::Key::LeftShift,  Modifier_Shift },
	{ entry::Key::RightAlt,   Modifier_Alt   },
	{ entry::Key::RightCtrl,  Modifier_Ctrl  },
	{ entry::Key::RightMeta,  Modifier_Meta  },
	{ entry::Key::RightShift, Modifier_Shift },
};

int GlobalInput::IsModifier(int key)
{
	using namespace entry;

	if (key) return FindDefault(MODIFIER_KEYS, key, 0);

	// clang-format off
	int flag = 0;
	if (keys[Key::LeftShift] || keys[Key::RightShift]) flag |= Modifier_Shift;
	if (keys[Key::LeftCtrl ] || keys[Key::RightCtrl ]) flag |= Modifier_Ctrl;
	if (keys[Key::LeftAlt  ] || keys[Key::RightAlt  ]) flag |= Modifier_Alt;
	if (keys[Key::LeftMeta ] || keys[Key::RightMeta ]) flag |= Modifier_Meta;
	// clang-format on
	return flag;
}

void GlobalInput::KeyDownUp(int key, bool down)
{
	if (keys[key] != down)
	{
		const int64_t nowMs = NowMs();

		keys[key]               = down;
		keyTimes[key]           = nowMs;
		keyChanges[keyChangeId] = { key, down, nowMs };
		keyChangeId             = (keyChangeId + 1) & 127;
		if (down)
		{
			keyDowns[key] = true;
			lastAscii     = KeyToAscii(key);
			lastKey       = key;
			//ui::Log("KeyDownUp: {} {} : {} {}", key, down, lastKey, lastAscii);
		}
		else
		{
			keyRepeats[key] = 0;
			keyUps[key]     = true;
			if (!IsModifier(key))
			{
				lastAscii = -1;
				lastKey   = 0;
			}
		}
	}
}

int GlobalInput::KeyToAscii(int key)
{
	if (key < 0 || key >= 100) return -1;

	if (IsModifier(key) & (Modifier_Ctrl | Modifier_Alt | Modifier_Meta)) return -1;

	return ASCII_KEYS[key][(IsModifier() & Modifier_Shift) ? 1 : 0];
}

void GlobalInput::MouseButton(int button, uint8_t state)
{
	if (button >= 0 && button < 8 && buttons[button] != state)
	{
		const int64_t nowMs = NowMs();

		buttons[button]     = state;
		buttonTimes[button] = nowMs;
		if (state)
			buttonDowns[button] = true;
		else
			buttonUps[button] = true;

		// reset initial position when clicking
		mouseRels0[0] = mouseRels[0];
		mouseRels0[1] = mouseRels[1];
		mouseRels0[2] = mouseRels[2];
	}
}

void GlobalInput::MouseDeltas()
{
	mouseRels2[0] = mouseRels[0] - mouseRels0[0];
	mouseRels2[1] = mouseRels[1] - mouseRels0[1];
	mouseRels2[2] = mouseRels[2] - mouseRels0[2];
	mouseRels0[0] = mouseRels[0];
	mouseRels0[1] = mouseRels[1];
	mouseRels0[2] = mouseRels[2];
}

void GlobalInput::MouseLock(bool lock)
{
	if (mouseLock != lock) mouseLock = lock;
}

void GlobalInput::MouseMove(int mx, int my, int mz, bool hasDelta, int dx, int dy)
{
	mouseAbs[0]  = mx;
	mouseAbs[1]  = my;
	mouseAbs[2]  = mz;
	mouseRels[0] = TO_FLOAT(mx) / resolution[0];
	mouseRels[1] = TO_FLOAT(my) / resolution[1];
	mouseRels[2] = TO_FLOAT(mz) / resolution[2];

	// relative motion?
	if (hasDelta)
	{
		mouseRels0[0] = TO_FLOAT(mx - dx) / resolution[0];
		mouseRels0[1] = TO_FLOAT(my - dy) / resolution[1];
	}

	if (!mouseFrame) memcpy(mouseRels0, mouseRels, sizeof(mouseRels));
	++mouseFrame;
}

bool GlobalInput::RepeatingKey(int key, int64_t keyInit, int64_t keyRepeat)
{
	// 1) key is not down?
	if (!keys[key])
	{
		keyRepeats[key] = 0;
		return false;
	}

	// 2) check if time to repeat
	{
		const int64_t firstMs  = keyTimes[key];
		int64_t&      repeatMs = keyRepeats[key];

		if (keyInit < 0) keyInit = xsettings.keyInit;
		if (keyRepeat < 0) keyRepeat = xsettings.keyRepeat;

		// 1st repeat after delay
		if (!repeatMs)
		{
			if (nowMs > firstMs + keyInit)
			{
				repeatMs = nowMs;
				return true;
			}
		}
		// subsequent repeats
		else
		{
			if (nowMs > repeatMs + keyRepeat)
			{
				repeatMs = nowMs;
				return true;
			}
		}
	}

	return false;
}

void GlobalInput::Reset()
{
	// clang-format off
	memset(buttons   , 0, sizeof(buttons   ));
	memset(keyChanges, 0, sizeof(keyChanges));
	memset(keyRepeats, 0, sizeof(keyRepeats));
	memset(keys      , 0, sizeof(keys      ));
	memset(keyTimes  , 0, sizeof(keyTimes  ));
	memset(mouseAbs  , 0, sizeof(mouseAbs  ));
	memset(mouseAbs2 , 0, sizeof(mouseAbs2 ));
	memset(mouseRels , 0, sizeof(mouseRels ));
	memset(mouseRels0, 0, sizeof(mouseRels0));
	memset(mouseRels2, 0, sizeof(mouseRels2));
	// clang-format on

	keyChangeId = 0;
	mouseFrame  = 0;
	mouseLock   = false;

	ResetAscii();
	ResetFixed();
}

void GlobalInput::ResetAscii()
{
	if (lastKey > 0 && RepeatingKey(lastKey))
		lastAscii = KeyToAscii(lastKey);
	else
		lastAscii = -1;

	memset(keyIgnores, 0, sizeof(keyIgnores));
}

void GlobalInput::ResetFixed()
{
	// clang-format off
	memset(buttonDowns, 0, sizeof(buttonDowns));
	memset(buttonUps  , 0, sizeof(buttonUps  ));
	memset(keyDowns   , 0, sizeof(keyDowns   ));
	memset(keyUps     , 0, sizeof(keyUps     ));
	// clang-format on
}

void GlobalInput::SetResolution(int width, int height, int wheelDelta)
{
	resolution[0] = TO_FLOAT(width);
	resolution[1] = TO_FLOAT(height);
	resolution[2] = TO_FLOAT(wheelDelta);
}

GlobalInput& GetGlobalInput()
{
	static GlobalInput globalInput;
	return globalInput;
}
