// TorusGeometry.cpp
// @author octopoulos
// @version 2025-08-05
//
// based on THREE.js TorusGeometry implementation

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreateTorusGeometry(float radius, float tube, int radialSegments, int tubularSegments, float arc)
{
	std::string args = fmt::format("{} {} {} {} {}", radius, tube, radialSegments, tubularSegments, arc);

	const int   radial  = std::max(3, radialSegments);
	const int   tubular = std::max(3, tubularSegments);
	const float torusR  = radius;
	const float tubeR   = tube;

	std::vector<PosNormalUV> vertices;
	std::vector<uint16_t>    indices;

	// 1) vertex generation
	for (int j = 0; j <= radial; ++j)
	{
		const float v   = float(j) / float(radial);
		const float phi = v * bx::kPi2;

		const float cosPhi = bx::cos(phi);
		const float sinPhi = bx::sin(phi);

		for (int i = 0; i <= tubular; ++i)
		{
			const float u     = float(i) / float(tubular);
			const float theta = u * arc;

			const float cosTheta = bx::cos(theta);
			const float sinTheta = bx::sin(theta);

			const float cx = torusR * cosTheta;
			const float cy = torusR * sinTheta;

			const float px = (torusR + tubeR * cosPhi) * cosTheta;
			const float py = tubeR * sinPhi;
			const float pz = (torusR + tubeR * cosPhi) * sinTheta;

			// center point on torus ring
			const float centerX = cx;
			const float centerY = 0.0f;
			const float centerZ = cy;

			// normal = normalize(vertex - center)
			const bx::Vec3 pos    = { px, py, pz };
			const bx::Vec3 center = { centerX, centerY, centerZ };
			const bx::Vec3 normal = bx::normalize(bx::sub(pos, center));

			// clang-format off
			vertices.push_back({
				pos.x, pos.y, pos.z,
				normal.x, normal.y, normal.z,
				u, v
			});
			// clang-format on
		}
	}

	// 2) index generation
	for (int j = 1; j <= radial; ++j)
	{
		for (int i = 1; i <= tubular; ++i)
		{
			const uint16_t a = (tubular + 1) * j + i - 1;
			const uint16_t b = (tubular + 1) * (j - 1) + i - 1;
			const uint16_t c = (tubular + 1) * (j - 1) + i;
			const uint16_t d = (tubular + 1) * j + i;

			indices.push_back(a);
			indices.push_back(b);
			indices.push_back(d);

			indices.push_back(b);
			indices.push_back(c);
			indices.push_back(d);
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

	// 5) bounding box and radius
	const float     totalRadius = torusR + tubeR;
	const btVector3 aabb        = { totalRadius, tubeR, totalRadius };
	const btVector3 dims        = { torusR, tubeR, 0.0f };
	const float     boundR      = totalRadius;

	return std::make_shared<Geometry>(GeometryType_Torus, std::move(args), vbh, ibh, aabb, dims, boundR, std::move(vertices), std::move(indices));
}
