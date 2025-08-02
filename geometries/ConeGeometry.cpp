// ConeGeometry.cpp
// @author octopoulos
// @version 2025-07-28
//
// based on THREE.js ConeGeometry implementation

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreateConeGeometry(float radius, float height, int radialSegments, int heightSegments, bool openEnded, float thetaStart, float thetaLength)
{
	// 1) configuration
	const float halfHeight = height * 0.5f;
	const int   radial     = std::max(3, radialSegments);
	const int   segmentsH  = std::max(1, heightSegments);

	std::vector<PosNormalUV> vertices;
	std::vector<uint16_t>    indices;

	const int vertsPerRow = radial + 1;

	// 2) side vertices
	for (int iy = 0; iy <= segmentsH; ++iy)
	{
		const float v     = float(iy) / float(segmentsH);
		const float currR = radius * (1.0f - v);
		const float y     = -halfHeight + v * height;

		for (int ix = 0; ix <= radial; ++ix)
		{
			const float u     = float(ix) / float(radial);
			const float theta = thetaStart + u * thetaLength;

			const float sinT = bx::sin(theta);
			const float cosT = bx::cos(theta);

			const float x = -currR * cosT;
			const float z = currR * sinT;

			const bx::Vec3 pos    = { x, y, z };
			const bx::Vec3 normal = bx::normalize({ x, radius, z });

			// clang-format off
			vertices.push_back({
				pos.x, pos.y, pos.z,
				normal.x, normal.y, normal.z,
				u, 1.0f - v,
			});
			// clang-format on
		}
	}

	// 3) side indices
	for (int iy = 0; iy < segmentsH; ++iy)
	{
		for (int ix = 0; ix < radial; ++ix)
		{
			const uint16_t a = iy * vertsPerRow + ix;
			const uint16_t b = a + 1;
			const uint16_t c = (iy + 1) * vertsPerRow + ix;
			const uint16_t d = c + 1;

			indices.push_back(a);
			indices.push_back(b);
			indices.push_back(c);

			indices.push_back(b);
			indices.push_back(d);
			indices.push_back(c);
		}
	}

	// 4) bottom cap
	if (!openEnded)
	{
		const uint16_t centerIndex = static_cast<uint16_t>(vertices.size());

		// center vertex
		// clang-format off
		vertices.push_back({
			0.0f, -halfHeight, 0.0f,
			0.0f, -1.0f, 0.0f,
			0.5f, 0.5f,
		});
		// clang-format on

		for (int ix = 0; ix <= radial; ++ix)
		{
			const float u     = float(ix) / float(radial);
			const float theta = thetaStart + u * thetaLength;

			const float sinT = bx::sin(theta);
			const float cosT = bx::cos(theta);

			const float x = -radius * cosT;
			const float z = radius * sinT;

			// clang-format off
			vertices.push_back({
				x, -halfHeight, z,
				0.0f, -1.0f, 0.0f,
				(x / radius + 1.0f) * 0.5f,
				(z / radius + 1.0f) * 0.5f,
			});
			// clang-format on
		}

		for (int ix = 0; ix < radial; ++ix)
		{
			const uint16_t a = centerIndex + 1 + ix;
			const uint16_t b = centerIndex + 1 + (ix + 1) % (radial + 1);
			indices.push_back(a);
			indices.push_back(b);
			indices.push_back(centerIndex);
		}
	}

	// 5) vertex layout
	// clang-format off
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position , 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Normal   , 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .end();
	// clang-format on

	// 6) buffers
	const bgfx::Memory*            vmem = bgfx::copy(vertices.data(), sizeof(PosNormalUV) * vertices.size());
	const bgfx::Memory*            imem = bgfx::copy(indices.data(), sizeof(uint16_t) * indices.size());
	const bgfx::VertexBufferHandle vbh  = bgfx::createVertexBuffer(vmem, layout);
	const bgfx::IndexBufferHandle  ibh  = bgfx::createIndexBuffer(imem);

	// 7) bounds: dims = [radius, height]
	const btVector3 aabb   = { radius, height * 0.5f, radius };
	const btVector3 dims   = { radius, height, 0.0f };
	const float     boundR = std::sqrt(radius * radius + height * height * 0.25f);

	return std::make_shared<Geometry>(vbh, ibh, aabb, dims, boundR);
}
