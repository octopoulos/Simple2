// PlaneGeometry.cpp
// @author octopoulos
// @version 2025-09-18
//
// based on THREE.js PlaneGeometry implementation

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreatePlaneGeometry(float width, float height, int widthSegments, int heightSegments)
{
	std::string args = FormatStr("%f %f %d %d", width, height, widthSegments, heightSegments);

	// 1) config
	widthSegments  = bx::max(1, widthSegments);
	heightSegments = bx::max(1, heightSegments);

	const float halfWidth  = width * 0.5f;
	const float halfHeight = height * 0.5f;

	const int gridX  = widthSegments;
	const int gridY  = heightSegments;
	const int gridX1 = gridX + 1;
	const int gridY1 = gridY + 1;

	const float segWidth  = width / float(gridX);
	const float segHeight = height / float(gridY);

	std::vector<PosNormalUvColor> vertices;
	std::vector<uint16_t>         indices;

	// 2) vertices
	for (int iy = 0; iy < gridY1; ++iy)
	{
		const float y = float(iy) * segHeight - halfHeight;

		for (int ix = 0; ix < gridX1; ++ix)
		{
			const float x = float(ix) * segWidth - halfWidth;

			// clang-format off
			vertices.push_back({
				x, 0.0f, y,
				0.0f, 1.0f, 0.0f,
				float(ix) / float(gridX), 1.0f - float(iy) / float(gridY),
				0.8f, 0.8f, 0.8f, 1.0f,
			});
			// clang-format on
		}
	}

	// 3) indices
	for (int iy = 0; iy < gridY; ++iy)
	{
		for (int ix = 0; ix < gridX; ++ix)
		{
			const uint16_t a = uint16_t(ix + gridX1 * iy);
			const uint16_t b = uint16_t(ix + gridX1 * (iy + 1));
			const uint16_t c = uint16_t((ix + 1) + gridX1 * (iy + 1));
			const uint16_t d = uint16_t((ix + 1) + gridX1 * iy);

			indices.push_back(a);
			indices.push_back(b);
			indices.push_back(d);

			indices.push_back(b);
			indices.push_back(c);
			indices.push_back(d);
		}
	}

	// 4) vertex layout
	// clang-format off
	bgfx::VertexLayout layout;
	layout.begin()
		.add(bgfx::Attrib::Position , 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Normal   , 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0   , 4, bgfx::AttribType::Float)
		.end();
	// clang-format on

	// 5) buffers
	const bgfx::Memory* vmem = bgfx::copy(vertices.data(), sizeof(PosNormalUvColor) * vertices.size());
	const bgfx::Memory* imem = bgfx::copy(indices.data(), sizeof(uint16_t) * indices.size());

	const bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(vmem, layout);
	const bgfx::IndexBufferHandle  ibh = bgfx::createIndexBuffer(imem);

	// 6) bounds: dims = [radius, height]
	const btVector3 aabb   = { halfWidth, 0.01f, halfHeight };
	const float     radius = (std::sqrt(halfWidth * halfWidth + halfHeight * halfHeight) + bx::max(halfWidth, halfHeight)) * 0.5f;
	const btVector3 dims   = { radius, 0.01f, 0.0f };

	return std::make_shared<Geometry>(GeometryType_Plane, std::move(args), vbh, ibh, aabb, dims, radius);
}
