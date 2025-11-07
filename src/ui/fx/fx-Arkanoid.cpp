// fx-Arkanoid.cpp
// @author octopoulos
// @version 2025-10-28

#include "stdafx.h"
#include "ui/ui-fx.h"

static void Fx_Arkanoid(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	struct Block
	{
		ImU32  color;
		ImVec2 min;
		ImVec2 max;
		bool   active = true;

		bool Contains(const ImVec2& p) const
		{
			return (p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y);
		}
	};

	// persistent state
	static Block  blocks[60] = {};
	static float  ballRadius = 3.0f;
	static float  blockH     = 0.0f;
	static float  blockW     = 0.0f;
	static float  cx         = 0.0f;
	static float  cy         = 0.0f; // ball center (px)
	static ImVec2 lastSize   = ImVec2(0.0f, 0.0f);
	static float  lastTime   = 0.0f;
	static int    needInit   = 3;
	static Block  paddle     = {};
	static float  paddleH    = 5.0f;
	static float  paddleW    = 40.0f;
	static float  vx         = 0.0f;
	static float  vy         = 0.0f; // ball velocity (px/sec)

	// time
	static float endTime;          // time when the game ends
	static float startTime = time; // time when the game started

	// reset when user requests (mouse.w == 0) or when size changes
	if (!mouse.w) needInit = 3;
	if (lastSize.x != size.x || lastSize.y != size.y) needInit = 3;

	// initialize layout and state
	if (needInit)
	{
		const int cols = 10;
		const int rows = 6;

		// layout derived from original 320x180 proportions
		blockW  = size.x / TO_FLOAT(cols);       // 10 columns
		blockH  = size.y / 12.0f;                // original used H/12 for block height
		paddleW = size.x * (60.0f / 320.0f);     // scale 40px => proportion of width
		paddleH = bx::max(4.0f, size.y * 0.03f); // small height scaled by size

		ballRadius = bx::max(2.0f, size.x * 0.01f);

		// ball start near bottom like original (a.y + H - 8)
		cx = topLeft.x + size.x * 0.5f;
		cy = topLeft.y + size.y * (1.0f - 8.0f / 180.0f);

		// velocities in px/sec (scaled to canvas size)
		const float vx_base = MerseneFloat(-120.0f, 120.0f); // px/sec on 320 width baseline
		const float vy_base = -120.0f;                       // px/sec on 180 height baseline
		const float sx      = size.x / 320.0f;
		const float sy      = size.y / 180.0f;
		vx                  = vx_base * sx;
		vy                  = vy_base * sy;

		// fill blocks grid (10x6)
		if (needInit & 2)
		{
			for (int r = 0; r < rows; ++r)
			{
				for (int c = 0; c < cols; ++c)
				{
					Block& b = blocks[c + r * cols];
					b.active = true;
					b.color  = ImColor(MerseneInt32(50, 240), MerseneInt32(50, 240), MerseneInt32(50, 240));
					b.min    = ImVec2(topLeft.x + c * blockW, topLeft.y + r * blockH);
					b.max    = ImVec2(topLeft.x + (c + 1) * blockW, topLeft.y + (r + 1) * blockH);
				}
			}
		}

		lastTime = time;
		lastSize = size;
		needInit = 0;
	}

	// time step
	const float dt = bx::clamp(time - lastTime, 0.0f, 0.05f);
	lastTime       = time;

	// paddle follows mouse.x (mouse.x is expected normalized 0..1 within the region)
	paddle.min.x = topLeft.x + mouse.x * size.x - paddleW * 0.5f;
	paddle.min.x = bx::clamp(paddle.min.x, topLeft.x, bottomRight.x - paddleW);
	paddle.min.y = bottomRight.y - paddleH;
	paddle.max.x = paddle.min.x + paddleW;
	paddle.max.y = bottomRight.y;

	// --- collisions with blocks (check current ball center) ---
	bool isHit = false;
	for (int i = 0; i < 60; ++i)
	{
		Block& b = blocks[i];
		if (!b.active) continue;
		if (!b.Contains(ImVec2(cx, cy))) continue;

		b.active = false;
		isHit    = true;

		// compute overlaps (how deep is the center inside each side)
		const float ol  = cx - b.min.x;
		const float or_ = b.max.x - cx;
		const float ot  = cy - b.min.y;
		const float ob  = b.max.y - cy;
		const float ox  = bx::min(ol, or_);
		const float oy  = bx::min(ot, ob);

		// decide axis of collision by smallest overlap
		if (ox < oy)
		{
			vx = -vx;
			// push ball slightly outside to avoid sticking
			if (ol < or_)
				cx = b.min.x - ballRadius;
			else
				cx = b.max.x + ballRadius;
		}
		else
		{
			vy = -vy;
			if (ot < ob)
				cy = b.min.y - ballRadius;
			else
				cy = b.max.y + ballRadius;
		}
	}

	// paddle collision (if ball center is inside paddle rect)
	if (paddle.Contains(ImVec2(cx, cy)))
	{
		const ImVec2 padCenter = (paddle.min + paddle.max) * 0.5f;
		const float  deltaX    = cx - padCenter.x;

		vx += deltaX * 2.5f;
		vy = -std::fabs(vy); // reflect upward
		// nudge ball above paddle
		cy = paddle.min.y - ballRadius - 1.0f;
	}

	// draw blocks (outline + fill)
	int numBlock = 0;
	for (int i = 0; i < 60; ++i)
	{
		const Block& b = blocks[i];
		if (b.active)
		{
			++numBlock;
			drawList->AddRectFilled(b.min, b.max, b.color);
			drawList->AddRect(b.min, b.max, IM_COL32(255, 255, 255, 120));
		}
	}

	// If all blocks are hit, record the end time
	if (isHit && numBlock == 0) endTime = time;

	// draw the paddle
	drawList->AddRectFilled(paddle.min, paddle.max, IM_COL32(200, 200, 200, 255));
	drawList->AddRect(paddle.min, paddle.max, IM_COL32(255, 255, 255, 120));

	// draw the ball
	drawList->AddCircleFilled(ImVec2(cx, cy), ballRadius, IM_COL32(255, 255, 255, 255));

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
		needInit = 1;

	// ----------------------------
	// STEP 5: DISPLAY SCORE AND TIME
	// ----------------------------
	// Format the text to show remaining balls and elapsed time
	const char* text = Format(
	    "Left: %d  Time: %.2f",
	    numBlock,                                       // blocks left
	    ((endTime > 0.0f) ? endTime : time) - startTime // if game ended, freeze timer
	);

	// Center the text horizontally
	const float width = ImGui::CalcTextSize(text).x;
	drawList->AddText(
	    ImVec2(
	        topLeft.x + size.x * 0.5f - width * 0.5f, // X position (center)
	        topLeft.y + size.y * 0.01f                // Y position (top)
	        ),
	    IM_COL32(255, 255, 255, 255), // white color
	    text                          // actual text to draw
	);
}

FX_REGISTER(Arkanoid)
