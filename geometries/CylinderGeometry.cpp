// CylinderGeometry.cpp
// @author octopoulos
// @version 2025-07-28

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreateCylinderGeometry(float radiusTop, float radiusBottom, float height, int radialSegments, int heightSegments, bool openEnded, float thetaStart, float thetaLength)
{
	// 1) parameters
	const int   radial     = std::max(3, radialSegments);
	const int   segmentsH  = std::max(1, heightSegments);
	const float halfHeight = height * 0.5f;

	std::vector<PosNormalUV> vertices;
	std::vector<uint16_t>    indices;

	std::vector<std::vector<uint16_t>> indexArray;
	indexArray.reserve(segmentsH + 1);

	// slope for normal calculation
	const float slope = (radiusBottom - radiusTop) / height;

	// 2) generate vertices, normals, uvs for the torso
	for (int y = 0; y <= segmentsH; ++y)
	{
		std::vector<uint16_t> indexRow;
		const float           v      = float(y) / float(segmentsH);
		const float           radius = v * (radiusBottom - radiusTop) + radiusTop;
		const float           posY   = halfHeight - v * height;

		for (int x = 0; x <= radial; ++x)
		{
			const float u     = float(x) / float(radial);
			const float theta = thetaStart + u * thetaLength;

			const float sinTheta = bx::sin(theta);
			const float cosTheta = bx::cos(theta);

			// vertex position
			const float px = radius * sinTheta;
			const float py = posY;
			const float pz = radius * cosTheta;

			// normal (side)
			const bx::Vec3 normal = bx::normalize({ sinTheta, slope, cosTheta });

			// clang-format off
			vertices.push_back({
				px, py, pz,
				normal.x, normal.y, normal.z,
				u, 1.0f - v,
			});
			// clang-format on

			indexRow.push_back(static_cast<uint16_t>(vertices.size() - 1));
		}
		indexArray.push_back(indexRow);
	}

	// 3) indices for sides
	for (int y = 0; y < segmentsH; ++y)
	{
		for (int x = 0; x < radial; ++x)
		{
			const uint16_t a = indexArray[y][x];
			const uint16_t b = indexArray[y + 1][x];
			const uint16_t c = indexArray[y + 1][x + 1];
			const uint16_t d = indexArray[y][x + 1];

			// faces with conditions from Three.js
			if (radiusTop > 0.0f || y != 0)
			{
				indices.push_back(a);
				indices.push_back(b);
				indices.push_back(d);
			}
			if (radiusBottom > 0.0f || y != segmentsH - 1)
			{
				indices.push_back(b);
				indices.push_back(c);
				indices.push_back(d);
			}
		}
	}

	// Helper lambda to build caps
	auto buildCap = [&](bool top) {
		const float sign   = top ? 1.0f : -1.0f;
		const float radius = top ? radiusTop : radiusBottom;
		if (radius <= 0.0f) return;

		const uint16_t centerIndexStart = static_cast<uint16_t>(vertices.size());

		// center vertices per segment (one per face for unique UVs)
		for (int x = 1; x <= radial; ++x)
		{
			// center vertex at cap center
			// clang-format off
			vertices.push_back({
				0.0f, sign * halfHeight, 0.0f,
				0.0f, sign, 0.0f,
				0.5f, 0.5f,
			});
			// clang-format on
		}

		const uint16_t centerIndexEnd = static_cast<uint16_t>(vertices.size());

		// surrounding vertices around circumference
		for (int x = 0; x <= radial; ++x)
		{
			const float u     = float(x) / float(radial);
			const float theta = u * thetaLength + thetaStart;

			const float cosTheta = bx::cos(theta);
			const float sinTheta = bx::sin(theta);

			const float px = radius * sinTheta;
			const float py = sign * halfHeight;
			const float pz = radius * cosTheta;

			// clang-format off
			vertices.push_back({
				px, py, pz,
				0.0f, sign, 0.0f,
				(cosTheta * 0.5f) + 0.5f, (sinTheta * 0.5f * sign) + 0.5f,
			});
			// clang-format on
		}

		// indices for cap faces
		for (int x = 0; x < radial; ++x)
		{
			const uint16_t c = centerIndexStart + x;
			const uint16_t i = centerIndexEnd + x;

			if (top)
			{
				indices.push_back(i);
				indices.push_back(i + 1);
				indices.push_back(c);
			}
			else
			{
				indices.push_back(i + 1);
				indices.push_back(i);
				indices.push_back(c);
			}
		}
	};

	// 4) caps
	if (!openEnded)
	{
		buildCap(true);  // top
		buildCap(false); // bottom
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
	const float     maxRadius = std::max(radiusTop, radiusBottom);
	const btVector3 aabb      = { maxRadius, halfHeight, maxRadius };
	const btVector3 dims      = { maxRadius, height, 0.0f };
	const float     boundR    = std::sqrt(maxRadius * maxRadius + (height * 0.5f) * (height * 0.5f));

	return std::make_shared<Geometry>(vbh, ibh, aabb, dims, boundR);
}
