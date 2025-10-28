// fx-Green.cpp
// This file defines a visual effect (animation) called "Green".
// Author: octopoulos
// Version: 2025-10-21

#include "stdafx.h"      // Common project header (basic includes and setup)
#include "ui/ui-fx.h"    // UI and drawing functions (from the ImGui graphics library)
#include "entry/input.h" // Input handling (keyboard, mouse, etc.)

// ----------------------------
// Define a " Ball" structure
// ----------------------------
// This is like a mini data container that holds everything we need
// to know about a single moving ball in our animation.
struct Ball
{
	ImVec2 accel;  // acceleration (how much the speed changes each frame)
	bool   alive;  // is the ball still active or already hit by the mouse?
	ImU32  color;  // color of the ball (32-bit RGBA value)
	ImVec2 pos;    // position on the screen (x and y)
	float  radius; // size (how big the ball is)
	ImVec2 speed;  // current speed in both directions (x and y)
};

// ----------------------------
// The main function that runs every frame
// ----------------------------
// Parameters:
// - drawList: where we draw shapes (provided by ImGui)
// - topLeft, bottomRight: corners of the drawing area
// - size: width/height of the area
// - mouse: info about mouse position
// - time: current time since start
static void Fx_Green(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	// Get access to the global input system (used to read the mouse)
	auto&       ginput = GetGlobalInput();
	const auto& finger = ginput.GetMouse(); // store the current mouse state in "finger"

	// ----------------------------
	// STEP 1: CREATE BALLS
	// ----------------------------
	// Use "static" so the data stays between frames (not erased each time)
	static std::vector<Ball> balls;            // list of all balls
	static float             endTime;          // time when the game ends
	static int               score;            // how many balls were hit
	static float             startTime = time; // time when the game started

	// If there are no balls yet, create 100 random ones
	if (balls.empty())
	{
		for (int i = 0; i < 100; ++i)
		{
			// push_back adds a new Ball object to the vector
			balls.push_back({
			    .accel  = ImVec2(0.0f, 0.0f), // no acceleration yet
			    .alive  = true,               // ball starts "alive"
			    .color  = ImColor(            // random RGB color (at least 50 R/G/B)
                    MerseneInt32(50, 255),
                    MerseneInt32(50, 255),
                    MerseneInt32(50, 255)),
			    .pos    = ImVec2( // start in the middle of the screen
                    topLeft.x + size.x * 0.5f,
                    topLeft.y + size.y * 0.5f),
			    .radius = MerseneFloat(2.0f, 25.0f), // random size
			    .speed  = ImVec2(                    // random direction and speed
                    MerseneFloat(-5.0f, 5.0f),
                    MerseneFloat(-5.0f, 5.0f)),
			});
		}
	}

	// ----------------------------
	// STEP 2: DRAW BALLS & CHECK MOUSE COLLISION
	// ----------------------------

	// The current mouse position in the window
	const ImVec2 pos(finger.abs[0], finger.abs[1]);
	// The "radius" of the mouse � how close you have to be to hit a ball
	const float  radius = 20.0f;

	// Loop through every ball
	for (auto& ball : balls)
	{
		// Choose the larger of the two radii (ball or mouse)
		const float radius2 = bx::max(radius, ball.radius);

		// Check if the mouse is touching this ball (collision detection)
		if (ball.alive && pos.x >= ball.pos.x - radius2 && pos.x <= ball.pos.x + radius2 && pos.y >= ball.pos.y - radius2 && pos.y <= ball.pos.y + radius2)
		{
			// If the mouse touches the ball:
			ball.accel = ImVec2(0.0f, 1.0f);  // start falling (gravity)
			ball.alive = false;               // mark as "dead"
			ball.color = IM_COL32(0, 255, 0, 255); // turn green

			// Add 1 point to the score
			++score;

			// If all balls are hit, record the end time
			if (score >= balls.size()) endTime = time;
		}

		// Draw the ball on screen (filled circle)
		drawList->AddCircleFilled(ball.pos, ball.radius, ball.color);
	}

	// ----------------------------
	// STEP 3: DRAW THE CURSOR
	// ----------------------------
	// Draw a ring around the mouse pointer so you can see your "hit zone"
	drawList->AddCircle(pos, 15.0f, IM_COL32(150, 30, 20, 255), 0, 5.0f);

	// ----------------------------
	// STEP 4: MOVE AND BOUNCE BALLS
	// ----------------------------
	for (auto& ball : balls)
	{
		// Update ball motion: add acceleration to speed, and speed to position
		ball.speed += ball.accel;
		ball.pos += ball.speed;

		// --- Handle bouncing from edges of the box ---
		// If the ball goes beyond the left edge, bounce right
		if (ball.pos.x - ball.radius < topLeft.x)
			ball.speed.x = bx::abs(ball.speed.x);

		// If it hits the top edge, bounce down
		if (ball.pos.y - ball.radius < topLeft.y)
			ball.speed.y = bx::abs(ball.speed.y);

		// If it hits the right edge, bounce left
		if (ball.pos.x + ball.radius > bottomRight.x)
			ball.speed.x = -bx::abs(ball.speed.x);

		// If it hits the bottom edge
		if (ball.pos.y + ball.radius > bottomRight.y)
		{
			// Bounce upward
			ball.speed.y = -bx::abs(ball.speed.y);

			// Dead balls bounce weaker (slower)
			if (!ball.alive)
				ball.speed *= 0.7f;
		}
	}

	// ----------------------------
	// STEP 5: DISPLAY SCORE AND TIME
	// ----------------------------
	// Format the text to show remaining balls and elapsed time
	const char* text = Format(
	    "Left: %d  Time: %.2f",
	    TO_INT(balls.size()) - score,                   // balls left
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
	    text                     // actual text to draw
	);
}

// This macro registers the effect so that the main UI system can find and use it.
FX_REGISTER(Green)
