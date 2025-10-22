// fx-Landscape.cpp
// @author octopoulos
// @version 2025-10-18

#include "stdafx.h"
#include "ui/ui-fx.h"

static void Fx_Landscape(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	const ImVec2 center(topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f);

	drawList->AddCircleFilled(center, 10.0f, ImColor(255, 0, 0));
}

FX_REGISTER(Landscape)
