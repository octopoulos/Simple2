// controls.cpp
// @author octopoulos
// @version 2025-07-21

#include "stdafx.h"
#include "app.h"
#include "common/camera.h"
#include "entry/input.h"
#include "imgui/imgui.h"

void App::Controls()
{
	using namespace entry;

	const auto& ginput = GetGlobalInput();

	for (int id = 0; id < Key::Count; ++id)
	{
		if (ginput.keyNews[id]) ui::Log("Controls: {} : {} : {} : {}", ginput.keyTimes[id], id, ginput.keyNews[id], ginput.keys[id]);
	}

	// new keys
	if (const auto& news = ginput.keyNews)
	{
		if (news[Key::F4] & KeyNew_Down)
			bulletDebug = !bulletDebug;

		if (news[Key::F5] & KeyNew_Down)
			isPerspective = !isPerspective;
	}

	MouseState mouseState;
	mouseState.m_buttons[0] = ginput.buttons[0];
	mouseState.m_buttons[1] = ginput.buttons[1];
	mouseState.m_buttons[2] = ginput.buttons[2];
	mouseState.m_buttons[3] = ginput.buttons[3];
	mouseState.m_mx         = ginput.mouseAbs[0];
	mouseState.m_my         = ginput.mouseAbs[1];
	mouseState.m_mz         = ginput.mouseAbs[2];

	cameraUpdate(deltaTime, mouseState, ImGui::MouseOverArea());

	GetGlobalInput().ResetNews();
}
