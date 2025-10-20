// fx-Circles.cpp
// @author octopoulos
// @version 2025-10-16

#include "stdafx.h"
#include "ui/ui-fx.h"

namespace ui
{

void Fx_Circles(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	// draw concentric circles in the center of the rectangle
	const ImVec2 center(topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f);

	const int circleCount = TO_INT((1.0f + sinf(time * 5.7f)) * 40.0f);

	for (int i = 0; i < circleCount; ++i)
	{
		const float radius = size.y * (0.01f + i * 0.03f);
		const ImU32 color  = IM_COL32(255, 140 - i * 4, i * 3, 255);

		drawList->AddCircle(center, radius, color);
	}
}

FX_REGISTER(Circles)

} // namespace ui
