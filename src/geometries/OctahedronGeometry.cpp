// OctahedronGeometry.cpp
// @author octopoulos
// @version 2025-09-18
//
// based on THREE.js OctahedronGeometry implementation

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreateOctahedronGeometry(float radius, int detail)
{
	std::string args = FormatStr("%f %d", radius, detail);

	// clang-format off
	constexpr float vertices[] = {
		1,  0, 0,  -1, 0, 0,  0, 1,  0,
		0, -1, 0,   0, 0, 1,  0, 0, -1,
	};

	constexpr uint16_t indices[] = {
		0, 2, 4,  0, 4, 3,  0, 3, 5,
		0, 5, 2,  1, 2, 5,  1, 5, 3,
		1, 3, 4,  1, 4, 2,
	};
	// clang-format on

	return CreatePolyhedronGeometry(GeometryType_Octahedron, std::move(args), vertices, BX_COUNTOF(vertices), indices, BX_COUNTOF(indices), radius, detail);
}
