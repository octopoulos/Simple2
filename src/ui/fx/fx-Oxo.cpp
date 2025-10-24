// fx-Oxo.cpp
// @author octopoulos
// @version 2025-10-19

#include "stdafx.h"
#include "ui/ui-fx.h"
//
#include "entry/input.h" // GetGlobalInput

#include "bx/easing.h" // easeInOutSine

// Check if there's a winner
static int CheckWinner(int matrix[3][3], int id, int winLine[3])
{
	// 3 horizontals
	for (int y = 0; y < 3; ++y)
	{
		if (matrix[y][0] == id && matrix[y][1] == id && matrix[y][2] == id)
		{
			winLine[0] = y * 3;
			winLine[1] = y * 3 + 1;
			winLine[2] = y * 3 + 2;
			return id;
		}
	}

	// 3 verticals
	for (int x = 0; x < 3; ++x)
	{
		if (matrix[0][x] == id && matrix[1][x] == id && matrix[2][x] == id)
		{
			winLine[0] = 0 + x;
			winLine[1] = 3 + x;
			winLine[2] = 6 + x;
			return id;
		}
	}

	// 2 diagonals
	if (matrix[0][0] == id && matrix[1][1] == id && matrix[2][2] == id)
	{
		winLine[0] = 0;
		winLine[1] = 4;
		winLine[2] = 8;
		return id;
	}
	if (matrix[0][2] == id && matrix[1][1] == id && matrix[2][0] == id)
	{
		winLine[0] = 2;
		winLine[1] = 4;
		winLine[2] = 6;
		return id;
	}

	// no winner, maybe full board?
	int count = 0;
	for (int y = 0; y < 3; ++y)
		for (int x = 0; x < 3; ++x)
			if (matrix[y][x]) ++count;

	winLine[0] = -1;
	return (count < 9) ? 0 : 3;
};

// Draw X/O
static void DrawSign(ImDrawList* drawList, const ImVec2& pos, int sign, float radius, float thickness) {
	switch (sign)
	{
	// X
	case 1:
		drawList->AddLine(ImVec2(pos.x - radius, pos.y - radius), ImVec2(pos.x + radius, pos.y + radius), ImColor(0, 0, 127), thickness);
		drawList->AddLine(ImVec2(pos.x - radius, pos.y + radius), ImVec2(pos.x + radius, pos.y - radius), ImColor(0, 0, 127), thickness);
		break;
	// O
	case 2:
		drawList->AddCircle(pos, radius, ImColor(127, 0, 0), 0, thickness);
		break;
	}
};

