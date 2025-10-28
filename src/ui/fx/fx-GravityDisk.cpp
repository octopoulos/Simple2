// fx-GravityDisk.cpp
// @author octopoulos
// @version 2025-10-24
//
// Simple particle simulation to demonstrate collapse + conservation of angular momentum and the flattening into a disk.
// Two views: top (X,Y) and side (X,Z).
// Controls: Reset, Pause, Particle count, Toggle Z-damping (cooling).

#include "stdafx.h"
#include "ui/ui-fx.h"
#include "entry/input.h"
#include "bx/math.h"

struct P
{
	glm::vec3 pos; // x, y, z
	glm::vec3 vel; // vx, vy, vz
	float     mass;
	ImU32     color;
};

static void Fx_GravityDisk(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	const float  margin    = 8.0f;
	const ImVec2 uiTopLeft = ImVec2(topLeft.x + margin, topLeft.y + margin);
	const ImVec2 uiSize    = ImVec2(size.x - margin * 2.0f, size.y * 0.14f);

	static std::vector<P> particles;
	static bool           paused         = false;
	static bool           zDamping       = true;
	static int            particleCount  = 800;
	static float          simSpeed       = 1.0f;
	static float          collapseFactor = 1.0f;
	static float          lastTime       = time;
	static float          softening      = 0.25f;
	static float          rotationOmega  = 0.8f;
	static float          zDampingFactor = 0.995f;
	static bool           showVectors    = false;

	// === UI background ===
	drawList->AddRectFilled(
	    uiTopLeft,
	    ImVec2(uiTopLeft.x + uiSize.x, uiTopLeft.y + uiSize.y),
	    IM_COL32(20, 20, 24, 200));

	auto&        ginput = GetGlobalInput();
	const auto&  finger = ginput.GetMouse();
	const ImVec2 mousePos(finger.abs[0], finger.abs[1]);

	// === Buttons ===
	const ImVec2 btn1  = ImVec2(uiTopLeft.x + 8, uiTopLeft.y + 8);
	const ImVec2 btn1b = ImVec2(btn1.x + 88, btn1.y + 28);
	drawList->AddRectFilled(btn1, btn1b, IM_COL32(64, 64, 64, 255));
	drawList->AddText(ImVec2(btn1.x + 8, btn1.y + 6), IM_COL32(255, 255, 255, 255), paused ? "Resume" : "Pause");

	const ImVec2 btn2  = ImVec2(btn1b.x + 8, btn1.y);
	const ImVec2 btn2b = ImVec2(btn2.x + 88, btn2.y + 28);
	drawList->AddRectFilled(btn2, btn2b, IM_COL32(64, 64, 64, 255));
	drawList->AddText(ImVec2(btn2.x + 8, btn2.y + 6), IM_COL32(255, 255, 255, 255), "Reset");

	// === Slider ===
	const ImVec2 sliderA = ImVec2(btn2b.x + 12, btn2.y);
	const ImVec2 sliderB = ImVec2(sliderA.x + 200, sliderA.y + 28);
	drawList->AddRectFilled(sliderA, sliderB, IM_COL32(40, 40, 40, 200));

	const char* pCountStr = Format("Particles: %d", particleCount);
	drawList->AddText(ImVec2(sliderA.x + 8, sliderA.y + 6), IM_COL32(200, 200, 200, 255), pCountStr);

	// === Click regions ===
	bool clickedPause          = false;
	bool clickedReset          = false;
	bool clickedIncrease       = false;
	bool clickedDecrease       = false;
	bool clickedToggleZDamping = false;
	bool clickedToggleVectors  = false;

	if (ginput.buttonClicks[1] > 0) // left click
	{
		if (mousePos.x >= btn1.x && mousePos.x <= btn1b.x && mousePos.y >= btn1.y && mousePos.y <= btn1b.y)
			clickedPause = true;
		else if (mousePos.x >= btn2.x && mousePos.x <= btn2b.x && mousePos.y >= btn2.y && mousePos.y <= btn2b.y)
			clickedReset = true;
		else if (mousePos.x >= sliderA.x && mousePos.x <= sliderB.x && mousePos.y >= sliderA.y && mousePos.y <= sliderB.y)
			clickedToggleZDamping = true;

		const ImVec2 plus   = ImVec2(sliderB.x + 12, sliderB.y - 28);
		const ImVec2 plusB  = ImVec2(plus.x + 28, plus.y + 28);
		const ImVec2 minus  = ImVec2(plusB.x + 6, plus.y);
		const ImVec2 minusB = ImVec2(minus.x + 28, minus.y + 28);

		drawList->AddRectFilled(plus, plusB, IM_COL32(64, 64, 64, 255));
		drawList->AddText(ImVec2(plus.x + 6, plus.y + 6), IM_COL32(255, 255, 255, 255), "+");
		drawList->AddRectFilled(minus, minusB, IM_COL32(64, 64, 64, 255));
		drawList->AddText(ImVec2(minus.x + 6, minus.y + 6), IM_COL32(255, 255, 255, 255), "-");

		if (mousePos.x >= plus.x && mousePos.x <= plusB.x && mousePos.y >= plus.y && mousePos.y <= plusB.y)
			clickedIncrease = true;
		if (mousePos.x >= minus.x && mousePos.x <= minusB.x && mousePos.y >= minus.y && mousePos.y <= minusB.y)
			clickedDecrease = true;

		const ImVec2 vecBox  = ImVec2(minusB.x + 12, minusB.y - 28);
		const ImVec2 vecBoxB = ImVec2(vecBox.x + 28, vecBox.y + 28);
		drawList->AddRectFilled(vecBox, vecBoxB, IM_COL32(64, 64, 64, 255));
		drawList->AddText(ImVec2(vecBox.x + 4, vecBox.y + 6), IM_COL32(255, 255, 255, 255), "V");

		if (mousePos.x >= vecBox.x && mousePos.x <= vecBoxB.x && mousePos.y >= vecBox.y && mousePos.y <= vecBoxB.y)
			clickedToggleVectors = true;
	}

	// === Apply click actions ===
	if (clickedPause) paused = !paused;
	if (clickedReset) particles.clear();
	if (clickedIncrease)
	{
		particleCount = bx::min(particleCount + 200, 5000);
		particles.clear();
	}
	if (clickedDecrease)
	{
		particleCount = bx::max(particleCount - 200, 100);
		particles.clear();
	}
	if (clickedToggleZDamping) zDamping = !zDamping;
	if (clickedToggleVectors) showVectors = !showVectors;

	// === Time step ===
	float dt = bx::clamp(time - lastTime, 0.0f, 0.1f) * simSpeed;
	lastTime = time;
	if (paused) dt = 0.0f;

	// === Initialization ===
	if (particles.empty())
	{
		const float R = 1.0f;
		particles.reserve(particleCount);
		for (int i = 0; i < particleCount; ++i)
		{
			const float u        = MerseneFloat(0.0f, 1.0f);
			const float costheta = MerseneFloat(-1.0f, 1.0f);
			const float phi      = MerseneFloat(0.0f, bx::kPi2);
			const float r        = bx::pow(u, 1.0f / 3.0f) * R;
			const float st       = bx::sqrt(1.0f - costheta * costheta);

			const float x = r * st * bx::cos(phi);
			const float y = r * st * bx::sin(phi);
			const float z = r * costheta;

			const float vx = -rotationOmega * y;
			const float vy = rotationOmega * x;
			const float vz = MerseneFloat(-0.05f, 0.05f);

			P p;
			p.pos   = glm::vec3(x, y, z);
			p.vel   = glm::vec3(vx, vy, vz);
			p.mass  = 1.0f;
			p.color = IM_COL32(200, 200, 255, 32);
			particles.push_back(p);
		}
	}

	// === Compute angular momentum (Lz) ===
	double Lz = 0.0;
	for (const P& p : particles)
		Lz += double(p.mass) * (double(p.pos.x) * double(p.vel.y) - double(p.pos.y) * double(p.vel.x));

	// === Physics integration ===
	const float G = 1.0f * collapseFactor;

	for (P& pi : particles)
	{
		const float rx    = pi.pos.x;
		const float ry    = pi.pos.y;
		const float rz    = pi.pos.z;
		const float r2    = rx * rx + ry * ry + rz * rz + softening;
		const float invR3 = 1.0f / (r2 * bx::sqrt(r2));

		glm::vec3 acc(-G * rx * invR3, -G * ry * invR3, -G * rz * invR3);

		// Small repulsion (approx)
		for (int k = 0; k < 3; ++k)
		{
			const auto j = (&pi - &particles[0]) + 13 * (k + 1) % particles.size();
			if (&particles[j] == &pi) continue;

			const P&    pj = particles[j];
			const float dx = pi.pos.x - pj.pos.x;
			const float dy = pi.pos.y - pj.pos.y;
			const float dz = pi.pos.z - pj.pos.z;
			const float d2 = dx * dx + dy * dy + dz * dz + 1e-6f;
			if (d2 < 0.01f)
			{
				const float push = 0.05f / d2;
				acc += glm::vec3(dx * push, dy * push, dz * push);
			}
		}

		pi.vel += acc * dt;

		if (zDamping)
			pi.vel.z *= bx::pow(zDampingFactor, dt * 60.0f);

		pi.vel.x *= 0.999f;
		pi.vel.y *= 0.999f;

		pi.pos += pi.vel * dt;
	}

	// === Panels ===
	const float  panelGap = 8.0f;
	const ImVec2 topPanelTL(topLeft.x + margin, uiTopLeft.y + uiSize.y + 12.0f);
	const float  halfHeight = (size.y - uiSize.y - margin * 2.0f - 12.0f) * 0.5f - panelGap * 0.5f;
	const ImVec2 topPanelBR(topLeft.x + size.x - margin, topPanelTL.y + halfHeight);
	const ImVec2 botPanelTL(topPanelTL.x, topPanelBR.y + panelGap);
	const ImVec2 botPanelBR(topPanelBR.x, botPanelTL.y + halfHeight);

	drawList->AddRectFilled(topPanelTL, topPanelBR, IM_COL32(8, 12, 18, 220));
	drawList->AddRectFilled(botPanelTL, botPanelBR, IM_COL32(8, 12, 18, 220));

	drawList->AddText(ImVec2(topPanelTL.x + 6, topPanelTL.y + 6), IM_COL32(220, 220, 220, 255), "Top view (X, Y)");
	drawList->AddText(ImVec2(botPanelTL.x + 6, botPanelTL.y + 6), IM_COL32(220, 220, 220, 255), "Side view (X, Z)");

	const float viewR       = 1.2f;
	const auto  toTopScreen = [&](const float sx, const float sy) -> ImVec2 {
        const float nx = (sx / viewR) * 0.5f + 0.5f;
        const float ny = (sy / viewR) * 0.5f + 0.5f;
        return ImVec2(
            bx::lerp(topPanelTL.x + 32.0f, topPanelBR.x - 32.0f, nx),
            bx::lerp(topPanelBR.y - 24.0f, topPanelTL.y + 24.0f, ny));
	};
	const auto toSideScreen = [&](const float sx, const float sz) -> ImVec2 {
		const float nx = (sx / viewR) * 0.5f + 0.5f;
		const float nz = (sz / viewR) * 0.5f + 0.5f;
		return ImVec2(
		    bx::lerp(botPanelTL.x + 32.0f, botPanelBR.x - 32.0f, nx),
		    bx::lerp(botPanelBR.y - 24.0f, botPanelTL.y + 24.0f, nz));
	};

	const ImVec2 centerTop  = toTopScreen(0, 0);
	const ImVec2 centerSide = toSideScreen(0, 0);
	drawList->AddCircle(centerTop, 6.0f, IM_COL32(255, 180, 0, 255), 8, 2.0f);
	drawList->AddCircle(centerSide, 6.0f, IM_COL32(255, 180, 0, 255), 8, 2.0f);

	for (const P& p : particles)
	{
		const ImVec2 s  = toTopScreen(p.pos.x, p.pos.y);
		const float  ps = bx::clamp(3.0f + 8.0f * (1.0f - bx::min(1.0f, bx::sqrt(p.pos.x * p.pos.x + p.pos.y * p.pos.y) / viewR)), 2.0f, 8.0f);
		drawList->AddCircleFilled(s, ps, p.color);

		const ImVec2 s2  = toSideScreen(p.pos.x, p.pos.z);
		const float  ps2 = bx::clamp(2.0f + 6.0f * (1.0f - bx::abs(p.pos.z) / viewR), 2.0f, 8.0f);
		drawList->AddCircleFilled(s2, ps2, p.color);

		if (showVectors)
		{
			const ImVec2 sv = ImVec2(s.x + p.vel.x * 8.0f, s.y - p.vel.y * 8.0f);
			drawList->AddLine(s, sv, IM_COL32(255, 255, 255, 120), 1.0f);
		}
	}

	const char* stats = Format("Particles: %d  Lz: %.3f  Z-damping: %s", TO_INT(particles.size()), Lz, zDamping ? "ON" : "OFF");
	drawList->AddText(ImVec2(topPanelTL.x + 6, topPanelBR.y + 6), IM_COL32(200, 200, 200, 255), stats);

	const char* instr = "Click Pause/Reset. Click slider to toggle Z-damping. +/- change count. V toggles velocity vectors.";
	drawList->AddText(ImVec2(topLeft.x + 6, bottomRight.y - 18), IM_COL32(160, 160, 160, 220), instr);
}

FX_REGISTER(GravityDisk)
