// controls.cpp
// @author octopoulos
// @version 2025-07-24

#include "stdafx.h"
#include "app.h"
#include "common/camera.h"
#include "entry/input.h"
#include "imgui/imgui.h"

void App::FixedControls()
{
	using namespace entry;

	const auto& ginput = GetGlobalInput();

	for (int id = 0; id < Key::Count; ++id)
		if (ginput.keyDowns[id]) ui::Log("Controls: {} {:3} {:5} {}", ginput.keyTimes[id], id, ginput.keys[id], getName((Key::Enum)id));

	// new keys
	if (const auto& downs = ginput.keyDowns)
	{
		if (downs[Key::KeyO])
		{
			isPaused       = false;
			pauseNextFrame = true;
		}
		if (downs[Key::KeyP]) isPaused = !isPaused;

		if (downs[Key::Down])
		{
			cursor->position.z = std::roundf(cursor->position.z - 1.0f);
			cursor->UpdateLocalMatrix();
		}
		if (downs[Key::Left])
		{
			cursor->position.x = std::roundf(cursor->position.x - 1.0f);
			cursor->UpdateLocalMatrix();
		}
		if (downs[Key::Right])
		{
			cursor->position.x = std::roundf(cursor->position.x + 1.0f);
			cursor->UpdateLocalMatrix();
		}
		if (downs[Key::Up])
		{
			cursor->position.z = std::roundf(cursor->position.z + 1.0f);
			cursor->UpdateLocalMatrix();
		}

		if (downs[Key::F4]) bulletDebug = !bulletDebug;
		if (downs[Key::F5]) xsettings.projection = 1 - xsettings.projection;
		if (downs[Key::F11]) xsettings.instancing = !xsettings.instancing;
	}

	// holding down
	if (const auto& keys = ginput.keys)
	{
		if (keys[Key::NumPadMinus] || keys[Key::Minus])
			xsettings.orthoZoom = std::min(xsettings.orthoZoom * 1.02f, 20.0f);
		if (keys[Key::NumPadPlus] || keys[Key::Equals])
			xsettings.orthoZoom = std::max(xsettings.orthoZoom / 1.02f, 0.0002f);
	}

	GetGlobalInput().ResetNews();

	++inputFrame;
}

void App::FluidControls()
{
	const auto& ginput = GetGlobalInput();

	// camera
	{
		entry::MouseState mouseState;
		mouseState.m_buttons[0] = ginput.buttons[0];
		mouseState.m_buttons[1] = ginput.buttons[1];
		mouseState.m_buttons[2] = ginput.buttons[2];
		mouseState.m_buttons[3] = ginput.buttons[3];
		mouseState.m_mx         = ginput.mouseAbs[0];
		mouseState.m_my         = ginput.mouseAbs[1];
		mouseState.m_mz         = ginput.mouseAbs[2];

		cameraUpdate(deltaTime, mouseState, ImGui::MouseOverArea());
	}
}
