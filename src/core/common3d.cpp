// common3d.cpp
// @author octopoulos
// @version 2025-10-04

#include "stdafx.h"
#include "core/common3d.h"

void DecomposeMatrix(const glm::mat4& matrix, glm::vec3& position, glm::quat& quaternion, glm::vec3& scale, float scaleRatio)
{
	// extract position (translation)
	position = glm::vec3(matrix[3]);

	// extract scale from basis vectors
	scale.x = glm::length(glm::vec3(matrix[0]));
	scale.y = glm::length(glm::vec3(matrix[1]));
	scale.z = glm::length(glm::vec3(matrix[2]));
	scale *= scaleRatio;

	// avoid division by zero
	if (scale.x == 0.0f) scale.x = 1.0f;
	if (scale.y == 0.0f) scale.y = 1.0f;
	if (scale.z == 0.0f) scale.z = 1.0f;

	// extract rotation matrix by removing scale
	glm::mat3 rotationMatrix(
	    glm::vec3(matrix[0]) / scale.x,
	    glm::vec3(matrix[1]) / scale.y,
	    glm::vec3(matrix[2]) / scale.z);

	// convert to quaternion
	quaternion = glm::normalize(glm::quat_cast(rotationMatrix));
}

void PrintMatrix(const glm::mat4& mat, std::string_view name)
{
	// clang-format off
	ui::Log("%s%s%s  %7.3f %7.3f %7.3f %7.3f\n  %7.3f %7.3f %7.3f %7.3f\n  %7.3f %7.3f %7.3f %7.3f\n  %7.3f %7.3f %7.3f %7.3f",
		name.size() ? "  " : "", Cstr(name), name.size() ? ":\n" : "",
		mat[0][0], mat[1][0], mat[2][0], mat[3][0],
		mat[0][1], mat[1][1], mat[2][1], mat[3][1],
		mat[0][2], mat[1][2], mat[2][2], mat[3][2],
		mat[0][3], mat[1][3], mat[2][3], mat[3][3]
	);
	// clang-format on
}
