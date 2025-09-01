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

#define V2 ImVec2

void FX2(ImDrawList* d, V2 a, V2 b, V2 sz, ImVec4 mouse, float t)
{
    for (int n = 0; n < (1.0f + sinf(t * 5.7f)) * 40.0f; n++)
    {
        d->AddCircle(
            V2(a.x + sz.x * 0.5f, a.y + sz.y * 0.5f),
            sz.y * (0.01f + n * 0.03f),
            IM_COL32(255, 140 - n * 4, n * 3, 255)
        );
    }
}

#define wh(a) ImColor(1.f,1.f,1.f,a)
void FX(ImDrawList* d, ImVec2 a, ImVec2 b, ImVec2 sz, ImVec4 mouse, float t)
{
    static float fl;
    if ((rand() % 500) == 0) fl = t;
    if ((t-fl) > 0)
    {
        auto ft = 0.25f;
        d->AddRectFilled(a, b, wh((ft - (t - fl)) / ft));
    }

    for (int i = 0; i < 2000; ++i) {
        unsigned h = ImGui::GetID(d+i + int(t/4));
        auto f = fmodf(t + fmodf(h / 777.f, 99), 99);
        auto tx = h % (int)sz.x;
        auto ty = h % (int)sz.y;
        if (f < 1) {
            auto py = ty - 1000 * (1 - f);
            d->AddLine({ a.x + tx, a.y + py }, { a.x + tx, a.y + bx::min(py + 10,ty) }, (ImU32)-1);
        }
        else if (f < 1.2f)
            d->AddCircle({ a.x + tx, a.y + ty }, (f - 1) * 10 + h % 5, wh(1-(f-1)*5.f));
    }
}

void App::TestUi()
{
	if (!showTest) return;

	if (ImGui::Begin("TestUi", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoDocking))
	{
		ImVec2 windowPos  = ImGui::GetCursorScreenPos();
		ImVec2 windowSize = ImGui::GetContentRegionAvail();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// --- BACKGROUND RECT ---
		const float size = bx::min(windowSize.x * 0.4f, windowSize.y * 0.8f);

		ImVec2 bgSize(size, size);
		ImVec2 bgPos(
			windowPos.x + (windowSize.x - bgSize.x) * 0.5f,
			windowPos.y + 20.0f
		);
		ImVec2 bgEnd = bgPos + bgSize;

		// draw background
		drawList->AddRectFilled(bgPos, bgEnd, IM_COL32(10, 12, 20, 255), 10.0f);
		drawList->AddRect(bgPos, bgEnd, IM_COL32(255, 255, 255, 255), 10.0f, 0, 2.0f);

		// --- FX inside clipped region ---
		drawList->PushClipRect(bgPos, bgEnd, true);

		ImVec4 mouse;
		ImGuiIO& io = ImGui::GetIO();
		mouse.x = (io.MousePos.x - bgPos.x) / bgSize.x;
		mouse.y = (io.MousePos.y - bgPos.y) / bgSize.y;
		mouse.z = io.MouseDownDuration[0];
		mouse.w = io.MouseDownDuration[1];

		FX(drawList, bgPos, bgEnd, bgSize, mouse, (float)ImGui::GetTime());

		drawList->PopClipRect();

		// move cursor below background for text/buttons
		ImGui::Dummy(ImVec2(0, bgSize.y + 30));

		// --- TEXT ---
		const char* label = "Do you recognize this?";
		ImVec2 textSize = ImGui::CalcTextSize(label);
		float textX = (windowSize.x - textSize.x) * 0.5f;
		ImGui::SetCursorPosX(textX);
		ImGui::TextUnformatted(label);

		ImGui::Dummy(ImVec2(0, 10)); // spacing

		// --- BUTTONS ---
		ImVec2 buttonSize(120, 0);
		float spacing = 20.0f;
		float totalWidth = buttonSize.x * 2 + spacing;
		float startX = (windowSize.x - totalWidth) * 0.5f;

		ImGui::SetCursorPosX(startX);
		if (ImGui::Button("I don't know", buttonSize))
		{
			// handle
		}
		ImGui::SameLine();
		if (ImGui::Button("I know", buttonSize))
		{
			// handle
		}
	}
	ImGui::End();
}
