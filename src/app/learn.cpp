// learn.cpp
// @author octopoulos
// @version 2025-08-27

#include "stdafx.h"
#include "app/App.h"
//
#include "ui/ui.h" // ImGui::

#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

void App::LearnUi()
{
	if (!showLearn) return;

	const ImVec2 viewportSize = ImGui::GetMainViewport()->Size;

	const float sx = std::clamp(viewportSize.x * 0.8f, 300.0f, 1024.0f);
	const float sy = std::clamp(viewportSize.y * 0.8f, 300.0f, 800.0f);

	const ImVec2 pos = ImVec2((viewportSize.x - sx) * 0.5f, (viewportSize.y - sy) * 0.5f);
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(ImVec2(sx, sy));

	if (ImGui::Begin("Learning", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::Dummy(ImVec2(0, sy * 0.3f));

		//// center text
		//ImGui::GetCurrentWindow()->FontWindowScale = 3.0f;
		////ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
		//ImGui::SetCursorPosX((sx - ImGui::CalcTextSize("HELLO").x) * 0.5f);
		//ImGui::Text("HELLO");
		////ImGui::PopFont();
		////ImGui::GetCurrentWindow()->FontWindowScale = 1.0f;

		{
			// get current font
			ImFont*     font    = ImGui::GetFont();
			float       bigSize = ImGui::GetFontSize() * 3.0f;
			const char* text    = "HELLO";

			ImFontBaked* baked   = font->GetFontBaked(bigSize);

			// measure text
			ImVec2 textSize = font->CalcTextSizeA(bigSize, FLT_MAX, 0.0f, text);
			ImVec2 cursor   = ImGui::GetCursorScreenPos();
			ImVec2 center   = ImVec2(cursor.x + (sx - textSize.x) * 0.5f, cursor.y + sy * 0.3f);

			// draw text
			const ImU32  color    = IM_COL32(255, 255, 255, 255);
			const ImVec4 clipRect = ImGui::GetCurrentWindow()->ClipRect.ToVec4();

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			font->RenderText(drawList, bigSize, center, color, clipRect, text, nullptr);
		}

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

        {
            // Using shortcut. You can use PushStyleColor()/PopStyleColor() for more flexibility.
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Pink");
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Yellow");
            ImGui::TextDisabled("Disabled");
            ImGui::SameLine();// HelpMarker("The TextDisabled color is stored in ImGuiStyle.");
        }

		// if (ImGui::TreeNode("Progress Bars"))
		{
			// Animate a simple progress bar
			static float progress = 0.0f, progress_dir = 1.0f;
			progress += progress_dir * 0.4f * ImGui::GetIO().DeltaTime;
			if (progress >= +1.1f) { progress = +1.1f; progress_dir *= -1.0f; }
			if (progress <= -0.1f) { progress = -0.1f; progress_dir *= -1.0f; }

			// Typically we would use ImVec2(-1.0f,0.0f) or ImVec2(-FLT_MIN,0.0f) to use all available width,
			// or ImVec2(width,0.0f) for a specified width. ImVec2(0.0f,0.0f) uses ItemWidth.
			ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
			ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::Text("Progress Bar");

			float progress_saturated = IM_CLAMP(progress, 0.0f, 1.0f);
			char buf[32];
			sprintf(buf, "%d/%d", (int)(progress_saturated * 1753), 1753);
			ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), buf);

			// Pass an animated negative value, e.g. -1.0f * (float)ImGui::GetTime() is the recommended value.
			// Adjust the factor if you want to adjust the animation speed.
			ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(0.0f, 0.0f), "Searching..");
			ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::Text("Indeterminate");

			// ImGui::TreePop();
		}
	}
	ImGui::End();
}

void App::TestUi()
{
	if (!showTest) return;

	if (ImGui::Begin("TestUi"))
	{
	}
	ImGui::End();
}
