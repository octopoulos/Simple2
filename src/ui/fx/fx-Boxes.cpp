// fx-Boxes.cpp
// @author octopoulos
// @version 2025-10-16

#include "stdafx.h"
#include "ui/ui-fx.h"

namespace ui
{

void Fx_Boxes(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	struct Box
	{
		ImVec2 pos;
		ImVec2 vel;
		float  size;
		ImU32  color;
	};

	static std::vector<Box> boxes;
	static bool             initialized = false;

	if (!initialized)
	{
		initialized        = true;
		const int numBoxes = 20;
		boxes.reserve(numBoxes);
		for (int i = 0; i < numBoxes; ++i)
		{
			const float  bsize = 8.0f + (i % 5) * 3.0f;
			const ImVec2 pos  = {
				MerseneFloat(topLeft.x, bottomRight.x),
				MerseneFloat(topLeft.y, bottomRight.y),
			};
			const ImVec2 speed = { MerseneFloat(-150.0f, 150.0f), MerseneFloat(-150.0f, 150.0f) };
			const ImU32  color = IM_COL32(rand() % 256, rand() % 256, rand() % 256, 200);
			boxes.push_back({ pos, speed, bsize, color });
		}
	}

	// compute delta time from frame-to-frame
	static float lastTime = time;
	float        dt       = time - lastTime;
	if (dt > 0.1f) dt = 0.1f; // clamp for safety
	lastTime = time;

	// move & bounce
	for (auto& b : boxes)
	{
		b.pos.x += b.vel.x * dt;
		b.pos.y += b.vel.y * dt;

		// collision with left/right
		if (b.pos.x - b.size < topLeft.x)
		{
			b.pos.x = topLeft.x + b.size;
			b.vel.x *= -1;
		}
		else if (b.pos.x + b.size > bottomRight.x)
		{
			b.pos.x = bottomRight.x - b.size;
			b.vel.x *= -1;
		}

		// collision with top/bottom
		if (b.pos.y - b.size < topLeft.y)
		{
			b.pos.y = topLeft.y + b.size;
			b.vel.y *= -1;
		}
		else if (b.pos.y + b.size > bottomRight.y)
		{
			b.pos.y = bottomRight.y - b.size;
			b.vel.y *= -1;
		}
	}

	// draw all boxes
	for (auto& b : boxes)
	{
		ImVec2 min = ImVec2(b.pos.x - b.size, b.pos.y - b.size);
		ImVec2 max = ImVec2(b.pos.x + b.size, b.pos.y + b.size);
		drawList->AddRectFilled(min, max, b.color, 3.0f); // rounded square
	}
}

FX_REGISTER(Boxes)

} // namespace ui
