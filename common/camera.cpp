// @version 2025-07-17
/*
 * Copyright 2013 Dario Manesku. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "stdafx.h"
#include "camera.h"
#include "entry/cmd.h"
#include "entry/entry.h"
#include "entry/input.h"
#include <bx/allocator.h>
#include <bx/math.h>
#include <bx/timer.h>

enum CustomCommands_
{
	Cmd_Invalid = -1,
	Cmd_Forward,
	Cmd_Forward2,
	Cmd_Backward,
	Cmd_Left,
	Cmd_Right,
	Cmd_Up,
	Cmd_Down,
};

// clang-format off
static const UMAP_STR_INT s_cmdMoveMap = {
	{ "forward" , Cmd_Forward  },
	{ "forward2", Cmd_Forward2 },
	{ "backward", Cmd_Backward },
	{ "left"    , Cmd_Left     },
	{ "right"   , Cmd_Right    },
	{ "up"      , Cmd_Up       },
	{ "down"    , Cmd_Down     },
};
// clang-format on

int cmdMove(CmdContext* /*_context*/, void* /*_userData*/, int _argc, char const* const* _argv)
{
	if (_argc > 1)
	{
		const int command = FindDefault(s_cmdMoveMap, _argv[1], Cmd_Invalid);
		switch (command)
		{
		case Cmd_Backward: cameraSetKeyState(CAMERA_KEY_BACKWARD, true); break;
		case Cmd_Down    : cameraSetKeyState(CAMERA_KEY_DOWN    , true); break;
		case Cmd_Forward : cameraSetKeyState(CAMERA_KEY_FORWARD , true); break;
		case Cmd_Forward2: cameraSetKeyState(CAMERA_KEY_FORWARD , true); break;
		case Cmd_Left    : cameraSetKeyState(CAMERA_KEY_LEFT    , true); break;
		case Cmd_Right   : cameraSetKeyState(CAMERA_KEY_RIGHT   , true); break;
		case Cmd_Up      : cameraSetKeyState(CAMERA_KEY_UP      , true); break;
		}
	}

	return 1;
}

static void cmd(const void* _userData)
{
	cmdExec((const char*)_userData);
}

// clang-format off
static const InputBinding s_camBindings[] = {
	INPUT_BINDING_SHIFT(KeyW            , "move forward2"),
	INPUT_BINDING      (KeyW            , "move forward" ),
	INPUT_BINDING      (GamepadUp       , "move forward" ),
	INPUT_BINDING      (KeyA            , "move left"    ),
	INPUT_BINDING      (GamepadLeft     , "move left"    ),
	INPUT_BINDING      (KeyS            , "move backward"),
	INPUT_BINDING      (GamepadDown     , "move backward"),
	INPUT_BINDING      (KeyD            , "move right"   ),
	INPUT_BINDING      (GamepadRight    , "move right"   ),
	INPUT_BINDING      (KeyQ            , "move down"    ),
	INPUT_BINDING      (GamepadShoulderL, "move down"    ),
	INPUT_BINDING      (KeyE            , "move up"      ),
	INPUT_BINDING      (GamepadShoulderR, "move up"      ),
	INPUT_BINDING_END
};
// clang-format on

struct Camera
{
	struct MouseCoords
	{
		int32_t m_mx;
		int32_t m_my;
		int32_t m_mz;
	};

	Camera()
	{
		reset();
		entry::MouseState mouseState;
		update(0.0f, mouseState, true);

		cmdAdd("move", cmdMove);
		inputAddBindings("camBindings", s_camBindings);
	}

	~Camera()
	{
		cmdRemove("move");
		inputRemoveBindings("camBindings");
	}

