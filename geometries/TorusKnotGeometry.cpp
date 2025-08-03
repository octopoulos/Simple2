// TorusKnotGeometry.cpp
// @author octopoulos
// @version 2025-07-30
//
// based on THREE.js TorusKnotGeometry implementation

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreateTorusKnotGeometry(float radius, float tube, int tubularSegments, int radialSegments, int p, int q)
{
	// 1) sanitize inputs
	tubularSegments = glm::max(3, tubularSegments);
	radialSegments  = glm::max(3, radialSegments);

	std::vector<PosNormalUV> vertices;
	std::vector<uint16_t>    indices;

	vertices.reserve((tubularSegments + 1) * (radialSegments + 1));
	indices.reserve(tubularSegments * radialSegments * 6);

	auto calculatePositionOnCurve = [](float u, int p, int q, float radius, glm::vec3& pos) {
		const float cu      = cosf(u);
		const float su      = sinf(u);
		const float quOverP = float(q) / float(p) * u;
		const float cs      = cosf(quOverP);

		pos.x = radius * (2 + cs) * 0.5f * cu;
		pos.y = radius * (2 + cs) * 0.5f * su;
		pos.z = radius * sinf(quOverP) * 0.5f;
	};

	// 2) vertices
	for (int i = 0; i <= tubularSegments; ++i)
	{
		const float u = (float(i) / tubularSegments) * float(p) * glm::two_pi<float>();

		glm::vec3 P1, P2;
		calculatePositionOnCurve(u, p, q, radius, P1);
		calculatePositionOnCurve(u + 0.01f, p, q, radius, P2);

		glm::vec3 T = glm::normalize(P2 - P1);
		glm::vec3 N = glm::normalize(P2 + P1);
		glm::vec3 B = glm::normalize(glm::cross(T, N));
		N           = glm::normalize(glm::cross(B, T));

		for (int j = 0; j <= radialSegments; ++j)
		{
			const float v  = (float(j) / radialSegments) * glm::two_pi<float>();
			const float cx = -tube * cosf(v);
			const float cy = tube * sinf(v);

			const glm::vec3 pos    = P1 + cx * N + cy * B;
			const glm::vec3 normal = glm::normalize(pos - P1);
			const float     uCoord = float(i) / tubularSegments;
			const float     vCoord = float(j) / radialSegments;

			// clang-format off
			vertices.push_back({
				pos.x, pos.y, pos.z,
				normal.x, normal.y, normal.z,
				uCoord, vCoord,
			});
			// clang-format on
		}
	}

	// 3) indices
	for (int j = 1; j <= tubularSegments; ++j)
	{
		for (int i = 1; i <= radialSegments; ++i)
		{
			const uint16_t a = (radialSegments + 1) * (j - 1) + (i - 1);
			const uint16_t b = (radialSegments + 1) * (j + 0) + (i - 1);
			const uint16_t c = (radialSegments + 1) * (j + 0) + (i + 0);
			const uint16_t d = (radialSegments + 1) * (j - 1) + (i + 0);

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
	    .end();
	// clang-format on

	// 5) buffers
	const bgfx::Memory*      vmem = bgfx::copy(vertices.data(), sizeof(PosNormalUV) * vertices.size());
	const bgfx::Memory*      imem = bgfx::copy(indices.data(), sizeof(uint16_t) * indices.size());
	bgfx::VertexBufferHandle vbh  = bgfx::createVertexBuffer(vmem, layout);
	bgfx::IndexBufferHandle  ibh  = bgfx::createIndexBuffer(imem);

	const btVector3 aabb = { radius + tube, radius + tube, radius + tube };
	const btVector3 dims = { radius * 2, tube * 2, 0.0f };

	return std::make_shared<Geometry>(GeometryType_TorusKnot, vbh, ibh, aabb, dims, radius, std::move(vertices), std::move(indices));
}
