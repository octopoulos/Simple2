// PolyhedronGeometry.cpp
// @author octopoulos
// @version 2025-08-05
//
// based on THREE.js PolyhedronGeometry implementation

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreatePolyhedronGeometry(int type, std::string&& args, const float* vertices, int vertexCount, const uint16_t* indices, int indexCount, float radius, int detail)
{
	if (vertexCount % 3 != 0 || indexCount % 3 != 0) return nullptr;

	// 1) Geometry data
	std::vector<glm::vec3> vertexBuffer; // Position data
	std::vector<float>     uvBuffer;     // UV data

	// helper functions
	auto getVertexByIndex = [&](int idx) {
		const int stride = idx * 3;
		return glm::vec3(vertices[stride], vertices[stride + 1], vertices[stride + 2]);
	};

	auto pushVertex = [&](const glm::vec3& v) {
		vertexBuffer.push_back(v);
	};

	auto subdivideFace = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, int detail) {
		const int cols = detail + 1;

		// we use this multidimensional array as a data structure for creating the subdivision
		std::vector<std::vector<glm::vec3>> v(cols + 1);

		// construct all of the vertices for this subdivision
		for (int i = 0; i <= cols; ++i)
		{
			v[i].resize(cols - i + 1);

			glm::vec3 aj = glm::mix(a, c, float(i) / cols);
			glm::vec3 bj = glm::mix(b, c, float(i) / cols);

			const int rows = cols - i;
			for (int j = 0; j <= rows; ++j)

				if (j == 0 && i == cols)
					v[i][j] = aj;
				else
					v[i][j] = glm::mix(aj, bj, float(j) / rows);
		}

		// construct all of the faces
		for (int i = 0; i < cols; ++i)
		{
			for (int j = 0; j < 2 * (cols - i) - 1; ++j)
			{
				const int k = j / 2;
				if ((j & 1) == 0)
				{
					pushVertex(v[i + 0][k + 1]);
					pushVertex(v[i + 1][k + 0]);
					pushVertex(v[i + 0][k + 0]);
				}
				else
				{
					pushVertex(v[i + 0][k + 1]);
					pushVertex(v[i + 1][k + 1]);
					pushVertex(v[i + 1][k + 0]);
				}
			}
		}
	};

	auto subdivide = [&](int detail) {
		// iterate over all faces and apply a subdivision with the given detail value
		for (int i = 0; i < indexCount; i += 3)
			subdivideFace(getVertexByIndex(indices[i]), getVertexByIndex(indices[i + 1]), getVertexByIndex(indices[i + 2]), detail);
	};

	auto applyRadius = [&](float radius) {
		// iterate over the entire buffer and apply the radius to each vertex
		for (auto& v : vertexBuffer)
			v = glm::normalize(v) * radius;
	};

	// Angle around the Y axis, counter-clockwise when looking from above.
	auto azimuth = [](const glm::vec3& vector) {
		return atan2f(vector.z, -vector.x);
	};

	// Angle above the XZ plane.
	auto inclination = [](const glm::vec3& vector) {
		return atan2f(-vector.y, sqrtf((vector.x * vector.x) + (vector.z * vector.z)));
	};

	auto correctUV = [&](float& u, size_t stride, const glm::vec3& vector, float azimuth) {
		if ((azimuth < 0) && (u == 1))
			uvBuffer[stride] = u - 1;

		if (glm::abs(vector.x) < 1e-6f && glm::abs(vector.z) < 1e-6f)
			uvBuffer[stride] = azimuth / glm::two_pi<float>() + 0.5f;
	};

	auto correctUVs = [&]() {
		for (size_t i = 0, j = 0; i < vertexBuffer.size(); i += 3, j += 6)
		{
			const auto& a        = vertexBuffer[i + 0];
			const auto& b        = vertexBuffer[i + 1];
			const auto& c        = vertexBuffer[i + 2];
			const auto  centroid = (a + b + c) / 3.0f;
			const float azi      = azimuth(centroid);

			correctUV(uvBuffer[j + 0], j + 0, a, azi);
			correctUV(uvBuffer[j + 2], j + 2, b, azi);
			correctUV(uvBuffer[j + 4], j + 4, c, azi);
		}
	};

	auto correctSeam = [&]() {
		// handle case when face straddles the seam, see #3269
		for (size_t i = 0; i < uvBuffer.size(); i += 6)
		{
			// uv data of a single face
			const float x0  = uvBuffer[i + 0];
			const float x1  = uvBuffer[i + 2];
			const float x2  = uvBuffer[i + 4];
			const float max = glm::max(glm::max(x0, x1), x2);
			const float min = glm::min(glm::min(x0, x1), x2);

			// 0.9 is somewhat arbitrary
			if (max > 0.9f && min < 0.1f)
			{
				if (x0 < 0.2f) uvBuffer[i + 0] += 1.0f;
				if (x1 < 0.2f) uvBuffer[i + 2] += 1.0f;
				if (x2 < 0.2f) uvBuffer[i + 4] += 1.0f;
			}
		}
	};

	auto generateUVs = [&]() {
		uvBuffer.clear();

		for (const auto& vertex : vertexBuffer)
		{
			const float u = azimuth(vertex) / glm::two_pi<float>() + 0.5f;
			const float v = inclination(vertex) / glm::pi<float>() + 0.5f;
			uvBuffer.push_back(u);
			uvBuffer.push_back(1 - v);
		}

		correctUVs();
		correctSeam();
	};

	// the subdivision creates the vertex buffer data
	subdivide(detail);

	// all vertices should lie on a conceptual sphere with a given radius
	applyRadius(radius);

	// finally, create the uv data
	generateUVs();

	// 2) vertices
	std::vector<PosNormalUV> finalVertices;
	for (size_t i = 0; i < vertexBuffer.size(); ++i)
	{
		const auto& v    = vertexBuffer[i];
		glm::vec3   norm = glm::normalize(v);

		// clang-format off
        finalVertices.push_back({
            v.x, v.y, v.z,
            norm.x, norm.y, norm.z,
            uvBuffer[i * 2], uvBuffer[i * 2 + 1]
        });
		// clang-format on
	}

	// 3) indices
	std::vector<uint16_t> finalIndices;
	for (uint16_t i = 0; i < finalVertices.size(); ++i)
		finalIndices.push_back(i);

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
	const bgfx::Memory*            vmem = bgfx::copy(finalVertices.data(), sizeof(PosNormalUV) * finalVertices.size());
	const bgfx::Memory*            imem = bgfx::copy(finalIndices.data(), sizeof(uint16_t) * finalIndices.size());
	const bgfx::VertexBufferHandle vbh  = bgfx::createVertexBuffer(vmem, layout);
	const bgfx::IndexBufferHandle  ibh  = bgfx::createIndexBuffer(imem);

	// 6) bounds
	const btVector3 aabb = { radius, radius, radius };
	const btVector3 dims = { radius, radius, 0.0f };

	return std::make_shared<Geometry>(type, std::move(args), vbh, ibh, aabb, dims, radius, std::move(finalVertices), std::move(finalIndices));
}
