// PolyhedronGeometry.cpp
// @author octopoulos
// @version 2025-07-28

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreatePolyhedronGeometry(const float* vertices, int vertexCount, const uint16_t* indices, int indexCount, float radius, int detail)
{
	// sanity check
	if (vertexCount % 3 != 0 || indexCount % 3 != 0) return nullptr;

	// 1) geometry
	std::vector<bx::Vec3> baseVerts;
	for (int i = 0; i < vertexCount; i += 3)
		baseVerts.push_back({ vertices[i], vertices[i + 1], vertices[i + 2] });

	std::vector<PosNormalUV> finalVertices;
	std::vector<uint16_t>    finalIndices;

	auto pushVertex = [&](const bx::Vec3& v) {
		bx::Vec3 norm = bx::normalize(v);
		// clang-format off
		finalVertices.push_back({
			norm.x * radius, norm.y * radius, norm.z * radius,
			norm.x, norm.y, norm.z,
			0.5f + atan2(norm.z, norm.x) / bx::kPi2,
			0.5f - asinf(norm.y) / bx::kPi,
		});
		// clang-format on
	};

	std::function<void(const bx::Vec3&, const bx::Vec3&, const bx::Vec3&, int)> subdivide;
	subdivide = [&](const bx::Vec3& a, const bx::Vec3& b, const bx::Vec3& c, int depth) {
		if (depth == 0)
		{
			const uint16_t idx = (uint16_t)finalVertices.size();
			pushVertex(a);
			pushVertex(b);
			pushVertex(c);
			finalIndices.push_back(idx);
			finalIndices.push_back(idx + 1);
			finalIndices.push_back(idx + 2);
			return;
		}

		const bx::Vec3 ab = bx::normalize(bx::add(a, b)); // midpoint, then normalize
		const bx::Vec3 bc = bx::normalize(bx::add(b, c));
		const bx::Vec3 ca = bx::normalize(bx::add(c, a));

		subdivide(a, ab, ca, depth - 1);
		subdivide(b, bc, ab, depth - 1);
		subdivide(c, ca, bc, depth - 1);
		subdivide(ab, bc, ca, depth - 1);
	};

	for (int i = 0; i < indexCount; i += 3)
	{
		const bx::Vec3& a = baseVerts[indices[i]];
		const bx::Vec3& b = baseVerts[indices[i + 1]];
		const bx::Vec3& c = baseVerts[indices[i + 2]];
		subdivide(a, b, c, detail);
	}

	// 2) vertex layout
	// clang-format off
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position , 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Normal   , 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .end();
	// clang-format on

	// 3) buffers
	const bgfx::Memory*            vmem = bgfx::copy(finalVertices.data(), sizeof(PosNormalUV) * finalVertices.size());
	const bgfx::Memory*            imem = bgfx::copy(finalIndices.data(), sizeof(uint16_t) * finalIndices.size());
	const bgfx::VertexBufferHandle vbh  = bgfx::createVertexBuffer(vmem, layout);
	const bgfx::IndexBufferHandle  ibh  = bgfx::createIndexBuffer(imem);

	// 4) bounds
	const btVector3 aabb = { radius, radius, radius };
	const btVector3 dims = { radius, radius, 0.0f };

	return std::make_shared<Geometry>(vbh, ibh, aabb, dims, radius);
}
