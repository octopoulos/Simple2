// common3d.cpp
// @author octopoulos
// @version 2025-08-18

#include "stdafx.h"
#include "core/common3d.h"

void PrintMatrix(const glm::mat4& mat, std::string_view name)
{
	// clang-format off
	ui::Log("{}{}{}  {:7.3f} {:7.3f} {:7.3f} {:7.3f}\n  {:7.3f} {:7.3f} {:7.3f} {:7.3f}\n  {:7.3f} {:7.3f} {:7.3f} {:7.3f}\n  {:7.3f} {:7.3f} {:7.3f} {:7.3f}",
		name.size() ? "  " : "", name, name.size() ? ":\n" : "",
		mat[0][0], mat[1][0], mat[2][0], mat[3][0],
		mat[0][1], mat[1][1], mat[2][1], mat[3][1],
		mat[0][2], mat[1][2], mat[2][2], mat[3][2],
		mat[0][3], mat[1][3], mat[2][3], mat[3][3]
	);
	// clang-format on
}
