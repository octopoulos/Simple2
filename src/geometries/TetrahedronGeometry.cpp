// TetrahedronGeometry.cpp
// @author octopoulos
// @version 2025-08-05
//
// based on THREE.js TetrahedronGeometry implementation

#include "stdafx.h"
#include "geometries/Geometry.h"

uGeometry CreateTetrahedronGeometry(float radius, int detail)
{
	std::string args = fmt::format("{} {}", radius, detail);

	// clang-format off
	constexpr float vertices[] = {
		1, 1, 1,  -1, -1, 1,  -1, 1, -1,  1, -1, -1,
	};

	constexpr uint16_t indices[] = {
		2, 1, 0,  0, 3, 2,  1, 3, 0,  2, 3, 1,
	};
	// clang-format on

	return CreatePolyhedronGeometry(GeometryType_Tetrahedron, std::move(args), vertices, BX_COUNTOF(vertices), indices, BX_COUNTOF(indices), radius, detail);
}
