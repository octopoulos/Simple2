// fx-Mandelbrot.cpp
// @author octopoulos
// @version 2025-10-16

#include "stdafx.h"
#include "ui/ui-fx.h"

namespace ui
{

void Fx_Mandelbrot(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
	constexpr int    maxRects   = 64000;
	constexpr size_t maxIter    = 32;
	static float     scale      = 0.01f;
	static ImVec2    shift      = { -2.12f, -0.9f };
	constexpr float  zoomFactor = 0.97f;

	const float  aspect   = size.x / size.y;
	const int    virtualY = TO_INT(std::sqrt(maxRects / aspect));
	const int    virtualX = TO_INT(virtualY * aspect);
	const ImVec2 scaleXy(size.x / virtualX, size.y / virtualY);

	// mouse-based zoom
	if ((mouse.x >= 0.f && mouse.x <= 1.f && mouse.y >= 0.f && mouse.y <= 1.f) && (mouse.z > 0.f || mouse.w > 0.f))
	{
		const float zf = (mouse.z > 0.f) ? zoomFactor : 1.f / zoomFactor;
		shift.x -= mouse.x * virtualX * scale * (zf - 1.f);
		shift.y -= mouse.y * virtualY * scale * (zf - 1.f);
		scale *= zf;
	}

	// allocate a virtual pixel buffer
	static std::vector<uint8_t> buffer;
	if (buffer.size() != virtualX * virtualY) buffer.resize(virtualX * virtualY);

	// compute Mandelbrot using separate real/imag floats
	#pragma omp parallel for schedule(dynamic)
	for (int iy = 0; iy < virtualY; ++iy)
	{
		const double ci = shift.y + iy / (virtualY - 1.f) * (virtualY * scale);
		for (int ix = 0; ix < virtualX; ++ix)
		{
			const double cr   = shift.x + ix / (virtualX - 1.f) * (virtualX * scale);
			double       zi   = 0.0;
			double       zr   = 0.0;
			size_t       iter = 0;

			while (iter < maxIter && zr * zr + zi * zi < 4.0)
			{
				double zr2 = zr * zr - zi * zi + cr;
				double zi2 = 2.0 * zr * zi + ci;
				zr         = zr2;
				zi         = zi2;
				++iter;
			}

			buffer[iy * virtualX + ix] = (iter < maxIter) ? TO_UINT8(std::log(TO_FLOAT(iter)) / std::log(TO_FLOAT(maxIter - 1)) * 255.f) : 0;
		}
	}

	// draw the scaled-up rectangles
	for (int iy = 0; iy < virtualY; ++iy)
	{
		for (int ix = 0; ix < virtualX; ++ix)
		{
			if (const uint8_t v = buffer[iy * virtualX + ix])
			{
				const ImU32  col     = IM_COL32(v, 255 - v, 255, 255);
				const ImVec2 pixelTl = { topLeft.x + ix * scaleXy.x, topLeft.y + iy * scaleXy.y };
				const ImVec2 pixelBr = { pixelTl.x + scaleXy.x, pixelTl.y + scaleXy.y };
				drawList->AddRectFilled(pixelTl, pixelBr, col);
			}
		}
	}
}

FX_REGISTER(Mandelbrot)

} // namespace ui