	void reset()
	{
		m_mouseNow.m_mx   = 0;
		m_mouseNow.m_my   = 0;
		m_mouseNow.m_mz   = 0;
		m_mouseLast.m_mx  = 0;
		m_mouseLast.m_my  = 0;
		m_mouseLast.m_mz  = 0;
		m_eye.x           = 0.0f;
		m_eye.y           = 0.0f;
		m_eye.z           = -35.0f;
		m_at.x            = 0.0f;
		m_at.y            = 0.0f;
		m_at.z            = -1.0f;
		m_up.x            = 0.0f;
		m_up.y            = 1.0f;
		m_up.z            = 0.0f;
		m_horizontalAngle = 0.01f;
		m_verticalAngle   = 0.0f;
		m_mouseSpeed      = 0.005f;
		m_gamepadSpeed    = 0.04f;
		m_moveSpeed       = 10.0f;
		m_keys            = 0;
		m_mouseDown       = false;
	}

	void setKeyState(uint8_t _key, bool _down)
	{
		m_keys &= ~_key;
		m_keys |= _down ? _key : 0;
	}

	void update(float _deltaTime, const entry::MouseState& _mouseState, bool _reset)
	{
		if (_reset)
		{
			m_mouseLast.m_mx = _mouseState.m_mx;
			m_mouseLast.m_my = _mouseState.m_my;
			m_mouseLast.m_mz = _mouseState.m_mz;
			m_mouseNow       = m_mouseLast;
			m_mouseDown      = false;

			return;
		}

		if (!m_mouseDown)
		{
			m_mouseLast.m_mx = _mouseState.m_mx;
			m_mouseLast.m_my = _mouseState.m_my;
		}

		m_mouseDown = !!_mouseState.m_buttons[entry::MouseButton::Left];

		if (m_mouseDown)
		{
			m_mouseNow.m_mx = _mouseState.m_mx;
			m_mouseNow.m_my = _mouseState.m_my;
		}

		m_mouseLast.m_mz = m_mouseNow.m_mz;
		m_mouseNow.m_mz  = _mouseState.m_mz;

		const float deltaZ = float(m_mouseNow.m_mz - m_mouseLast.m_mz);

		if (m_mouseDown)
		{
			const int32_t deltaX = m_mouseNow.m_mx - m_mouseLast.m_mx;
			const int32_t deltaY = m_mouseNow.m_my - m_mouseLast.m_my;

			m_horizontalAngle += m_mouseSpeed * float(deltaX);
			m_verticalAngle -= m_mouseSpeed * float(deltaY);

			m_mouseLast.m_mx = m_mouseNow.m_mx;
			m_mouseLast.m_my = m_mouseNow.m_my;
		}

		entry::GamepadHandle handle = { 0 };
		m_horizontalAngle += m_gamepadSpeed * inputGetGamepadAxis(handle, entry::GamepadAxis::RightX) / 32768.0f;
		m_verticalAngle -= m_gamepadSpeed * inputGetGamepadAxis(handle, entry::GamepadAxis::RightY) / 32768.0f;
		const int32_t gpx = inputGetGamepadAxis(handle, entry::GamepadAxis::LeftX);
		const int32_t gpy = inputGetGamepadAxis(handle, entry::GamepadAxis::LeftY);
		m_keys |= gpx < -16834 ? CAMERA_KEY_LEFT : 0;
		m_keys |= gpx > 16834 ? CAMERA_KEY_RIGHT : 0;
		m_keys |= gpy < -16834 ? CAMERA_KEY_FORWARD : 0;
		m_keys |= gpy > 16834 ? CAMERA_KEY_BACKWARD : 0;

		const bx::Vec3 direction = {
			bx::cos(m_verticalAngle) * bx::sin(m_horizontalAngle),
			bx::sin(m_verticalAngle),
			bx::cos(m_verticalAngle) * bx::cos(m_horizontalAngle),
		};

		const bx::Vec3 right = {
			bx::sin(m_horizontalAngle - bx::kPiHalf),
			0.0f,
			bx::cos(m_horizontalAngle - bx::kPiHalf),
		};

		const bx::Vec3 up = bx::cross(right, direction);

		m_eye = bx::mad(direction, deltaZ * _deltaTime * m_moveSpeed, m_eye);

		if (m_keys & CAMERA_KEY_FORWARD)
		{
			m_eye = bx::mad(direction, _deltaTime * m_moveSpeed, m_eye);
			setKeyState(CAMERA_KEY_FORWARD, false);
		}

		if (m_keys & CAMERA_KEY_BACKWARD)
		{
			m_eye = bx::mad(direction, -_deltaTime * m_moveSpeed, m_eye);
			setKeyState(CAMERA_KEY_BACKWARD, false);
		}

		if (m_keys & CAMERA_KEY_LEFT)
		{
			m_eye = bx::mad(right, _deltaTime * m_moveSpeed, m_eye);
			setKeyState(CAMERA_KEY_LEFT, false);
		}

		if (m_keys & CAMERA_KEY_RIGHT)
		{
			m_eye = bx::mad(right, -_deltaTime * m_moveSpeed, m_eye);
			setKeyState(CAMERA_KEY_RIGHT, false);
		}

		if (m_keys & CAMERA_KEY_UP)
		{
			m_eye = bx::mad(up, _deltaTime * m_moveSpeed, m_eye);
			setKeyState(CAMERA_KEY_UP, false);
		}

		if (m_keys & CAMERA_KEY_DOWN)
		{
			m_eye = bx::mad(up, -_deltaTime * m_moveSpeed, m_eye);
			setKeyState(CAMERA_KEY_DOWN, false);
		}

		m_at = bx::add(m_eye, direction);
		m_up = bx::cross(right, direction);
	}

