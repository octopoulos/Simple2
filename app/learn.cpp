// learn.cpp
// @author octopoulos
// @version 2025-07-24

#include "stdafx.h"
#include "app/app.h"

#include "dear-imgui/imgui.h"

void App::LearnUi()
{
	if (!showLearn) return;

	const ImVec2 viewportSize = ImGui::GetMainViewport()->Size;

	const float sx = std::clamp(viewportSize.x * 0.8f, 300.0f, 1024.0f);
	const float sy = std::clamp(viewportSize.y * 0.8f, 300.0f, 800.0f);

	const ImVec2 pos = ImVec2((viewportSize.x - sx) * 0.5f, (viewportSize.y - sy) * 0.5f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(sx, sy), ImGuiCond_Always);

	if (ImGui::Begin("Learning", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::Dummy(ImVec2(0, sy * 0.3f));

		// center text
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
		ImGui::SetCursorPosX((sx - ImGui::CalcTextSize("HELLO").x) * 0.5f);
		ImGui::Text("HELLO");
		ImGui::PopFont();

		ImGui::Dummy(ImVec2(0, sy * 0.4f));

		// button row
		const float buttonWidth = 120.0f;
		const float spacing     = 20.0f;
		const float totalWidth  = buttonWidth * 2 + spacing;
		const float startX      = (sx - totalWidth) * 0.5f;

		ImGui::SetCursorPosX(startX);
		ImGui::Button("I don't know", ImVec2(buttonWidth, 0));

		ImGui::SameLine();
		ImGui::Button("I know", ImVec2(buttonWidth, 0));
	}
	ImGui::End();
}
