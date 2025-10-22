// fx-Oxo.cpp
// @author octopoulos
// @version 2025-10-18

#include "stdafx.h"
#include "ui/ui-fx.h"

static void Fx_Oxo(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	const ImVec2 center(topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f);
	const float  radius    = size.x * 0.12f;
	const ImVec2 vecRadius = ImVec2(radius, radius);
	const float  stride    = radius * 2.0f + size.x * 0.01f;
	const ImVec2 mousePos  = ImVec2(topLeft.x + mouse.x * size.x, topLeft.y + mouse.y * size.y);

	ImVec2 pos0 = ImVec2(center.x - stride, center.y - stride);

	for (int j = 0; j < 3; ++j)
	{
		for (int i = 0; i < 3; ++i)
		{
			ImVec2  pos   = ImVec2(pos0.x + i * stride, pos0.y + j * stride);
			ImColor color = ImColor(255, 160, 0);
			if (mousePos.x >= pos.x - radius && mousePos.x <= pos.x + radius && mousePos.y >= pos.y - radius && mousePos.y <= pos.y + radius)
				color = ImColor(255, 0, 0);

			drawList->AddRectFilled(pos - vecRadius, pos + vecRadius, color);
			pos.x += stride;
		}
	}
}

FX_REGISTER(Oxo)
