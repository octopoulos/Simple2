// fx-Boids.cpp
// @author octopoulos
// @version 2025-10-18

#include "stdafx.h"
#include "ui/ui-fx.h"

static void Fx_Boids(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	struct Boid
	{
		ImVec2 pos;
		ImVec2 speed;
		ImVec2 accel;
		int    leader;
	};

	static Boid boids[50];
	static bool initialized = false;

	auto Length = [](ImVec2 v) {
		return sqrtf(v.x * v.x + v.y * v.y);
	};
	auto Normalize = [&](ImVec2 v) {
		float l = Length(v);
		return l > 0 ? ImVec2 { v.x / l, v.y / l } : ImVec2 { 0, 0 };
	};
	auto Limit = [&](ImVec2 v, float max) {
		float l = Length(v);
		return l > max ? ImVec2 { v.x / l * max, v.y / l * max } : v;
	};
	auto Rotate = [](ImVec2 v, float angle) {
		return ImVec2 { v.x * bx::cos(angle) - v.y * bx::sin(angle), v.x * bx::sin(angle) + v.y * bx::cos(angle) };
	};

	if (!initialized)
	{
		for (int i = 0; i < 50; ++i)
		{
			boids[i].pos = ImVec2{
				topLeft.x + TO_FLOAT(rand() % TO_INT(bottomRight.x - topLeft.x)),
				topLeft.y + TO_FLOAT(rand() % TO_INT(bottomRight.y - topLeft.y))
			};
			float angle     = TO_FLOAT(rand() % 360);
			boids[i].speed  = ImVec2 { bx::cos(angle), bx::sin(angle) };
			boids[i].leader = (i % 20 == 0);
		}
		initialized = true;
	}

	for (int i = 0; i < 50; ++i)
	{
		Boid&  b = boids[i];
		ImVec2 align { 0, 0 }, cohesion { 0, 0 }, separation { 0, 0 }, avoidLeader { 0, 0 };
		int    countAlign = 0, countCohesion = 0, countSeparation = 0, countAvoid = 0;
		int    radius = b.leader ? 60 : 20;

		for (int j = 0; j < 50; ++j)
		{
			Boid&  o = boids[j];
			ImVec2 diff { o.pos.x - b.pos.x, o.pos.y - b.pos.y };
			float  dist = Length(diff);
			if (dist > 0 && dist < radius)
			{
				align = ImVec2 { align.x + o.speed.x, align.y + o.speed.y };
				++countAlign;
				cohesion = ImVec2 { cohesion.x + o.pos.x, cohesion.y + o.pos.y };
				++countCohesion;
			}
			if (dist > 0 && dist < 10)
			{
				separation = ImVec2 { separation.x + (b.pos.x - o.pos.x) / dist / dist, separation.y + (b.pos.y - o.pos.y) / dist / dist };
				++countSeparation;
			}
			if (!b.leader && o.leader)
			{
				if (dist > 0 && Length(ImVec2 { b.pos.x + b.speed.x - o.pos.x, b.pos.y + b.speed.y - o.pos.y }) < 40)
				{
					avoidLeader = ImVec2 { avoidLeader.x + (b.pos.x - o.pos.x) / dist * 3, avoidLeader.y + (b.pos.y - o.pos.y) / dist * 3 };
					++countAvoid;
				}
			}
		}

		if (countAlign) align = Normalize(ImVec2 { align.x / countAlign, align.y / countAlign }) * 2.0f - b.speed;
		if (countCohesion) cohesion = Normalize(ImVec2 { cohesion.x / countCohesion - b.pos.x, cohesion.y / countCohesion - b.pos.y }) * 2.0f - b.speed;
		if (countSeparation) separation = Normalize(ImVec2 { separation.x / countSeparation * 2.0f, separation.y / countSeparation * 2.0f });
		if (countAvoid) avoidLeader = Normalize(ImVec2 { avoidLeader.x / countAvoid, avoidLeader.y / countAvoid }) - b.speed;

		b.accel = ImVec2 { separation.x + align.x + cohesion.x + avoidLeader.x, separation.y + align.y + cohesion.y + avoidLeader.y };
		b.speed = Limit(ImVec2 { b.speed.x + b.accel.x, b.speed.y + b.accel.y }, 2.0f);
		b.pos   = ImVec2 { b.pos.x + b.speed.x, b.pos.y + b.speed.y };

		if (b.pos.x < topLeft.x - 2) b.pos.x = bottomRight.x + 2;
		if (b.pos.y < topLeft.y - 2) b.pos.y = bottomRight.y + 2;
		if (b.pos.x > bottomRight.x + 2) b.pos.x = topLeft.x - 2;
		if (b.pos.y > bottomRight.y + 2) b.pos.y = topLeft.y - 2;

		float angle = atan2f(b.speed.x, -b.speed.y);
		drawList->AddLine(b.pos + Rotate(ImVec2 { 0, -4 }, angle), b.pos + Rotate(ImVec2 { 2, 4 }, angle), b.leader ? 0xff0000ff : ~0);
	}
}

FX_REGISTER(Boids)
