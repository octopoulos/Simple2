// fx-PacMan.cpp
// @author octopoulos
// @version 2025-10-21

#include "stdafx.h"
#include "ui/ui-fx.h"
//
#include "entry/input.h" // GetGlobalInput

static void Fx_PacMan(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	// 1) acquire user inputs
	auto&       ginput = GetGlobalInput();
	const auto& finger = ginput.GetMouse();

	// 2) static vars
	static ImVec2 center(topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f);
	static float  radius = size.x * 0.05f;

	// 3) start at the bottom of the mouth
	// (<
	//      270 = Pi*3/2
	//     /---\ 315 = Pi*7/4 = Pi*2 - Pi/4
	// 180 |   | 0 ~ Pi*2
	// =Pi \---/ *45 = Pi/4
	//      90 = Pi/2
	const float time4 = time * 4.0f;

	static float angleDir = 0.0f;

	// 4) draw the pacman
	{
		const float  opening = bx::kPi * (bx::sin(time4) * 0.22f + 0.22f);
		const float  angle1  = angleDir + opening;
		const float  angle2  = angleDir - opening + bx::kPi2;
		const ImVec2 start(radius * bx::cos(angle1), radius * bx::sin(angle1));

		drawList->PathLineTo(center);
		drawList->PathLineTo(center + start);
		drawList->PathArcTo(center, radius, angle1, angle2);
		drawList->PathFillConvex(IM_COL32(255, 190, 0, 255));
	}

	const ImVec2 right = ImVec2(bx::cos(angleDir), bx::sin(angleDir)) * radius;
	const ImVec2 up    = ImVec2(bx::cos(angleDir - bx::kPiHalf), bx::sin(angleDir - bx::kPiHalf)) * radius;

	// 5) draw the cheek
	{
		const ImVec2 cheek = center - right * 0.6f + up * 0.03f;
		drawList->AddCircleFilled(cheek, radius * 0.21f, IM_COL32(246, 152, 71, 255));
	}

	// 6) draw 1 eye
	{
		const ImVec2 eye = center - right * 0.35f + up * 0.3f;
		drawList->AddCircleFilled(eye, radius * 0.14f, IM_COL32(255, 255, 255, 255));

		const float  alpha = bx::sin(time4) * 0.4f + 0.4f;
		const ImVec2 pupil = eye + right * 0.05f + up * 0.05f;
		drawList->AddCircleFilled(pupil, radius * 0.065f, IM_COL32(0, 0, 0, 255));

		const ImVec2 reflex = pupil + right * 0.028f + up * 0.018f;
		drawList->AddCircleFilled(reflex, radius * 0.014f, IM_COL32(255, 255, 255, 255));
	}

	// 7) controls
	{
		using namespace entry;
		const auto& keys  = ginput.keys;
		const float speed = 5.0f;
		ImVec2      dir   = ImVec2(0.0f, 0.0f);

		if (keys[Key::Down])
		{
			center.y += speed;
			dir.y += 1.0f;
		}
		if (keys[Key::Left])
		{
			center.x -= speed;
			dir.x -= 1.0f;
		}
		if (keys[Key::Right])
		{
			center.x += speed;
			dir.x += 1.0f;
		}
		if (keys[Key::Up])
		{
			center.y -= speed;
			dir.y -= 1.0f;
		}
		if (keys[Key::Minus]) radius *= 0.99f;
		if (keys[Key::Equals]) radius *= 1.01f;
		if (keys[Key::LeftBracket]) angleDir -= 0.1f;
		if (keys[Key::RightBracket]) angleDir += 0.1f;

		if (dir.x != 0.0f || dir.y != 0.0f)
		{
			const float target = bx::atan2(dir.y, dir.x);
			angleDir = bx::angleLerp(angleDir, target, 0.1f);
		}
	}
}

FX_REGISTER(PacMan)
