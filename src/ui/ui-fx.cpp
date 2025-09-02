// ui-fx.cpp
// @author octopoulos
// @version 2025-08-28
//
// https://github.com/ocornut/imgui/issues/3606

#include "stdafx.h"
#include "ui/ui.h"
//
#include "imgui.h"          // ImGui::
#include "imgui_internal.h" // IM_PI

namespace ui
{

void Fx_Arkanoid(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	struct Block
	{
		ImVec2 min;
		ImVec2 max;
		bool   active = true;

		bool Contains(const ImVec2& p) const
		{
			return (p.x > min.x && p.x < max.x) && (p.y > min.y && p.y < max.y);
		}
	};

	// persistent state
	static bool   needInit = true;
	static ImVec2 lastSize = ImVec2(0.0f, 0.0f);
	static float  cx = 0.0f, cy = 0.0f; // ball center (px)
	static float  vx = 0.0f, vy = 0.0f; // ball velocity (px/sec)
	static float  lastTime   = 0.0f;
	static float  ballRadius = 3.0f;
	static float  blockW = 0.0f, blockH = 0.0f;
	static float  paddleW = 40.0f, paddleH = 5.0f;
	static Block  blocks[60];
	static Block  paddle;

	// reset when user requests (mouse.w == 0) or when size changes
	if (!mouse.w) needInit = true;
	if (lastSize.x != size.x || lastSize.y != size.y) needInit = true;

	// initialize layout and state
	if (needInit)
	{
		const int cols = 10;
		const int rows = 6;

		// layout derived from original 320x180 proportions
		blockW  = size.x / static_cast<float>(cols); // 10 columns
		blockH  = size.y / 12.0f;                    // original used H/12 for block height
		paddleW = size.x * (40.0f / 320.0f);         // scale 40px => proportion of width
		paddleH = std::max(4.0f, size.y * 0.03f);    // small height scaled by size

		ballRadius = std::max(2.0f, size.x * 0.01f);

		// ball start near bottom like original (a.y + H - 8)
		cx = topLeft.x + size.x * 0.5f;
		cy = topLeft.y + size.y * (1.0f - 8.0f / 180.0f);

		// velocities in px/sec (scaled to canvas size)
		const float vx_base = -120.0f; // px/sec on 320 width baseline
		const float vy_base = -180.0f; // px/sec on 180 height baseline
		const float sx      = size.x / 320.0f;
		const float sy      = size.y / 180.0f;
		vx                  = vx_base * sx;
		vy                  = vy_base * sy;

		// fill blocks grid (10x6)
		for (int r = 0; r < rows; ++r)
		{
			for (int c = 0; c < cols; ++c)
			{
				Block& b = blocks[c + r * cols];
				b.active = true;
				b.min    = ImVec2(topLeft.x + c * blockW, topLeft.y + r * blockH);
				b.max    = ImVec2(topLeft.x + (c + 1) * blockW, topLeft.y + (r + 1) * blockH);
			}
		}

		lastTime = time;
		lastSize = size;
		needInit = false;
	}

	// time step
	float dt = time - lastTime;
	if (dt < 0.0f) dt = 0.0f;
	// clamp dt for stability (avoid huge jumps)
	if (dt > 0.05f) dt = 0.05f;
	lastTime = time;

	// paddle follows mouse.x (mouse.x is expected normalized 0..1 within the region)
	paddle.min.x = topLeft.x + mouse.x * size.x - paddleW * 0.5f;
	paddle.min.x = std::clamp(paddle.min.x, topLeft.x, bottomRight.x - paddleW);
	paddle.min.y = bottomRight.y - paddleH;
	paddle.max.x = paddle.min.x + paddleW;
	paddle.max.y = bottomRight.y;

	// --- collisions with blocks (check current ball center) ---
	for (int i = 0; i < 60; ++i)
	{
		Block& b = blocks[i];
		if (!b.active) continue;
		if (!b.Contains(ImVec2(cx, cy))) continue;

		b.active = false;

		// compute overlaps (how deep is the center inside each side)
		const float ol  = cx - b.min.x;
		const float or_ = b.max.x - cx;
		const float ot  = cy - b.min.y;
		const float ob  = b.max.y - cy;
		const float ox  = std::min(ol, or_);
		const float oy  = std::min(ot, ob);

		// decide axis of collision by smallest overlap
		if (ox < oy)
		{
			vx = -vx;
			// push ball slightly outside to avoid sticking
			if (ol < or_) cx = b.min.x - ballRadius;
			else cx = b.max.x + ballRadius;
		}
		else
		{
			vy = -vy;
			if (ot < ob) cy = b.min.y - ballRadius;
			else cy = b.max.y + ballRadius;
		}
	}

	// paddle collision (if ball center is inside paddle rect)
	if (paddle.Contains(ImVec2(cx, cy)))
	{
		vy = -std::fabs(vy); // reflect upward
		// nudge ball above paddle
		cy = paddle.min.y - ballRadius - 1.0f;
	}

	// draw blocks (outline + fill) and paddle/ball
	for (int i = 0; i < 60; ++i)
	{
		const Block& b = blocks[i];
		if (!b.active) continue;
		drawList->AddRectFilled(b.min, b.max, IM_COL32(80, 180, 100, 255));
		drawList->AddRect(b.min, b.max, IM_COL32(255, 255, 255, 120));
	}

	drawList->AddRect(paddle.min, paddle.max, IM_COL32(255, 255, 255, 255));
	drawList->AddCircle(ImVec2(cx, cy), ballRadius, IM_COL32(255, 255, 255, 255));

	// --- integrate position (pixels) ---
	cx += vx * dt;
	cy += vy * dt;

	// bounce on vertical walls (account for radius)
	if (cx - ballRadius < topLeft.x)
	{
		cx = topLeft.x + ballRadius;
		vx = -vx;
	}
	if (cx + ballRadius > bottomRight.x)
	{
		cx = bottomRight.x - ballRadius;
		vx = -vx;
	}

	// bounce on top
	if (cy - ballRadius < topLeft.y)
	{
		cy = topLeft.y + ballRadius;
		vy = -vy;
	}

	// ball falls below bottom => reset to initial state
	if (cy - ballRadius > bottomRight.y)
		needInit = true;
}

void Fx_Boxes(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 /*mouse*/, float time)
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

void Fx_Eyes(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	struct EllipseHelper
	{
		static void Draw(ImDrawList* drawList, ImVec2 center, float rx, float ry, ImU32 color)
		{
			drawList->PathClear();
			for (int step = 0; step < 36; ++step)
			{
				float angle = IM_PI / 18.0f * step;
				drawList->PathLineTo(ImVec2(center.x + sinf(angle) * rx, center.y + cosf(angle) * ry));
			}
			drawList->PathFillConvex(color);
		}
	};

	// mouse in local coordinates
	ImVec2 mousePos(topLeft.x + mouse.x * size.x, topLeft.y + mouse.y * size.y);

	const float virtualX = 320.0f;
	const float virtualY = 180.0f;

	ImVec2 scale(size.x / virtualX, size.y / virtualY);

	auto Scale = [&](ImVec2 pos) {
		return ImVec2(topLeft.x + pos.x * scale.x, topLeft.y + pos.y * scale.y);
	};

	ImVec2 center = Scale(ImVec2(virtualX * 0.5f, virtualY * 0.5f));

	// background
	drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(102, 85, 68, 255));

