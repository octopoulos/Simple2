// fx-Basket.cpp
// @author octopoulos
// @version 2025-11-05

#include "stdafx.h"
#include "ui/ui-fx.h"
//
#include "entry/input.h" // GetGlobalInput

struct Ball
{
	ImVec2 accel  = ImVec2(0.0f, 0.0f);           // acceleration (how much the speed changes each frame)
	bool   alive  = true;                         // is the ball still active or already hit by the mouse?
	ImU32  color  = IM_COL32(255, 255, 255, 255); // color of the ball (32-bit RGBA value)
	ImVec2 pos    = ImVec2(0.0f, 0.0f);           // position on the screen (x and y)
	ImVec2 pos0   = ImVec2(0.0f, 0.0f);           // previous position
	float  radius = 10.0f;                        // size (how big the ball is)
	bool   scored = false;                        // ball has entered the net
	ImVec2 speed  = ImVec2(0.0f, 0.0f);           // current speed in both directions (x and y)
};

struct Basket
{
	float  height = 0.0f;               // height of the net
	ImVec2 pos    = ImVec2(0.0f, 0.0f); // center position
	ImVec2 radius = ImVec2(0.0f, 0.0f); // radius of the ellipse (rx, ry)
};

static void Fx_Basket(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	// 1) get inputs
	auto&       ginput = GetGlobalInput();
	const auto& finger = ginput.GetMouse();

	// 2) initialization
	const ImVec2 center(topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f);
	const float  radius = size.x * 0.03f;

	static std::vector<Ball> balls;             //
	static Basket            basket;            //
	static float             endTime   = 0.0f;  // time when the game ends
	static ImVec2            lastSize  = size;  //
	static float             lastTime  = -1.0f; //
	static bool              needInit  = true;  //
	static int               score     = 0;     //
	static float             startTime = time;  // time when the game started

	if (size != lastSize) needInit = true;

	if (needInit)
	{
		needInit = false;
		lastSize = size;

		balls.clear();
		balls.reserve(3);
		for (int i = 0; i < 3; ++i)
		{
			balls.push_back({
				.color = ImColor(
					MerseneInt32(50, 255),
					MerseneInt32(50, 255),
					MerseneInt32(50, 255),
					192
				),
				.pos    = ImVec2(topLeft.x + MerseneFloat(radius, radius * 3), center.y),
				.radius = radius * MerseneFloat(0.95f, 1.05f),
			});
		}

		basket.height = radius * 2.5f;
		basket.pos    = ImVec2(bottomRight.x - MerseneFloat(radius, radius * 4), center.y);
		basket.radius = ImVec2(radius * 1.8f, radius * 1.1f);
	}

	// 3) elapsed time
	const float delta = (lastTime < 0.0f) ? 0.016f : time - lastTime;
	lastTime = time;

	const ImVec2 mousePos = ImVec2(finger.abs[0], finger.abs[1]);

	// 4) ball physics
	for (auto& ball : balls)
	{
		ball.pos0 = ball.pos;
		ball.speed += ball.accel;
		ball.pos += ball.speed;

		bool isHit = false;

		if (ball.pos.x - ball.radius < topLeft.x)
		{
			ball.speed.x = bx::abs(ball.speed.x);
			isHit = true;
		}

		if (ball.pos.y - ball.radius < topLeft.y)
		{
			ball.speed.y = bx::abs(ball.speed.y);
			isHit = true;
		}

		if (ball.pos.x + ball.radius > bottomRight.x)
		{
			ball.speed.x = -bx::abs(ball.speed.x);
			isHit = true;
		}

		if (ball.pos.y + ball.radius > bottomRight.y)
		{
			ball.speed.y = -bx::abs(ball.speed.y);
			isHit = true;
		}

		if (isHit) ball.pos += ball.speed;

		// ball enters basket?
		if (!ball.scored && ball.speed.y > 0 && ball.pos0.y <= basket.pos.y && ball.pos.y > basket.pos.y)
		{
			if (ball.pos.x - ball.radius >= basket.pos.x - basket.radius.x && ball.pos.x + ball.radius <= basket.pos.x + basket.radius.x)
			{
				ball.scored  = true;
				ball.speed.x *= 0.1f;
				ball.speed.y *= 0.1f;
				++score;
			}
		}

		// gravity + air resistance
		ball.accel = ImVec2(0.0f, 0.981f);
		ball.accel -= ball.speed * 0.015f * MerseneFloat(0.95f, 1.05f);

		if (!ball.scored && ginput.buttonOnes[1])
			ball.accel = (mousePos - ball.pos) * 3.2f * delta;
	}

	// 5) draw everything
	{
		for (const auto& ball : balls)
		{
			drawList->AddCircleFilled(ball.pos, ball.radius, ball.color);

			// draw the line between ball and mouse
			if (!ball.scored) drawList->AddLine(ball.pos, mousePos, IM_COL32(255, 160, 0, 128));
		}

		// draw the basket
		{
			drawList->AddEllipse(basket.pos, basket.radius, IM_COL32(220, 220, 220, 255));
			drawList->AddLine(ImVec2(basket.pos.x - basket.radius.x, basket.pos.y), ImVec2(basket.pos.x - basket.radius.x * 0.5f, basket.pos.y + basket.height), IM_COL32(220, 220, 220, 255));
			drawList->AddLine(ImVec2(basket.pos.x + basket.radius.x, basket.pos.y), ImVec2(basket.pos.x + basket.radius.x * 0.5f, basket.pos.y + basket.height), IM_COL32(220, 220, 220, 255));
		}
	}

	// 6) time + score
	const char* text  = Format("Score: %d  Time: %.2f", score, ((endTime > 0.0f) ? endTime : time) - startTime);
	const float width = ImGui::CalcTextSize(text).x;
	drawList->AddText(ImVec2(topLeft.x + size.x * 0.5f - width * 0.5f, topLeft.y + size.y * 0.01f), IM_COL32(255, 255, 255, 255), text);
}

FX_REGISTER(Basket)
