// @version 2025-10-24
/*
 * Copyright 2010-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "stdafx.h"
#include "entry/input.h"
//
#include "common/config.h" // DEV_inputKeys
#include "entry_p.h"       // entry::
#include "ui/ui.h"         // ui::
#include "ui/xsettings.h"  // xsettings

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

void inputSetGamepadAxis(entry::GamepadHandle _handle, entry::GamepadAxis::Enum _axis, int32_t _value)
{
	s_input->m_gamepad[_handle.idx].setAxis(_axis, _value);
}

int32_t inputGetGamepadAxis(entry::GamepadHandle _handle, entry::GamepadAxis::Enum _axis)
{
	return s_input->m_gamepad[_handle.idx].getAxis(_axis);
}

bool IsHoveringUi()
{
	const auto& io = ImGui::GetIO();
	return io.WantCaptureMouse;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FINGER
/////////

void Finger::ComputeDeltas()
{
	rels2[0] = rels[0] - rels0[0];
	rels2[1] = rels[1] - rels0[1];
	rels2[2] = rels[2] - rels0[2];
	memcpy(rels0, rels, sizeof(rels));
}

void Finger::Update(int mx, int my, int mz, bool hasDelta, int dx, int dy, float pressure_, const float* resolution)
{
	abs[0]   = mx;
	abs[1]   = my;
	abs[2]   = mz;
	pressure = pressure_;
	rels[0]  = TO_FLOAT(mx) / resolution[0];
	rels[1]  = TO_FLOAT(my) / resolution[1];
	rels[2]  = TO_FLOAT(mz) / resolution[2];

	// relative motion?
	if (hasDelta)
	{
		rels0[0] = TO_FLOAT(mx - dx) / resolution[0];
		rels0[1] = TO_FLOAT(my - dy) / resolution[1];
	}

	if (!frame) memcpy(rels0, rels, sizeof(rels));
	++frame;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVICE
/////////

void Device::CalculateMotion()
{
	gesture   = Gesture_None;
	motion[0] = 0.0f;
	motion[1] = 0.0f;
	motion[2] = 0.0f;

	int count = 0;

	if (fingers.size())
	{
		float ax0 = 0.0f;
		float ay0 = 0.0f;
		float x0  = 0.0f;
		float y0  = 0.0f;

		for (int id = -1; const auto& [fid, finger] : fingers)
		{
			++id;
			const float x1 = finger.rels2[0];
			const float y1 = finger.rels2[1];
			motion[0] += x1;
			motion[1] += y1;

			// ignore fingers not moving
			if (x1 == 0.0f && y1 == 0.0f) continue;
			++count;

			// 1st finger
			if (count == 1)
			{
				ax0 = finger.rels0[0];
				ay0 = finger.rels0[1];
				x0  = x1;
				y0  = y1;
			}
			// other fingers are compared to the 1st finger
			else
			{
				const float dotX = x0 * x1;
				const float dotY = y0 * y1;
				if (dotX >=0 || dotY >= 0)
				{
					if ((dotX >= 0 && dotY >= 0) || (dotX >= 0 && dotX > bx::abs(dotY) * 5.0f) || (dotY >= 0 && dotY > bx::abs(dotX) * 5.0f))
						gesture = Gesture_Parallel;
				}
				else if (dotX <= 0 && dotY <= 0)
				{
					gesture = Gesture_Zoom;
					const float ax1 = finger.rels0[0];
					const float ay1 = finger.rels0[1];

					// distance before/after
					const float dist0 = bx::distanceSq(bx::Vec3(ax0     , ay0     , 0.0f), bx::Vec3(ax1     , ay1     , 0.0f));
					const float dist1 = bx::distanceSq(bx::Vec3(ax0 + x0, ay0 + y0, 0.0f), bx::Vec3(ax1 + x1, ay1 + y1, 0.0f));

					if (dist1 > dist0)
						gesture |= Gesture_ZoomIn;
					else if (dist1 < dist0)
						gesture |= Gesture_ZoomOut;
				}
			}
		}

		// normalize motion
		if (count > 1)
		{
			motion[0] /= count;
			motion[1] /= count;
		}
	}
}

void Device::RemoveFinger(uint64_t fingerId)
{
	if (auto it = fingers.find(fingerId); it != fingers.end())
		fingers.erase(it);
	else
		ui::Log("RemoveFingers: Cannot find %lld", fingerId);
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

void GlobalInput::BeginFrame()
{
	ResetFixed();

	// 1) ascii
	if (lastKey > 0 && RepeatingKey(lastKey))
		lastAscii = KeyToAscii(lastKey);
	else
		lastAscii = -1;

	// 2) ignores
	memset(keyIgnores, 0, sizeof(keyIgnores));

	// 3) check mouse clicks
	const int64_t nowMs = NowMs();
	for (int i = 0; i < 8; ++i)
	{
		const int64_t click = buttonClicks[i];
		if (click && nowMs > click + xsettings.clickTwo)
		{
			if (DEV_inputMouse) ui::Log("BeginFrame: CLICK: %lld", nowMs - click);
			buttonClicks[i] = 0;
			buttonOnes[i]   = true;
		}
	}
}

void GlobalInput::ComputeDeltas()
{
	for (auto& [did, device] : devices)
	{
		for (auto& [fid, finger] : device.fingers)
			finger.ComputeDeltas();
	}
}

std::pair<int, int> GlobalInput::DecodeKey(int keyMod)
{
	return { keyMod & 4095, keyMod >> 12 };
}

TEST_CASE("DecodeKey")
{
	// clang-format off
	const std::vector<std::tuple<int, int, int>> vectors = {
		{ 0        , 0, 0 },
		{ 4        , 4, 0 },
		{ 4 | 4096 , 4, 1 },
		{ 4 | 8192 , 4, 2 },
		{ 4 | 16384, 4, 4 },
		{ 4 | 32768, 4, 8 },
	};
	// clang-format on
	for (int i = -1; const auto& [keyMod, answerKey, answerModified] : vectors)
	{
		SUBCASE_FMT("%d_%d", ++i, keyMod)
		{
			const auto& [key, modified] = GlobalInput::DecodeKey(keyMod);
			CHECK(key == answerKey);
			CHECK(modified == answerModified);
		}
	}
}

int GlobalInput::EncodeKey(int key, int modifier)
{
	return (key & 4095) | (modifier << 12);
}

TEST_CASE("EncodeKey")
{
	// clang-format off
	const std::vector<std::tuple<int, int, int>> vectors = {
		{ 0, 0, 0         },
		{ 4, 0, 4         },
		{ 4, 1, 4 | 4096  },
		{ 4, 2, 4 | 8192  },
		{ 4, 4, 4 | 16384 },
		{ 4, 8, 4 | 32768 },
	};
	// clang-format on
	for (int i = -1; const auto& [key, modifier, answer] : vectors)
	{
		SUBCASE_FMT("%d_%d_%d", ++i, key, modifier)
		CHECK(GlobalInput::EncodeKey(key, modifier) == answer);
	}
}

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
			//ui::Log("KeyDownUp: %d %d : %d %d", key, down, lastKey, lastAscii);
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
		Finger&       mouse = GetMouse();
		const int64_t nowMs = NowMs();
		if (state)
		{
			buttonDowns[button] = true;
			// dblClick
			if (const int64_t click = buttonClicks[button]; nowMs < click + xsettings.clickTwo)
			{
				if (DEV_inputMouse) ui::Log("DOUBLE CLICK: %d", button);
				buttonClicks[button] = 0;
				buttonTwos[button]   = true;
				return;
			}
		}
		// click
		else
		{
			buttonUps[button] = true;
			const float dist = bx::distance(bx::load<bx::Vec3>(mouse.rels), bx::load<bx::Vec3>(mouse.rels0)) * 100.0f;
			if (xsettings.clickDist <= 0 || dist < xsettings.clickDist)
			{
				if (nowMs < buttonTimes[button] + xsettings.clickOne)
				{
					if (DEV_inputMouse) ui::Log("ONE CLICK? %d", button);
					buttonClicks[button] = nowMs;
					// buttonOnes[button]   = true;
				}
			}
		}

		buttons[button]     = state;
		buttonTimes[button] = nowMs;

		// reset initial position when clicking
		mouse.rels0[0] = mouse.rels[0];
		mouse.rels0[1] = mouse.rels[1];
		mouse.rels0[2] = mouse.rels[2];
	}
}

void GlobalInput::MouseLock(bool lock)
{
	if (mouseLock != lock)
	{
		entry::setMouseLock(entry::kDefaultWindowHandle, lock);
		mouseLock = lock;
	}
}

void GlobalInput::MouseMove(uint64_t deviceId, uint64_t fingerId, int mx, int my, int mz, bool hasDelta, int dx, int dy, float pressure)
{
	Device& device = devices[deviceId];

	// remove finger?
	if (pressure < 0)
		device.RemoveFinger(fingerId);
	else
	{
		Finger& finger = device.fingers[fingerId];
		finger.Update(mx, my, mz, hasDelta, dx, dy, pressure, resolution);

		if (deviceId) lastDevice = deviceId;
		device.CalculateMotion();
	}

	// if (DEV_inputMouse) ui::Log("MouseMove: %x/%x/%zu %d,%d,%d %d %d,%d,%f", deviceId, fingerId, device.fingers.size(), mx, my, mz, hasDelta, dx, dy, pressure);
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

void GlobalInput::PrintKeys()
{
	if (DEV_inputKeys)
	{
		for (int id = 0; id < entry::Key::Count; ++id)
			if (keyDowns[id]) ui::Log("Controls: %lld %3d %5d %s", keyTimes[id], id, keys[id], getName((entry::Key::Enum)id));
	}
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
	memset(buttons     , 0, sizeof(buttons     ));
	memset(buttonClicks, 0, sizeof(buttonClicks));
	memset(buttonTimes , 0, sizeof(buttonTimes ));
	memset(keyChanges  , 0, sizeof(keyChanges  ));
	memset(keyRepeats  , 0, sizeof(keyRepeats  ));
	memset(keys        , 0, sizeof(keys        ));
	memset(keyTimes    , 0, sizeof(keyTimes    ));
	// clang-format on

	devices.clear();
	keyChangeId = 0;
	mouseLock   = false;

	BeginFrame();
}

void GlobalInput::ResetFixed()
{
	// clang-format off
	memset(buttonOnes , 0, sizeof(buttonOnes ));
	memset(buttonDowns, 0, sizeof(buttonDowns));
	memset(buttonTwos , 0, sizeof(buttonTwos ));
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

void GlobalInput::ShowInfoTable(bool showTitle) const
{
	if (showTitle) ImGui::TextUnformatted("GlobalInput");

	// clang-format off
	std::vector<std::tuple<std::string, std::string>> stats = {
		{ "lastAscii", std::to_string(lastAscii) },
		{ "lastKey"  , std::to_string(lastKey)   },
		{ "mouseLock", BoolString(mouseLock)     },
	};
	// clang-format on

	for (int i = -1; const auto& [did, device] : devices)
	{
		++i;
		stats.push_back({ Format("device.%d", i), Format("%x %d (%zu)", did, device.gesture, device.fingers.size()) });
		stats.push_back({ " - motion", Format("%d %d", TO_INT(device.motion[0] * 1000), TO_INT(device.motion[1] * 1000)) });
		for (int j = -1; const auto& [fid, finger] : device.fingers)
		{
			++j;
			stats.push_back({ Format(" - finger.%d", j), Cstr(TrimDouble(finger.pressure)) });
			stats.push_back({ "   - abs", Format("%d %d", finger.abs[0], finger.abs[1]) });
			stats.push_back({ "   - rel0", Format("%d %d", TO_INT(finger.rels0[0] * 1000), TO_INT(finger.rels0[1] * 1000)) });
			stats.push_back({ "   - rel2", Format("%d %d", TO_INT(finger.rels2[0] * 1000), TO_INT(finger.rels2[1] * 1000)) });
		}
	}

	ui::ShowTable(stats);

}

GlobalInput& GetGlobalInput()
{
	static GlobalInput globalInput;
	return globalInput;
}