	ImVec2 eyes[2] = {
		Scale(ImVec2(virtualX * 0.5f - 50.0f, virtualY * 0.5f)),
		Scale(ImVec2(virtualX * 0.5f + 50.0f, virtualY * 0.5f))
	};

	for (int ei = 0; ei < 2; ++ei)
	{
		// outer sclera (black) and inner (white)
		EllipseHelper::Draw(drawList, eyes[ei], 40.0f * scale.x, 70.0f * scale.y, IM_COL32(0, 0, 0, 255));
		EllipseHelper::Draw(drawList, eyes[ei], 35.0f * scale.x, 65.0f * scale.y, IM_COL32(255, 255, 255, 255));

		// vector to mouse
		ImVec2 vecMouse(mousePos.x - eyes[ei].x, mousePos.y - eyes[ei].y);
		float  distMouse = sqrtf(vecMouse.x * vecMouse.x + vecMouse.y * vecMouse.y);
		if (distMouse > 1e-5f)
		{
			vecMouse.x /= distMouse;
			vecMouse.y /= distMouse;
		}
		if (distMouse > 20.0f) distMouse = 20.0f;

		ImVec2 pupilPos(
		    eyes[ei].x + vecMouse.x * distMouse * scale.x,
		    eyes[ei].y + vecMouse.y * distMouse * scale.y);

		// Draw pupil
		EllipseHelper::Draw(drawList, pupilPos, 10.0f * scale.x, 10.0f * scale.y, IM_COL32(0, 0, 0, 255));
	}
}

