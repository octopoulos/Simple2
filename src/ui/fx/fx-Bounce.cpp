// fx-Bounce.cpp
// @author octopoulos
// @version 2025-10-16

#include "stdafx.h"
#include "ui/ui-fx.h"

namespace ui
{

void Fx_Bounce(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	// 1) state
	static ImVec2 pos      = ImVec2(0.0f, 0.0f);
	static ImVec2 speed    = ImVec2(0.0f, 0.0f);
	static ImVec2 lastSize = ImVec2(0.0f, 0.0f);
	static float  lastTime = 0.0f;
	static bool   needInit = true;
	static float  radius   = 10.0f;

	// reset when user requests (mouse.w == 0) or when size changes
	if (!mouse.w) needInit = true;
	if (lastSize.x != size.x || lastSize.y != size.y) needInit = true;

	if (needInit)
	{
		pos.x   = topLeft.x + size.x * 0.5f;
		pos.y   = topLeft.y + size.y * 0.5f;
		radius  = size.x * 0.05f;
		speed.x = radius * MerseneFloat(6.0f, 30.0f) * ((MerseneInt32() & 1) ? -1 : 1);
		speed.y = radius * MerseneFloat(6.0f, 30.0f) * ((MerseneInt32() & 1) ? -1 : 1);

		lastSize = size;
		lastTime = time;
		needInit = false;
	}

	// 2)
	const float delta = time - lastTime;
	lastTime = time;
	pos += speed * delta;

	// clang-format off
	if (pos.x - radius < topLeft.x    ) speed.x =  bx::abs(speed.x);
	if (pos.x + radius > bottomRight.x) speed.x = -bx::abs(speed.x);
	if (pos.y - radius < topLeft.y    ) speed.y =  bx::abs(speed.y);
	if (pos.y + radius > bottomRight.y) speed.y = -bx::abs(speed.y);
	// clang-format on

	drawList->AddCircleFilled(pos, radius, ImColor(255, 0, 0));
}

FX_REGISTER(Bounce)

} // namespace ui
