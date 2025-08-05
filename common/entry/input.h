// @version 2025-07-31
/*
 * Copyright 2010-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#pragma once

#include "entry.h"

typedef void (*InputBindingFn)(const void* _userData);

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
void inputAddBindings(const char* _name, const InputBinding* _bindings);

///
void inputRemoveBindings(const char* _name);

///
void inputProcess();

///
void inputSetKeyState(entry::Key::Enum _key, uint8_t _modifiers, bool _down);

///
bool inputGetKeyState(entry::Key::Enum _key, uint8_t* _modifiers = nullptr);

///
uint8_t inputGetModifiersState();

/// Adds single UTF-8 encoded character into input buffer.
void inputChar(uint8_t _len, const uint8_t _char[4]);

/// Returns single UTF-8 encoded character from input buffer.
const uint8_t* inputGetChar();

/// Flush internal input buffer.
void inputCharFlush();

///
void inputSetMouseResolution(uint16_t _width, uint16_t _height);

///
void inputSetMousePos(int32_t _mx, int32_t _my, int32_t _mz, bool hasDelta, int32_t _dx, int32_t _dy);

///
void inputSetMouseButtonState(entry::MouseButton::Enum _button, uint8_t _state);

///
void inputSetMouseLock(bool _lock);

///
void inputGetMouse(float _mouse[3]);

///
bool inputIsMouseLocked();

///
void inputSetGamepadAxis(entry::GamepadHandle _handle, entry::GamepadAxis::Enum _axis, int32_t _value);

///
int32_t inputGetGamepadAxis(entry::GamepadHandle _handle, entry::GamepadAxis::Enum _axis);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GlobalInput
//////////////

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
	bool     buttonDowns[8]  = {};                          ///< mouse buttons pushed this frame
	uint8_t  buttons[8]      = {};                          ///< mouse buttons pushed
	int64_t  buttonTimes[8]  = {};                          ///< when the button was pushed last time (in ms)
	bool     buttonUps[8]    = {};                          ///< mouse buttons released this frame
	int      keyChangeId     = 0;                           ///< index of current history
	KeyState keyChanges[128] = {};                          ///< history of key changes
	bool     keyDowns[256]   = {};                          ///< keys pushed this frame
	int64_t  keyRepeats[256] = {};                          ///< last repeat time (in ms)
	bool     keys[256]       = {};                          ///< keys pushed
	int64_t  keyTimes[256]   = {};                          ///< when the key was pushed last time (in ms)
	bool     keyUps[256]     = {};                          ///< keys released this frame
	int      lastAscii       = -1;                          ///< last ascii char pushed
	int      lastKey         = 0;                           ///< last key pushed
	int      mouseAbs[3]     = {};                          ///< mouse absolute coordinates
	int      mouseAbs2[3]    = {};                          ///< mouse absolute coordinates: deltas
	int      mouseFrame      = 0;                           ///< mouse input frame
	bool     mouseLock       = false;                       ///< mouse is locked?
	float    mouseDeltas[3]  = {};                          ///< mouse changes
	float    mouseRels[3]    = {};                          ///< mouse relative coordinates
	float    mouseRels0[3]   = {};                          ///< mouse relative coordinates: previous
	float    mouseRels2[3]   = {};                          ///< mouse relative coordinates: deltas
	int64_t  nowMs           = 0;                           ///< current timestamp (in ms)
	float    resolution[3]   = { 1280.0f, 720.0f, 120.0f }; ///< used to normalize mouse coords

	/// Check the global modifier, or if the key is a modifier
	/// @param key: 0 for global modifier
	/// @returns &1: shift, &2: ctrl, &4: alt, &8: meta
	int IsModifier(int key = 0);

	/// Keyboard key down/up
	void KeyDownUp(int key, bool down);

	/// Convert key to ascii char
	int KeyToAscii(int key);

	/// Mouse button change
	void MouseButton(int button, uint8_t state);

	/// Compute mouse deltas
	/// - call this every frame
	void MouseDeltas();

	/// Mouse locked or unlocked
	void MouseLock(bool lock);

	/// Mouse motion
	void MouseMove(int mx, int my, int mz, bool hasDelta, int dx, int dy);

	/// Should the key be repeated?
	bool RepeatingKey(int key);

	/// Reset data to initial state
	void Reset();

	/// Check if the last ascii must be repeated, if not then reset it
	void ResetAscii();

	/// Reset the keyNews only, must do this every frame
	void ResetFixed();

	/// Set resolution to calculate mouseNorm
	void SetResolution(int width, int height, int wheelDelta);
};

GlobalInput& GetGlobalInput();
