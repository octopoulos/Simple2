// fx-Oxo.cpp
// @author octopoulos
// @version 2025-10-18

#include "stdafx.h"
#include "ui/ui-fx.h"
//
#include "entry/input.h"

// Check if there's a winner
static int CheckWinner(int matrix[3][3], int id)
{
	// 3 horizontals
	if (matrix[0][0] == id && matrix[0][1] == id && matrix[0][2] == id) return id;
	if (matrix[1][0] == id && matrix[1][1] == id && matrix[1][2] == id) return id;
	if (matrix[2][0] == id && matrix[2][1] == id && matrix[2][2] == id) return id;

	// 3 verticals
	if (matrix[0][0] == id && matrix[1][0] == id && matrix[2][0] == id) return id;
	if (matrix[0][1] == id && matrix[1][1] == id && matrix[2][1] == id) return id;
	if (matrix[0][2] == id && matrix[1][2] == id && matrix[2][2] == id) return id;

	// 2 diagonals
	if (matrix[0][0] == id && matrix[1][1] == id && matrix[2][2] == id) return id;
	if (matrix[0][2] == id && matrix[1][1] == id && matrix[2][0] == id) return id;

	// no winner
	return 0;
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
	static int   matrix[3][3] = {};
	static int   playerId     = 1;
	static float restartTime  = -1.0f;
	static int   scores[3]    = {};

	if (restartTime > 0.0f && time > restartTime)
	{
		memset(matrix, 0, sizeof(matrix));
		restartTime = -1.0f;
	}

	// 2) get inputs
	auto&       ginput = GetGlobalInput();
	const auto& finger = ginput.GetMouse();

	// 3) coords + sizes
	const ImVec2 center(topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f);
	const float  radius    = size.x * 0.12f;
	const float  thickness = size.x * 0.01f;
	const ImVec2 vecRadius = ImVec2(radius, radius);
	const float  stride    = radius * 2.0f + size.x * 0.01f;
	const ImVec2 mousePos  = ImVec2(finger.abs[0], finger.abs[1]);

	// 4) draw board
	// top left square
	const ImVec2 pos0 = ImVec2(center.x - stride, center.y - stride);

	for (int j = 0; j < 3; ++j)
	{
		for (int i = 0; i < 3; ++i)
		{
			const ImVec2 pos = ImVec2(pos0.x + i * stride, pos0.y + j * stride);

			// 4.a) draw square + maybe sign animation
			{
				const bool contain = (mousePos.x >= pos.x - radius && mousePos.x <= pos.x + radius && mousePos.y >= pos.y - radius && mousePos.y <= pos.y + radius);

				// draw square
				{
					const ImU32 color = (contain && restartTime < 0) ? ImColor(240, 210, 180) : ImColor(200, 180, 160);
					drawList->AddRectFilled(pos - vecRadius, pos + vecRadius, color);
				}

				// draw sign animation
				if (contain && matrix[j][i] == 0 && restartTime < 0)
				{
					if (ginput.buttonClicks[1])
					{
						matrix[j][i] = playerId;
						const int winner = CheckWinner(matrix, playerId);
						// winner? => restart in 1 sec
						if (winner > 0)
						{
							++scores[playerId];
							restartTime = time + 1.0f;
						}
						playerId = 3 - playerId;
					}
					else DrawSign(drawList, pos, playerId, radius * (0.5f + 0.2f * bx::sin(time * 4.0f)), thickness);
				}
			}

			// 4.b) draw placed X/O
			DrawSign(drawList, pos, matrix[j][i], radius * 0.9f, thickness);
		}
	}

	// 5) display scores
	{
		const float gap = size.x * 0.1f;
		drawList->AddText(ImVec2(topLeft.x + gap, topLeft.y + size.y * 0.05f), ImColor(255, 255, 255), Format("X: %d", scores[1]));
		const char* score2 = Format("O: %d", scores[2]);
		const float width2 = ImGui::CalcTextSize(score2).x;
		drawList->AddText(ImVec2(bottomRight.x - gap - width2, topLeft.y + size.y * 0.05f), ImColor(255, 255, 255), score2);
	}
}

FX_REGISTER(Oxo)