static void Fx_Oxo(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	// 1) persistent state
	static int   aiCoord      = -1;
	static float aiClickTime  = -1.0f;
	static int   lastCoord    = -1;
	static int   matrix[3][3] = {};
	static int   numPlayer    = 1;
	static int   playerId     = 1;
	static float restartTime  = -1.0f;
	static int   scores[4]    = {};
	static int   winLine[3]   = { -1, 0, 0 };

	if (restartTime > 0.0f && time > restartTime)
	{
		memset(matrix, 0, sizeof(matrix));
		aiCoord     = -1;
		lastCoord   = -1;
		restartTime = -1.0f;
	}

	// 2) get inputs
	auto&       ginput = GetGlobalInput();
	const auto& finger = ginput.GetMouse();

	// keyboard
	{
		using namespace entry;

		const auto& downs    = ginput.keyDowns;
		const auto& ignores  = ginput.keyIgnores;
		const int   modifier = ginput.IsModifier();

		if (!modifier)
		{
			if (downs[Key::Key1])
			{
				ui::Log("KEY1!!!!!!!!!!!!!!!!!!!!!!");
				numPlayer   = 1;
				restartTime = time;
			}
			if (downs[Key::Key2])
			{
				ui::Log("KEY2!!!!!!!!!!!!!!!!!!!!!!");
				numPlayer   = 2;
				restartTime = time;
			}
			if (downs[Key::Key0])
			{
				ui::Log("KEY0!!!!!!!!!!!!!!!!!!!!!!");
				numPlayer   = 0;
				restartTime = time;
			}
		}
	}

	// 3) coords + sizes
	const ImVec2 center(topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f);
	const float  radius    = size.x * 0.12f;
	const float  thickness = size.x * 0.01f;
	const ImVec2 vecRadius = ImVec2(radius, radius);
	const float  stride    = radius * 2.0f + size.x * 0.01f;
	const ImVec2 mousePos  = ImVec2(finger.abs[0], finger.abs[1]);

	// 4) AI
	const bool  isPlayer     = (numPlayer == 2 || (numPlayer == 1 && playerId == 1));
	const float restartDelay = 1.0f;

	if (restartTime < 0.0f && !isPlayer)
	{
		if (aiCoord < 0 || matrix[aiCoord / 3][aiCoord % 3])
		{
			if (!matrix[1][1] && (MerseneInt32() & 3))
				aiCoord = 4;
			else
			{
				do
				{
					aiCoord = MerseneInt32() % 9;
				}
				while (matrix[aiCoord / 3][aiCoord % 3]);
			}
			const float clickDelay = numPlayer ? MerseneFloat(0.5f, 1.5f) : MerseneFloat(0.1f, 0.2f);
			aiClickTime = time + clickDelay;
		}
	}

	// 5) draw board
	// top left square
	const ImVec2 pos0 = ImVec2(center.x - stride, center.y - stride);

	auto CoordVec2 = [&](int x, int y) -> ImVec2 {
		return ImVec2(pos0.x + x * stride, pos0.y + y * stride);
	};

	for (int j = 0; j < 3; ++j)
	{
		for (int i = 0; i < 3; ++i)
		{
			const int    coord = j * 3 + i;
			const ImVec2 pos   = CoordVec2(i, j);

			// 5.a) draw square + maybe sign animation
			{
				const bool contain = isPlayer
					? (mousePos.x >= pos.x - radius && mousePos.x <= pos.x + radius && mousePos.y >= pos.y - radius && mousePos.y <= pos.y + radius)
					: (coord == aiCoord);

				// draw square
				{
					const ImU32 color = (contain && restartTime < 0.0f) ? ImColor(240, 210, 180) : ImColor(200, 180, 160);
					drawList->AddRectFilled(pos - vecRadius, pos + vecRadius, color);
				}

				// draw sign animation
				if (contain && matrix[j][i] == 0 && restartTime < 0.0f)
				{
					const bool isClick = isPlayer
						? (ginput.buttonClicks[1] > 0)
						: (time > aiClickTime);

					if (isClick)
					{
						matrix[j][i] = playerId;
						lastCoord    = coord;

						// winner? => restart in 1 sec
						if (const int winner = CheckWinner(matrix, playerId, winLine); winner > 0)
						{
							++scores[winner];
							restartTime = time + restartDelay;
						}
						playerId = 3 - playerId;

						if (isPlayer) ginput.buttonClicks[1] = 0;
					}
					else DrawSign(drawList, pos, playerId, radius * (0.5f + 0.2f * bx::sin(time * 4.0f)), thickness);
				}
			}

			// 5.b) draw placed X/O
			DrawSign(drawList, pos, matrix[j][i], radius * 0.9f, thickness);
		}
	}

	// 6) draw winning line
	// - start from last placed coord
	if (restartTime > 0.0f && winLine[0] >= 0)
	{
		const int    from     = (lastCoord == winLine[0]) ? 0 : ((lastCoord == winLine[2]) ? 2 : 0);
		const ImVec2 vec0     = CoordVec2(winLine[from] % 3, winLine[from] / 3);
		const ImVec2 vec2     = CoordVec2(winLine[2 - from] % 3, winLine[2 - from] / 3);
		const ImVec2 dir      = (vec2 - vec0) * 0.15f;
		const float  delta    = bx::easeInOutSine(bx::max((restartTime - restartDelay * 0.5f - time) * 2.0f / restartDelay, 0.0f));
		const ImVec2 vecDelta = vec0 * delta + (vec2 + dir) * (1.0f - delta);
		drawList->AddLine(vec0 - dir, vecDelta, (playerId == 1) ? ImColor(255, 160, 0) : ImColor(0, 160, 255), thickness * 3);
	}

	// 7) display scores
	{
		const float gap    = size.x * 0.1f;
		const char* draws  = Format("D: %d", scores[3]);
		const float widthD = ImGui::CalcTextSize(draws).x;
		drawList->AddText(ImVec2(topLeft.x + size.x * 0.5f - widthD * 0.5f, topLeft.y + size.y * 0.05f), ImColor(255, 255, 255), draws);
		drawList->AddText(ImVec2(topLeft.x + gap, topLeft.y + size.y * 0.05f), ImColor(0, 160, 255), Format("X: %d", scores[1]));
		const char* score2 = Format("O: %d", scores[2]);
		const float width2 = ImGui::CalcTextSize(score2).x;
		drawList->AddText(ImVec2(bottomRight.x - gap - width2, topLeft.y + size.y * 0.05f), ImColor(255, 160, 0), score2);
	}
}

FX_REGISTER(Oxo)
