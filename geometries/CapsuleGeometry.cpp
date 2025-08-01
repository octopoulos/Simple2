// CapsuleGeometry.cpp
// @author octopoulos
// @version 2025-07-28

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreateCapsuleGeometry(float radius, float height, int capSegments, int radialSegments, int heightSegments)
{
	// 1) configuration
	const float h         = std::max(0.0f, height);
	const int   caps      = std::max(1, capSegments);
	const int   radial    = std::max(3, radialSegments);
	const int   segmentsH = std::max(1, heightSegments);

	const float halfHeight       = h * 0.5f;
	const float capArcLength     = bx::kPiHalf * radius;
	const float cylinderLength   = h;
	const float totalArcLength   = capArcLength * 2.0f + cylinderLength;
	const int   verticalSegments = caps * 2 + segmentsH;
	const int   vertsPerRow      = radial + 1;

	// 2) vertices + indices
	std::vector<PosNormalUV> vertices;
	std::vector<uint16_t>    indices;

	uint16_t index = 0;

	for (int iy = 0; iy <= verticalSegments; ++iy)
	{
		float profileY         = 0.0f;
		float profileRadius    = 0.0f;
		float normalYComponent = 0.0f;
		float arcLength        = 0.0f;

		if (iy <= caps)
		{
			// bottom cap
			const float t    = float(iy) / float(caps);
			const float ang  = t * bx::kPiHalf;
			profileY         = -halfHeight - radius * bx::cos(ang);
			profileRadius    = radius * bx::sin(ang);
			normalYComponent = -bx::cos(ang) * radius;
			arcLength        = t * capArcLength;
		}
		else if (iy <= caps + segmentsH)
		{
			// cylinder
			const float t    = float(iy - caps) / float(segmentsH);
			profileY         = -halfHeight + t * h;
			profileRadius    = radius;
			normalYComponent = 0.0f;
			arcLength        = capArcLength + t * cylinderLength;
		}
		else
		{
			// top cap
			const float t    = float(iy - caps - segmentsH) / float(caps);
			const float ang  = t * bx::kPiHalf;
			profileY         = halfHeight + radius * bx::sin(ang);
			profileRadius    = radius * bx::cos(ang);
			normalYComponent = radius * bx::sin(ang);
			arcLength        = capArcLength + cylinderLength + t * capArcLength;
		}

		const float v       = std::clamp(arcLength / totalArcLength, 0.0f, 1.0f);
		float       uOffset = 0.0f;

		if (iy == 0)
			uOffset = 0.5f / float(radial);
		else if (iy == verticalSegments)
			uOffset = -0.5f / float(radial);

		for (int ix = 0; ix <= radial; ++ix)
		{
			const float u     = float(ix) / float(radial);
			const float theta = u * bx::kPi2;
			const float sinT  = bx::sin(theta);
			const float cosT  = bx::cos(theta);

			const float x = -profileRadius * cosT;
			const float y = profileY;
			const float z = profileRadius * sinT;

			const bx::Vec3 pos    = { x, y, z };
			const bx::Vec3 normal = bx::normalize({ x, normalYComponent, z });

			// clang-format off
			vertices.push_back({
				pos.x, pos.y, pos.z,
				normal.x, normal.y, normal.z,
				u + uOffset, v,
			});
			// clang-format on

			if (iy > 0 && ix < radial)
			{
				const uint16_t i1 = (iy - 1) * vertsPerRow + ix;
				const uint16_t i2 = i1 + 1;
				const uint16_t i3 = iy * vertsPerRow + ix;
				const uint16_t i4 = i3 + 1;

				indices.push_back(i1);
				indices.push_back(i2);
				indices.push_back(i3);

				indices.push_back(i2);
				indices.push_back(i4);
				indices.push_back(i3);
			}

			++index;
		}
	}

	// 3) vertex layout
	// clang-format off
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position , 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Normal   , 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .end();
	// clang-format on

	// 4) buffers
	const bgfx::Memory*            vmem = bgfx::copy(vertices.data(), sizeof(PosNormalUV) * vertices.size());
	const bgfx::Memory*            imem = bgfx::copy(indices.data(), sizeof(uint16_t) * indices.size());
	const bgfx::VertexBufferHandle vbh  = bgfx::createVertexBuffer(vmem, layout);
	const bgfx::IndexBufferHandle  ibh  = bgfx::createIndexBuffer(imem);

	// 5) bounds: dims = [radius, height]
	const btVector3 aabb   = { radius, h * 0.5f + radius, radius };
	const btVector3 dims   = { radius, h, 0.0f };
	const float     boundR = h * 0.5f + radius;

	return std::make_shared<Geometry>(vbh, ibh, aabb, dims, boundR);
}
