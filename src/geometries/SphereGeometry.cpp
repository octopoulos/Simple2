// SphereGeometry.cpp
// @author octopoulos
// @version 2025-09-27
//
// based on THREE.js SphereGeometry implementation

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreateSphereGeometry(float radius, int widthSegments, int heightSegments, float phiStart, float phiLength, float thetaStart, float thetaLength)
{
	std::string args = FormatStr("%s %d %d %s %s %s %s", Cfloat(radius), widthSegments, heightSegments, Cfloat(phiStart), Cfloat(phiLength), Cfloat(thetaStart), Cfloat(thetaLength));

	// 1) geometry
	widthSegments  = bx::max(3, widthSegments);
	heightSegments = bx::max(2, heightSegments);

	const float thetaEnd = bx::min(thetaStart + thetaLength, bx::kPi);

	std::vector<PosNormalUvColor>      vertices;
	std::vector<uint16_t>              indices;
	std::vector<std::vector<uint16_t>> grid;

	uint16_t index = 0;
	for (int iy = 0; iy <= heightSegments; ++iy)
	{
		std::vector<uint16_t> verticesRow;
		const float           v = float(iy) / float(heightSegments);

		float uOffset = 0.0f;
		if (iy == 0 && thetaStart == 0.0f)
			uOffset = 0.5f / float(widthSegments);
		else if (iy == heightSegments && thetaEnd == bx::kPi)
			uOffset = -0.5f / float(widthSegments);

		for (int ix = 0; ix <= widthSegments; ++ix)
		{
			const float u = float(ix) / float(widthSegments);
			const float x = -radius * cosf(phiStart + u * phiLength) * sinf(thetaStart + v * thetaLength);
			const float y = radius * cosf(thetaStart + v * thetaLength);
			float       z = radius * sinf(phiStart + u * phiLength) * sinf(thetaStart + v * thetaLength);

			const bx::Vec3 pos    = { x, y, z };
			const bx::Vec3 normal = bx::normalize(pos);

			// clang-format off
			vertices.push_back({
				pos.x, pos.y, pos.z,
				normal.x, normal.y, normal.z,
				u + uOffset, 1.0f - v,
				0.8f, 0.8f, 0.8f, 1.0f,
			});
			// clang-format on

			verticesRow.push_back(index++);
		}

		grid.push_back(verticesRow);
	}

	for (int iy = 0; iy < heightSegments; ++iy)
	{
		for (int ix = 0; ix < widthSegments; ++ix)
		{
			const uint16_t a = grid[iy][ix + 1];
			const uint16_t b = grid[iy][ix];
			const uint16_t c = grid[iy + 1][ix];
			const uint16_t d = grid[iy + 1][ix + 1];

			if (iy != 0 || thetaStart > 0.0f)
			{
				indices.push_back(a);
				indices.push_back(b);
				indices.push_back(d);
			}
			if (iy != heightSegments - 1 || thetaEnd < bx::kPi)
			{
				indices.push_back(b);
				indices.push_back(c);
				indices.push_back(d);
			}
		}
	}

	// 2) vertex layout
	// clang-format off
	bgfx::VertexLayout layout;
	layout.begin()
		.add(bgfx::Attrib::Position , 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Normal   , 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0   , 4, bgfx::AttribType::Float)
		.end();
	// clang-format on

	// 3) buffers
	const bgfx::Memory*            vmem = bgfx::copy(vertices.data(), sizeof(PosNormalUvColor) * vertices.size());
	const bgfx::Memory*            imem = bgfx::copy(indices.data(), sizeof(uint16_t) * indices.size());
	const bgfx::VertexBufferHandle vbh  = bgfx::createVertexBuffer(vmem, layout);
	const bgfx::IndexBufferHandle  ibh  = bgfx::createIndexBuffer(imem);

	// 4) bounds
	const btVector3 dims = { radius, radius, radius };

	return std::make_shared<Geometry>(GeometryType_Sphere, std::move(args), vbh, ibh, dims, dims, radius);
}
