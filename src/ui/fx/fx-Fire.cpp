// fx-Fire.cpp
// @author octopoulos
// @version 2025-10-16

#include "stdafx.h"
#include "ui/ui-fx.h"

namespace ui
{

void Fx_Fire(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	// virtual canvas
	const int virtualX = 160;
	const int virtualY = 90;

	// fire palette
	static const int fireColors[666] = {
		0xffffff, 0xc7efef, 0x9fdfdf, 0x6fcfcf, 0x37b7b7, 0x2fb7b7, 0x2fafb7, 0x2fafbf,
		0x27a7bf, 0x27a7bf, 0x1f9fbf, 0x1f9fbf, 0x1f97c7, 0x178fc7, 0x1787c7, 0x1787cf,
		0xf7fcf, 0xf77cf, 0xf6fcf, 0xf67d7, 0x75fd7, 0x75fd7, 0x757df, 0x757df,
		0x74fdf, 0x747c7, 0x747bf, 0x73faf, 0x72f9f, 0x7278f, 0x71f77, 0x71f67,
		0x71757, 0x70f47, 0x70f2f, 0x7071f, 0x70707
	};

	// fire buffer
	static uint8_t fireBuffer[virtualY + 2][virtualX] = {};

	// frame counter
	static int frameCounter = 0;
	if (20.0f * time > frameCounter)
		++frameCounter;
	else
		time = 0;

	// compute scaling factor
	const ImVec2 scale(size.x / virtualX, size.y / virtualY);

	// helper to convert virtual coordinates to screen
	auto ScreenPos = [&](int x, int y) -> ImVec2 {
		return ImVec2(topLeft.x + x * scale.x, bottomRight.y - y * scale.y);
	};

	// draw fire
	for (int x = 0; x < virtualX; ++x)
	{
		for (int y = 0; y < virtualY; ++y)
		{
			int colorIndex = fireBuffer[y + 1][x];
			drawList->AddRectFilled(
			    ScreenPos(x, y),
			    ScreenPos(x + 1, y + 1),
			    fireColors[colorIndex] | (255U << 24));

			// random propagation
			const int r = 3 & rand();
			if (time) fireBuffer[y + 1][x] = fireBuffer[y][x + r - (r > 1)] + (r & 1);
		}
	}
}

FX_REGISTER(Fire)

} // namespace ui
