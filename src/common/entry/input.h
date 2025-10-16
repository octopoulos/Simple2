// @version 2025-10-12
/*
 * Copyright 2010-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#pragma once

#include "entry.h"

#include <bx/ringbuffer.h>

typedef void (*InputBindingFn)(const void* _userData);

enum Gestures_ : int
{
	Gesture_None     = 0,
	Gesture_Parallel = 1,
	Gesture_Zoom     = 2,
	Gesture_ZoomIn   = 4 | Gesture_Zoom,
	Gesture_ZoomOut  = 8 | Gesture_Zoom,
};

struct InputBinding
{
	void set(entry::Key::Enum _key, uint8_t _modifiers, uint8_t _flags, InputBindingFn _fn, const void* _userData = nullptr)
	{
		m_key       = _key;
		m_modifiers = _modifiers;
		m_flags     = _flags;
		m_fn        = _fn;
		m_userData  = _userData;
	}

	void end()
	{
		m_key       = entry::Key::None;
		m_modifiers = entry::Modifier::None;
		m_flags     = 0;
		m_fn        = nullptr;
		m_userData  = nullptr;
	}

	entry::Key::Enum m_key;
	uint8_t          m_modifiers;
	uint8_t          m_flags;
	InputBindingFn   m_fn;
	const void*      m_userData;
};

#define INPUT_BINDING(name, command)       { entry::Key::name, entry::Modifier::None, 0, cmd, command }
#define INPUT_BINDING_ALT(name, command)   { entry::Key::name, entry::Modifier::LeftAlt | entry::Modifier::RightAlt, 0, cmd, command }
#define INPUT_BINDING_CTRL(name, command)  { entry::Key::name, entry::Modifier::LeftCtrl | entry::Modifier::RightCtrl, 0, cmd, command }
#define INPUT_BINDING_SHIFT(name, command) { entry::Key::name, entry::Modifier::LeftShift | entry::Modifier::RightShift, 0, cmd, command }
#define INPUT_BINDING_END                  { entry::Key::None, entry::Modifier::None, 0, nullptr, nullptr }

///
void inputInit();

///
void inputShutdown();

///
void inputSetGamepadAxis(entry::GamepadHandle _handle, entry::GamepadAxis::Enum _axis, int32_t _value);

///
int32_t inputGetGamepadAxis(entry::GamepadHandle _handle, entry::GamepadAxis::Enum _axis);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FINGER
/////////

struct Finger
{
	int     abs[3]   = {};    ///< x, y, z/pressure - direct access
	bool    down     = false; ///< pushed down?
	int64_t downTime = 0;     ///< when was it pushed
	int     frame    = 0;     ///< used to initialize rels0 on the first frame
	float   pressure = 0.0f;  ///< finger pressure
	float   rels[3]  = {};    ///< normalized x, y, z
	float   rels0[3] = {};    ///< previous frame normalized
	float   rels2[3] = {};    ///< deltas between frames

	/// Compute finger deltas
	/// - call this every frame
	void ComputeDeltas();

	/// Finger/mouse action
	void Update(int mx, int my, int mz, bool hasDelta, int dx, int dy, float pressure = 0.0f, const float* resolution = nullptr);
};

struct Device
{
	MAP<uint64_t, Finger> fingers   = {}; ///< fingers (trackpad)
	int                   gesture   = 0;  ///< Gestures_
	float                 motion[3] = {}; ///< average motion vector

	/// Calculate average motion
	/// + check if all fingers move in the same direction
	/// + check if (2) fingers go in opposite directions
	void CalculateMotion();

	/// Remove a finger
	void RemoveFinger(uint64_t fingerId);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GlobalInput
//////////////

#define GI_DOWN(key)          (!ignores[key] && downs[key])
#define GI_KEY(key)           (!ignores[key] && keys[key])
#define GI_REPEAT(key)        ((!ignores[key] && (downs[key] || ginput.RepeatingKey(key))) ? 1 : 0)
#define GI_REPEAT_CURSOR(key) ((!ignores[key] && (downs[key] || ginput.RepeatingKey(key, xsettings.cursorInit, xsettings.cursorRepeat))) ? 1 : 0)
#define GI_REPEAT_RUBIK(key)  ((!ignores[key] && (downs[key] || ginput.RepeatingKey(key, xsettings.rubikInit, xsettings.rubikRepeat))) ? 1 : 0)

enum Modifiers_ : int
{
	Modifier_None  = 0,
	Modifier_Shift = 1,
	Modifier_Ctrl  = 2,
	Modifier_Alt   = 4,
	Modifier_Meta  = 8,
};

struct KeyState
{
	int     key;   ///< key code
	bool    down;  ///< key down or up
	int64_t stamp; ///< timestamp in ms
};

struct GlobalInput
{
	int64_t               buttonClicks[8] = {};                          ///< mouse click times
	int                   buttonCounts[8] = {};                          ///< how many times the button was pushed
	bool                  buttonDowns[8]  = {};                          ///< mouse buttons pushed this frame
	bool                  buttonOnes[8]   = {};                          ///< mouse single clicks
	uint8_t               buttons[8]      = {};                          ///< mouse buttons pushed
	int64_t               buttonTimes[8]  = {};                          ///< when the button was pushed last time (in ms)
	bool                  buttonTwos[8]   = {};                          ///< mouse double clicks
	bool                  buttonUps[8]    = {};                          ///< mouse buttons released this frame
	MAP<uint64_t, Device> devices         = {};                          ///< devices, 0: mouse, 1+: others
	int                   keyChangeId     = 0;                           ///< index of current history
	KeyState              keyChanges[128] = {};                          ///< history of key changes
	bool                  keyDowns[256]   = {};                          ///< keys pushed this frame
	bool                  keyIgnores[256] = {};                          ///< keys to ignore (ex: already handled by RubikCube)
	int64_t               keyRepeats[256] = {};                          ///< last repeat time (in ms)
	bool                  keys[256]       = {};                          ///< keys pushed
	int64_t               keyTimes[256]   = {};                          ///< when the key was pushed last time (in ms)
	bool                  keyUps[256]     = {};                          ///< keys released this frame
	int                   lastAscii       = -1;                          ///< last ascii char pushed
	uint64_t              lastDevice      = 0;                           ///< last used device other than mouse
	int                   lastKey         = 0;                           ///< last key pushed
	bool                  mouseLock       = false;                       ///< mouse is locked?
	int64_t               nowMs           = 0;                           ///< current timestamp (in ms)
	float                 resolution[3]   = { 1280.0f, 720.0f, 120.0f }; ///< used to normalize mouse coords

	// char ring
	uint8_t               chars[256] = {};
	bx::RingBufferControl ring;

	GlobalInput(): ring(BX_COUNTOF(chars) - 4)
	{
	}

	/// Check if the last ascii must be repeated, if not then reset it
	/// + reset keyIgnores
	/// + executed every frame
	void BeginFrame();

	/// Compute all finger deltas
	/// - call this every frame
	void ComputeDeltas();

	/// Decode encoded to key, modifier
	static std::pair<int, int> DecodeKey(int keyMod);

	/// Encode a key, modifier into an encoded key
	static int EncodeKey(int key, int modifier);

	/// Reset the char ring
	void FlushChar();

	/// Get the mouse (device 0, finger 0)
	Finger& GetMouse() { return devices[0].fingers[0]; }

	/// Check the global modifier, or if the key is a modifier
	/// @param key: 0 for global modifier
	/// @returns &1: shift, &2: ctrl, &4: alt, &8: meta
	int IsModifier(int key = 0) const;

	/// Keyboard key down/up
	void KeyDownUp(int key, bool down);

	/// Convert key to ascii char
	int KeyToAscii(int key) const;

	/// Mouse button change
	void MouseButton(int button, uint8_t state);

	/// Mouse locked or unlocked
	void MouseLock(bool lock);

	/// Mouse motion / trackpad action
	void MouseMove(uint64_t deviceId, uint64_t fingerId, int mx, int my, int mz, bool hasDelta, int dx, int dy, float pressure);

	/// Pop a char from the ring
	const uint8_t* PopChar();

	/// Print keys being pushed
	void PrintKeys();

	/// Push a char onto the ring
	void PushChar(uint8_t _len, const uint8_t _char[4]);

	/// Should the key be repeated?
	bool RepeatingKey(int key, int64_t keyInit = -1, int64_t keyRepeat = -1);

	/// Reset data to initial state
	void Reset();

	/// Reset the keyNews only, must do this every frame
	void ResetFixed();

	/// Set resolution to calculate mouseNorm
	void SetResolution(int width, int height, int wheelDelta);

	/// Show info table in ImGui
	void ShowInfoTable(bool showTitle = true) const;
};

GlobalInput& GetGlobalInput();
