// fx-Tunnel.cpp
// @author octopoulos
// @version 2025-10-16

#include "stdafx.h"
#include "ui/ui-fx.h"

namespace ui
{

void Fx_Tunnel(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	// helper: convert from tunnel virtual coordinates to screen coordinates
	auto Project = [&](ImVec2 pos, float depth, const ImVec2& rectSize, const ImVec2& offset) -> ImVec2 {
		return ImVec2((pos.x / depth) * rectSize.x * 5.f + rectSize.x * 0.5f, (pos.y / depth) * rectSize.y * 5.f + rectSize.y * 0.5f) + offset;
	};

	// helper: rotate 2d vector
	auto Rotate = [](ImVec2 v, float angle) -> ImVec2 {
		angle *= 0.1f;
		const float c = cosf(angle);
		const float s = sinf(angle);
		return ImVec2(v.x * c - v.y * s, v.x * s + v.y * c);
	};

	// fill background
	drawList->AddRectFilled(topLeft, bottomRight, 0xFF000000, 0);

	const float t = time * 4.f;

	for (int i = 0; i < 20; ++i)
	{
		const float z           = 21.f - i - (t - floorf(t)) * 2.f;
		const float rotation    = -t * 2.1f + z;
		const float offset0     = -t + z * 0.2f;
		const float offset1     = -t + (z + 1.f) * 0.2f;
		const float offsetScale = 0.3f;

		const ImVec2 scale[] = {
			ImVec2(cosf((t + z) * 0.1f) * 0.2f + 1.f, sinf((t + z) * 0.1f) * 0.2f + 1.f),
			ImVec2(cosf((t + z + 1.f) * 0.1f) * 0.2f + 1.f, sinf((t + z + 1.f) * 0.1f) * 0.2f + 1.f)
		};

		const ImVec2 offsets[] = {
			ImVec2(cosf(offset0) * offsetScale, sinf(offset0) * offsetScale),
			ImVec2(cosf(offset1) * offsetScale, sinf(offset1) * offsetScale)
		};

		const ImVec2 quadCorners[4] = { ImVec2(-1, -1), ImVec2(1, -1), ImVec2(1, 1), ImVec2(-1, 1) };
		ImVec2       pts[8];

		// compute 8 points for the quad slice
		for (int j = 0; j < 8; ++j)
		{
			const int n = j / 4;
			pts[j]      = Project(Rotate(quadCorners[j % 4] * scale[n] + offsets[n], rotation + n), (z + n) * 2.f, size, topLeft);
		}

		// draw 4 quads
		for (int j = 0; j < 4; ++j)
		{
			const ImVec2 quad[4]   = { pts[j], pts[(j + 1) % 4], pts[((j + 1) % 4) + 4], pts[j + 4] };
			const float  intensity = (((i & 1) ? 0.5f : 0.6f) + j * 0.05f) * ((21.f - z) / 21.f);
			drawList->AddConvexPolyFilled(quad, 4, ImColor::HSV(0.6f + sinf(time * 0.03f) * 0.5f, 1.f, sqrtf(intensity)));
		}
	}
}

FX_REGISTER(Tunnel)

} // namespace ui
