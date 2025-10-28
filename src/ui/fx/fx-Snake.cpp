// fx-Snake.cpp
// @author octopoulos
// @version 2025-10-21

#include "stdafx.h"
#include "ui/ui-fx.h"
//
#include "entry/input.h" // GetGlobalInput

static void Fx_Snake(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	auto&       ginput = GetGlobalInput();
	const auto& finger = ginput.GetMouse();

	const ImVec2 center(topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f);
	const float  radius = size.x * 0.3f;

	// 1) draw axes
	drawList->AddLine(ImVec2(topLeft.x, center.y), ImVec2(bottomRight.x, center.y), IM_COL32(128, 128, 128, 255));
	drawList->AddLine(ImVec2(center.x, topLeft.y), ImVec2(center.x, bottomRight.y), IM_COL32(128, 128, 128, 255));

	// 2) draw circle
	drawList->AddCircle(center, radius, IM_COL32(128, 128, 128, 255));

	// 3) draw object
	const float radius2 = size.x * 0.02f;
	static float angle = 0.52f;
	const ImVec2 pos(radius * bx::cos(angle), radius * bx::sin(angle));
	drawList->AddCircleFilled(center + pos, radius2, IM_COL32(255, 0, 0, 255));

	angle = finger.abs[0] * 0.01f;
}

FX_REGISTER(Snake)