void Fx_Fire(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4, float time)
{
	// virtual canvas
	const int virtualX = 160;
	const int virtualY = 90;

	// fire palette
	static const int fireColors[666] = {
		0xFFFFFF, 0xC7EFEF, 0x9FDFDF, 0x6FCFCF, 0x37B7B7, 0x2FB7B7, 0x2FAFB7, 0x2FAFBF,
		0x27A7BF, 0x27A7BF, 0x1F9FBF, 0x1F9FBF, 0x1F97C7, 0x178FC7, 0x1787C7, 0x1787CF,
		0xF7FCF, 0xF77CF, 0xF6FCF, 0xF67D7, 0x75FD7, 0x75FD7, 0x757DF, 0x757DF,
		0x74FDF, 0x747C7, 0x747BF, 0x73FAF, 0x72F9F, 0x7278F, 0x71F77, 0x71F67,
		0x71757, 0x70F47, 0x70F2F, 0x7071F, 0x70707
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

void Fx_Loading(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4, float time)
{
	static float lastTriggerTime = 0.0f;

	// every 2 seconds, advance trigger
	if (time > lastTriggerTime + 2.0f)
		lastTriggerTime += 2.0f;

	// compute color at current phase
	auto makeColor = [](float x) {
		const int r = TO_INT((std::sin(x) + 1.0f) * 255.0f / 2.0f);
		const int g = TO_INT((std::cos(x) + 1.0f) * 255.0f / 2.0f);
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
		const float elapsed  = std::max(time - lastTriggerTime - rowDelay, 0.0f);

		if (time - lastTriggerTime < rowDelay + 1.0f)
		{
			float progress = 0.0f;
			// ease-in quart
			if (elapsed < 0.5f)
				progress = 8.0f * elapsed * elapsed * elapsed * elapsed;
			// ease-out quart
			else
				progress = bx::min(1.0f - std::pow(-2.0f * elapsed + 2.0f, 4.0f) / 2.0f, 1.0f);

			barRightX = topLeft.x + progress * size.x;
		}

		const float y0 = topLeft.y + rowHeight * i;
		const float y1 = y0 + rowHeight;
		drawList->AddRectFilled(ImVec2(topLeft.x, y0), ImVec2(barRightX, y1), barColor);
	}
}

void Fx_Mosaic(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	auto frc = [](float x) {
		return x - floorf(x);
	};
	auto hash = [](int x) {
		srand(x);
		return ImLerp(rand() / 32767.0f, rand() / 32767.0f, 0.5f);
	};
	auto noise = [&](float x) {
		return ImLerp(hash((int)x), hash((int)x + 1), frc(x));
	};

	// normalize to a 160×90 "grid", regardless of size
	const int cols = 160;
	const int rows = 90;
	for (int gx = 0; gx < cols; ++gx)
	{
		for (int gy = 0; gy < rows; ++gy)
		{
			float rx = gx - cols / 2.0f;
			float ry = gy - rows / 2.0f;

			float an = atan2f(rx, ry);
			if (an < 0.0f)
				an += IM_PI * 2.0f;

			float len = (rx * rx + ry * ry + 0.1f) + time * 4.0f;
			float n0  = noise(an);
			float n1  = noise(len);
			float al  = n0 + n1;

			// map grid cell to pixel position in given size
			float px = topLeft.x + (gx / float(cols)) * size.x * 2.0f;
			float py = topLeft.y + (gy / float(rows)) * size.y * 2.0f;

			drawList->AddRectFilled(
			    ImVec2(px, py),
			    ImVec2(px + 2, py + 2),
			    ImColor(1.0f, 1.0f, 1.0f, al)
			);
		}
	}
}

void Fx_RaceTrack(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4, float time)
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

void Fx_RainDrops(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
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

void Fx_Tunnel(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4, float time)
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

// clang-format off
static std::vector<std::pair<std::string, FxFunc>> fxFunctions = {
	{ "Arkanoid" , Fx_Arkanoid  },
	{ "Boxes"    , Fx_Boxes     },
	{ "Circles"  , Fx_Circles   },
	{ "Eyes"     , Fx_Eyes      },
	{ "Fire"     , Fx_Fire      },
	{ "Loading"  , Fx_Loading   },
	{ "Mosaic"   , Fx_Mosaic    },
	{ "RaceTrack", Fx_RaceTrack },
	{ "RainDrops", Fx_RainDrops },
	{ "Tunnel"   , Fx_Tunnel    },
};
// clang-format on

std::vector<std::pair<std::string, FxFunc>> GetFxFunctions()
{
	return fxFunctions;
}

} // namespace ui
