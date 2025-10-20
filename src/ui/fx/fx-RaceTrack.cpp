// fx-RaceTrack.cpp
// @author octopoulos
// @version 2025-10-16

#include "stdafx.h"
#include "ui/ui-fx.h"

namespace ui
{

void Fx_RaceTrack(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	// virtual race track parameters
	static float trackOffset = 0.0f;
	trackOffset += 0.5f;

	// base coordinates
	ImVec2 bl  = { 160.0f, 90.0f };
	ImVec2 br  = bl;
	ImVec2 bli = bl;
	ImVec2 bri = bl;

	// draw starting quads
	drawList->AddQuadFilled(
	    topLeft,
	    { bottomRight.x, topLeft.y },
	    { bottomRight.x, topLeft.y + 30.0f },
	    { topLeft.x, topLeft.y + 30.0f },
	    0xffffff00);
	drawList->AddQuadFilled(
	    { topLeft.x, topLeft.y + 30.0f },
	    { bottomRight.x, topLeft.y + 30.0f },
	    bottomRight,
	    { topLeft.x, bottomRight.y },
	    0xff007f00);

	// draw track segments
	for (int segment = 300; segment > 0; --segment)
	{
		const float curve  = sinf((trackOffset + segment) * 0.1f) * 500.0f;
		const float offset = cosf((trackOffset + segment) * 0.02f) * 1000.0f;

		const ImVec2 prevTopLeft    = bl;
		const ImVec2 prevTopRight   = br;
		const ImVec2 prevInnerLeft  = bli;
		const ImVec2 prevInnerRight = bri;

		// slightly adjust inner coordinates
		ImVec2 innerLeft  = prevInnerLeft;
		ImVec2 innerRight = prevInnerRight;
		++innerLeft.y; // shift down 1 pixel
		++innerRight.y;

		const float ss         = 0.003f / segment;
		const float widthOuter = 2000.0f * ss * 160.0f;
		const float widthInner = 1750.0f * ss * 160.0f;

		const float px = topLeft.x + 160.0f + (offset * ss * 160.0f);
		const float py = topLeft.y + 30.0f - (ss * (curve * 2.0f - 2500.0f) * 90.0f);

		bl  = { px - widthOuter, py };
		br  = { px + widthOuter, py };
		bli = { px - widthInner, py };
		bri = { px + widthInner, py };

		if (segment != 300)
		{
			const bool alternate = fmodf(trackOffset + segment, 10.0f) < 5.0f;
			drawList->AddQuadFilled(prevTopLeft, prevTopRight, br, bl, alternate ? 0xffffffff : 0xff0000ff);
			drawList->AddQuadFilled(prevInnerLeft, prevInnerRight, bri, bli, alternate ? 0xff2f2f2f : 0xff3f3f3f);
		}
	}
}

FX_REGISTER(RaceTrack)

} // namespace ui
