// BoxGeometry.cpp
// @author octopoulos
// @version 2025-09-18
//
// based on THREE.js BoxGeometry implementation

#include "stdafx.h"
#include "geometries/Geometry.h"

static void BuildPlane(std::vector<PosNormalUvColor>& vertices, std::vector<uint16_t>& indices, int u, int v, int w, int udir, int vdir, float width, float height, float depth, int gridX, int gridY, uint16_t& vertexOffset, const std::vector<glm::vec4>& faceColors, int faceId)
{
	const int    gridX1        = gridX + 1;
	const int    gridY1        = gridY + 1;
	const float  halfDepth     = depth / 2;
	const float  halfHeight    = height / 2;
	const float  halfWidth     = width / 2;
	const size_t numColor      = faceColors.size();
	const float  segmentHeight = height / gridY;
	const float  segmentWidth  = width / gridX;

	// normal vector
	float normal[3] = { 0 };
	{
		normal[w] = depth > 0 ? 1.0f : -1.0f;
	}

	// generate vertices
	for (int iy = 0; iy < gridY1; ++iy)
	{
		const float y = iy * segmentHeight - halfHeight;
		for (int ix = 0; ix < gridX1; ++ix)
		{
			const float x = ix * segmentWidth - halfWidth;

			float vector[3] = { 0 };
			{
				vector[u] = x * udir;
				vector[v] = y * vdir;
				vector[w] = halfDepth;
			}

			const glm::vec4& color = (numColor >= 6) ? faceColors[faceId] : (numColor ? faceColors[0] : glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));

			// clang-format off
			vertices.push_back({
				vector[0], vector[1], vector[2],
				normal[0], normal[1], normal[2],
				(float)ix / gridX, 1.0f - (float)iy / gridY,
				color.r, color.g, color.b, color.a,
			});
			// clang-format on
		}
	}

	// indices
	for (int iy = 0; iy < gridY; ++iy)
	{
		for (int ix = 0; ix < gridX; ++ix)
		{
			const uint16_t a = vertexOffset + ix + gridX1 * iy;
			const uint16_t b = vertexOffset + ix + gridX1 * (iy + 1);
			const uint16_t c = vertexOffset + (ix + 1) + gridX1 * (iy + 1);
			const uint16_t d = vertexOffset + (ix + 1) + gridX1 * iy;

			indices.push_back(a);
			indices.push_back(b);
			indices.push_back(d);
			indices.push_back(b);
			indices.push_back(c);
			indices.push_back(d);
		}
	}

	vertexOffset += gridX1 * gridY1;
}

uGeometry CreateBoxGeometry(float width, float height, float depth, int widthSegments, int heightSegments, int depthSegments, const std::vector<glm::vec4>& faceColors)
{
	std::string args = FormatStr("%f %f %f %d %d %d", width, height, depth, widthSegments, heightSegments, depthSegments);

	// 1) geometry
	std::vector<PosNormalUvColor> vertices;
	std::vector<uint16_t>         indices;

	uint16_t vertexOffset = 0;

	// clang-format off
	// px, nx
	BuildPlane(vertices, indices, 2, 1, 0,  1, -1, depth, height,  width, depthSegments, heightSegments, vertexOffset, faceColors, 0);
	BuildPlane(vertices, indices, 2, 1, 0, -1, -1, depth, height, -width, depthSegments, heightSegments, vertexOffset, faceColors, 1);
	// py, ny
	BuildPlane(vertices, indices, 0, 2, 1,  1,  1, width, depth,  height, widthSegments, depthSegments , vertexOffset, faceColors, 2);
	BuildPlane(vertices, indices, 0, 2, 1,  1, -1, width, depth, -height, widthSegments, depthSegments , vertexOffset, faceColors, 3);
	// pz, nz
	BuildPlane(vertices, indices, 0, 1, 2,  1, -1, width, height,  depth, widthSegments, heightSegments, vertexOffset, faceColors, 4);
	BuildPlane(vertices, indices, 0, 1, 2, -1, -1, width, height, -depth, widthSegments, heightSegments, vertexOffset, faceColors, 5);
	// clang-format on

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

	// 4) bounds: dims = [radius, height]
	const btVector3 aabb   = { width * 0.5f, height * 0.5f, depth * 0.5f };
	const btVector3 dims   = { std::sqrt(width * width + depth * depth) * 0.5f, height, 0.0f };
	const float     radius = bx::max(bx::max(aabb[0], aabb[1]), aabb[2]);

	return std::make_shared<Geometry>(GeometryType_Box, std::move(args), vbh, ibh, aabb, dims, radius);
}
