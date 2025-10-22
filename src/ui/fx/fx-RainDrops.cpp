// fx-RainDrops.cpp
// @author octopoulos
// @version 2025-10-18

#include "stdafx.h"
#include "ui/ui-fx.h"

static void Fx_RainDrops(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	static float lastFlashTime = 0.0f;

	// occasionally trigger a white flash
	if ((rand() % 500) == 0)
		lastFlashTime = time;

	if ((time - lastFlashTime) > 0)
	{
		const float flashDuration = 0.25f;
		const float alpha         = (flashDuration - (time - lastFlashTime)) / flashDuration;
		if (alpha > 0.0f) drawList->AddRectFilled(topLeft, bottomRight, ImColor(1.0f, 1.0f, 1.0f, alpha));
	}

	// draw randomized sparkles / glitchy effects
	for (int i = 0; i < 2000; ++i)
	{
		// use ImGui ID system to generate a pseudo-random seed
		const uint32_t hash = ImGui::GetID(drawList + i + int(time / 4));

		// fractional time offset
		const float f = fmodf(time + fmodf(hash / 777.f, 99.0f), 99.0f);

		// pseudo-random position inside the box
		const int   area = (int)(size.x * size.y);
		const float tx   = (hash % area) % (int)size.x;
		const float ty   = (hash % area) / (int)size.x;

		if (f < 1.0f)
		{
			// falling line effect
			const float py = ty - 1000.0f * (1.0f - f);
			drawList->AddLine(
			    ImVec2(topLeft.x + tx, topLeft.y + py),
			    ImVec2(topLeft.x + tx, topLeft.y + bx::min(py + 10.0f, ty)),
			    (ImU32)-1
			);
		}
		else if (f < 1.2f)
		{
			// small circle "sparkle"
			const float radius = (f - 1.0f) * 10.0f + (hash % 5);
			const float alpha  = 1.0f - (f - 1.0f) * 5.0f;
			drawList->AddCircle(
			    ImVec2(topLeft.x + tx, topLeft.y + ty),
			    radius,
			    ImColor(1.0f, 1.0f, 1.0f, alpha)
			);
		}
	}
}

FX_REGISTER(RainDrops)
