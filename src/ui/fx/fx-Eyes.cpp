// fx-Eyes.cpp
// @author octopoulos
// @version 2025-10-18

#include "stdafx.h"
#include "ui/ui-fx.h"

static void Fx_Eyes(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	auto DrawEllipse = [](ImDrawList* drawList, ImVec2 center, float rx, float ry, ImU32 color) {
		drawList->PathClear();
		for (int step = 0; step < 36; ++step)
		{
			float angle = IM_PI / 18.0f * step;
			drawList->PathLineTo(ImVec2(center.x + sinf(angle) * rx, center.y + cosf(angle) * ry));
		}
		drawList->PathFillConvex(color);
	};

	// mouse in local coordinates
	ImVec2 mousePos(topLeft.x + mouse.x * size.x, topLeft.y + mouse.y * size.y);

	const float virtualX = 320.0f;
	const float virtualY = 320.0f;

	ImVec2 scale(size.x / virtualX, size.y / virtualY);

	auto Scale = [&](ImVec2 pos) {
		return ImVec2(topLeft.x + pos.x * scale.x, topLeft.y + pos.y * scale.y);
	};

	ImVec2 center = Scale(ImVec2(virtualX * 0.5f, virtualY * 0.5f));

	// background
	drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(102, 85, 68, 255));

	ImVec2 eyes[2] = {
		Scale(ImVec2(virtualX * 0.5f - 50.0f, virtualY * 0.5f)),
		Scale(ImVec2(virtualX * 0.5f + 50.0f, virtualY * 0.5f))
	};

	for (int ei = 0; ei < 2; ++ei)
	{
		// outer sclera (black) and inner (white)
		DrawEllipse(drawList, eyes[ei], 40.0f * scale.x, 70.0f * scale.y, IM_COL32(0, 0, 0, 255));
		DrawEllipse(drawList, eyes[ei], 35.0f * scale.x, 65.0f * scale.y, IM_COL32(255, 255, 255, 255));

		// vector to mouse
		ImVec2 vecMouse(mousePos.x - eyes[ei].x, mousePos.y - eyes[ei].y);
		float  distMouse = bx::sqrt(vecMouse.x * vecMouse.x + vecMouse.y * vecMouse.y);
		if (distMouse > 1e-5f)
		{
			vecMouse.x /= distMouse;
			vecMouse.y /= distMouse;
		}
		if (distMouse > 20.0f) distMouse = 20.0f;

		ImVec2 pupilPos(
		    eyes[ei].x + vecMouse.x * distMouse * scale.x,
		    eyes[ei].y + vecMouse.y * distMouse * scale.y);

		// Draw pupil
		DrawEllipse(drawList, pupilPos, 10.0f * scale.x, 10.0f * scale.y, IM_COL32(0, 0, 0, 255));
	}
}

FX_REGISTER(Eyes)
