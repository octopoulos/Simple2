// fx-Chess.cpp
// @author octopoulos
// @version 2025-10-21

#include "stdafx.h"
#include "ui/ui-fx.h"
//
#include "entry/input.h" // GetGlobalInput

static void Fx_Chess(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	auto&       ginput = GetGlobalInput();
	const auto& finger = ginput.GetMouse();

	const ImVec2 center(topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f);

	drawList->AddCircleFilled(center, 10.0f, ImColor(255, 0, 0));
}

FX_REGISTER(Chess)
