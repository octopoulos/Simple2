// controls.cpp
// @author octopoulos
// @version 2025-07-24

#include "stdafx.h"
#include "app.h"
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
			xsettings.physPaused = false;
			pauseNextFrame       = true;
		}
		if (downs[Key::KeyP]) xsettings.physPaused = !xsettings.physPaused;

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

	GetGlobalInput().ResetFixed();
	++inputFrame;
}

void App::FluidControls()
{
	auto& ginput = GetGlobalInput();
	ginput.MouseDeltas();

	// camera
	if (!ImGui::MouseOverArea())
	{
		if (ginput.buttons[1])
			camera->Orbit(ginput.mouseRels2[0], ginput.mouseRels2[1]);
		camera->Update(deltaTime);
	}

	// holding down
	if (const auto& keys = ginput.keys)
	{
		using namespace entry;

		float speed = deltaTime * 10.0f;
		if (keys[Key::LeftCtrl]) speed *= 0.5f;
		if (keys[Key::LeftShift]) speed *= 2.0f;

		if (keys[Key::KeyA])
		{
			camera->pos2    = bx::mad(camera->right, -speed, camera->pos2);
			camera->target2 = bx::mad(camera->right, -speed, camera->target2);
		}
		if (keys[Key::KeyD])
		{
			camera->pos2    = bx::mad(camera->right, speed, camera->pos2);
			camera->target2 = bx::mad(camera->right, speed, camera->target2);
		}
		if (keys[Key::KeyE])
		{
			camera->pos2    = bx::mad(camera->up, speed, camera->pos2);
			camera->target2 = bx::mad(camera->up, speed, camera->target2);
		}
		if (keys[Key::KeyQ])
		{
			camera->pos2    = bx::mad(camera->up, -speed, camera->pos2);
			camera->target2 = bx::mad(camera->up, -speed, camera->target2);
		}
		if (keys[Key::KeyS])
		{
			camera->pos2    = bx::mad(camera->forward, -speed, camera->pos2);
			camera->target2 = bx::mad(camera->forward, -speed, camera->target2);
		}
		if (keys[Key::KeyW])
		{
			camera->pos2    = bx::mad(camera->forward, speed, camera->pos2);
			camera->target2 = bx::mad(camera->forward, speed, camera->target2);
		}
	}
}