	void getViewMtx(float* _viewMtx)
	{
		bx::mtxLookAt(_viewMtx, bx::load<bx::Vec3>(&m_eye.x), bx::load<bx::Vec3>(&m_at.x), bx::load<bx::Vec3>(&m_up.x));
	}

	void setPosition(const bx::Vec3& _pos)
	{
		m_eye = _pos;
	}

	void setVerticalAngle(float _verticalAngle)
	{
		m_verticalAngle = _verticalAngle;
	}

	void setHorizontalAngle(float _horizontalAngle)
	{
		m_horizontalAngle = _horizontalAngle;
	}

	MouseCoords m_mouseNow;
	MouseCoords m_mouseLast;

	bx::Vec3 m_eye = bx::InitZero;
	bx::Vec3 m_at  = bx::InitZero;
	bx::Vec3 m_up  = bx::InitZero;
	float    m_horizontalAngle;
	float    m_verticalAngle;

	float m_mouseSpeed;
	float m_gamepadSpeed;
	float m_moveSpeed;

	uint8_t m_keys;
	bool    m_mouseDown;
};

static Camera* s_camera = nullptr;

void cameraCreate()
{
	s_camera = BX_NEW(entry::getAllocator(), Camera);
}

void cameraDestroy()
{
	bx::deleteObject(entry::getAllocator(), s_camera);
	s_camera = nullptr;
}

void cameraSetPosition(const bx::Vec3& _pos)
{
	s_camera->setPosition(_pos);
}

void cameraSetHorizontalAngle(float _horizontalAngle)
{
	s_camera->setHorizontalAngle(_horizontalAngle);
}

void cameraSetVerticalAngle(float _verticalAngle)
{
	s_camera->setVerticalAngle(_verticalAngle);
}

void cameraSetKeyState(uint8_t _key, bool _down)
{
	s_camera->setKeyState(_key, _down);
}

void cameraGetViewMtx(float* _viewMtx)
{
	s_camera->getViewMtx(_viewMtx);
}

bx::Vec3 cameraGetPosition()
{
	return s_camera->m_eye;
}

bx::Vec3 cameraGetAt()
{
	return s_camera->m_at;
}

void cameraUpdate(float _deltaTime, const entry::MouseState& _mouseState, bool _reset)
{
	s_camera->update(_deltaTime, _mouseState, _reset);
}
