// fx-Loading.cpp
// @author octopoulos
// @version 2025-10-16

#include "stdafx.h"
#include "ui/ui-fx.h"

namespace ui
{

void Fx_Loading(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	static float lastTriggerTime = 0.0f;

	// every 2 seconds, advance trigger
	if (time > lastTriggerTime + 2.0f)
		lastTriggerTime += 2.0f;

	// compute color at current phase
	auto makeColor = [](float x) {
		const int r = TO_INT((bx::sin(x) + 1.0f) * 255.0f / 2.0f);
		const int g = TO_INT((bx::cos(x) + 1.0f) * 255.0f / 2.0f);
		const int b = 99;
		const int a = 255;
		return IM_COL32(r, g, b, a);
	};

	const ImU32 backgroundColor = makeColor(lastTriggerTime);
	const ImU32 barColor        = makeColor(lastTriggerTime + 2.0f);

	// fill the full background rectangle
	drawList->AddRectFilled(topLeft, bottomRight, backgroundColor);

	// draw 8 horizontal bars with animated width
	const float rowHeight = size.y / 8.0f;
	for (int i = 0; i < 8; ++i)
	{
		float barRightX = bottomRight.x;

		// each row starts animation slightly offset in time
		const float rowDelay = (i / 7.0f) * 0.2f;
		const float elapsed  = bx::max(time - lastTriggerTime - rowDelay, 0.0f);

		if (time - lastTriggerTime < rowDelay + 1.0f)
		{
			float progress = 0.0f;
			// ease-in quart
			if (elapsed < 0.5f)
				progress = 8.0f * elapsed * elapsed * elapsed * elapsed;
			// ease-out quart
			else
				progress = bx::min(1.0f - bx::pow(-2.0f * elapsed + 2.0f, 4.0f) / 2.0f, 1.0f);

			barRightX = topLeft.x + progress * size.x;
		}

		const float y0 = topLeft.y + rowHeight * i;
		const float y1 = y0 + rowHeight;
		drawList->AddRectFilled(ImVec2(topLeft.x, y0), ImVec2(barRightX, y1), barColor);
	}
}

FX_REGISTER(Loading)

} // namespace ui
