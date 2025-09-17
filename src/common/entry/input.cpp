// @version 2025-09-13
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

	void reset()
	{
		for (uint32_t ii = 0; ii < BX_COUNTOF(m_gamepad); ++ii)
			m_gamepad[ii].reset();

		// !NEW
		GetGlobalInput().Reset();
	}

	Gamepad m_gamepad[ENTRY_CONFIG_MAX_GAMEPADS];
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

void inputSetMouseResolution(uint16_t _width, uint16_t _height)
{
	GetGlobalInput().SetResolution(_width, _height, 0);
}

void inputSetKeyState(entry::Key::Enum _key, uint8_t _modifiers, bool _down)
{
	GetGlobalInput().KeyDownUp(TO_INT(_key), _down);
}

void inputChar(uint8_t _len, const uint8_t _char[4])
{
	GetGlobalInput().PushChar(_len, _char);
}

const uint8_t* inputGetChar()
{
	return GetGlobalInput().PopChar();
}

void inputCharFlush()
{
	GetGlobalInput().FlushChar();
}

void inputSetMousePos(int32_t _mx, int32_t _my, int32_t _mz, bool hasDelta, int32_t _dx, int32_t _dy)
{
	GetGlobalInput().MouseMove(_mx, _my, _mz, hasDelta, _dx, _dy);
}

void inputSetMouseButtonState(entry::MouseButton::Enum _button, uint8_t _state)
{
	GetGlobalInput().MouseButton(_button, _state);
}

bool inputIsMouseLocked()
{
	return GetGlobalInput().mouseLock;
}

void inputSetMouseLock(bool _lock)
{
	if (GetGlobalInput().mouseLock != _lock)
		entry::setMouseLock(entry::kDefaultWindowHandle, _lock);

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

void GlobalInput::FlushChar()
{
	ring.m_current = 0;
	ring.m_read    = 0;
	ring.m_write   = 0;
}

int GlobalInput::IsModifier(int key) const
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

int GlobalInput::KeyToAscii(int key) const
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

const uint8_t* GlobalInput::PopChar()
{
	if (ring.available() > 0)
	{
		const uint8_t* utf8 = &chars[ring.m_read];
		ring.consume(4);
		return utf8;
	}
	return nullptr;
}

void GlobalInput::PushChar(uint8_t _len, const uint8_t _char[4])
{
	for (uint32_t len = ring.reserve(4); len < _len; len = ring.reserve(4))
		PopChar();

	bx::memCopy(&chars[ring.m_current], _char, 4);
	ring.commit(4);
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
	if (width > 0) resolution[0] = TO_FLOAT(width);
	if (height > 0) resolution[1] = TO_FLOAT(height);
	if (wheelDelta > 0) resolution[2] = TO_FLOAT(wheelDelta);
}

GlobalInput& GetGlobalInput()
{
	static GlobalInput globalInput;
	return globalInput;
}
