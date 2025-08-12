// IcosahedronGeometry.cpp
// @author octopoulos
// @version 2025-08-05
//
// based on THREE.js IcosahedronGeometry implementation

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreateIcosahedronGeometry(float radius, int detail)
{
	std::string args = fmt::format("{} {}", radius, detail);

	constexpr float t = (1.0f + bx::sqrt(5.0f)) * 0.5f;

	// clang-format off
	constexpr float vertices[] = {
		 -1, t,  0,  1, t, 0,  -1, -t, 0,    1, -t,  0,
		 0, -1,  t,  0, 1, t,   0, -1, -t,   0,  1, -t,
		 t,  0, -1,  t, 0, 1,  -t,  0, -1,  -t,  0,  1,
	};

	constexpr uint16_t indices[] = {
		 0, 11, 5,  0,  5,  1,   0,  1,  7,   0, 7, 10,  0, 10, 11,
		 1,  5, 9,  5, 11,  4,  11, 10,  2,  10, 7,  6,  7,  1,  8,
		 3,  9, 4,  3,  4,  2,   3,  2,  6,   3, 6,  8,  3,  8,  9,
		 4,  9, 5,  2,  4, 11,   6,  2, 10,   8, 6,  7,  9,  8,  1,
	};
	// clang-format on

	return CreatePolyhedronGeometry(GeometryType_Icosahedron, std::move(args), vertices, BX_COUNTOF(vertices), indices, BX_COUNTOF(indices), radius, detail);
}
