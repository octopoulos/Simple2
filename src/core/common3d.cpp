// common3d.cpp
// @author octopoulos
// @version 2025-10-19

#include "stdafx.h"
#include "core/common3d.h"

#include <mikktspace.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

struct MikkTUserData
{
	std::vector<Vertex>*         verts;
	const std::vector<uint32_t>* indices;
};

static int mikk_getNumFaces(const SMikkTSpaceContext* ctx)
{
	const auto* ud = static_cast<const MikkTUserData*>(ctx->m_pUserData);
	return static_cast<int>(ud->indices->size() / 3);
}

static int mikk_getNumVertsOfFace(const SMikkTSpaceContext*, const int) { return 3; }

static void mikk_getPosition(const SMikkTSpaceContext* ctx, float* out, const int face, const int vert)
{
	const auto* ud = static_cast<const MikkTUserData*>(ctx->m_pUserData);
	const auto& v  = (*ud->verts)[(*ud->indices)[face * 3 + vert]];
	out[0]         = v.position.x;
	out[1]         = v.position.y;
	out[2]         = v.position.z;
}

static void mikk_getNormal(const SMikkTSpaceContext* ctx, float* out, const int face, const int vert)
{
	const auto* ud = static_cast<const MikkTUserData*>(ctx->m_pUserData);
	const auto& v  = (*ud->verts)[(*ud->indices)[face * 3 + vert]];
	out[0]         = v.normal.x;
	out[1]         = v.normal.y;
	out[2]         = v.normal.z;
}

static void mikk_getTexCoord(const SMikkTSpaceContext* ctx, float* out, const int face, const int vert)
{
	const auto* ud = static_cast<const MikkTUserData*>(ctx->m_pUserData);
	const auto& v  = (*ud->verts)[(*ud->indices)[face * 3 + vert]];
	out[0]         = v.uv.x;
	out[1]         = v.uv.y;
}

static void mikk_setTSpaceBasic(const SMikkTSpaceContext* ctx, const float* tangent, const float biSign, const int face, const int vert)
{
	auto* ud  = static_cast<MikkTUserData*>(ctx->m_pUserData);
	auto& v   = (*ud->verts)[(*ud->indices)[face * 3 + vert]];
	v.tangent = glm::vec4(tangent[0], tangent[1], tangent[2], biSign);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
///////

void ComputeTangentsMikktspace(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	if (indices.empty() || vertices.empty()) return;

	MikkTUserData userData { &vertices, &indices };

	SMikkTSpaceInterface iface = {
		mikk_getNumFaces,
		mikk_getNumVertsOfFace,
		mikk_getPosition,
		mikk_getNormal,
		mikk_getTexCoord,
		mikk_setTSpaceBasic
	};

	SMikkTSpaceContext ctx = { &iface, &userData };

	if (!genTangSpaceDefault(&ctx))
		ui::LogError("ComputeTangentsMikktspace: genTangSpaceDefault failed");
}

void DecomposeMatrix(const glm::mat4& matrix, glm::vec3& position, glm::quat& quaternion, glm::vec3& scale, float scaleRatio)
{
	// 1) scale matrix
	glm::mat4 scaled = (scaleRatio != 1.0f) ? glm::scale(glm::mat4(1.0f), glm::vec3(scaleRatio)) * matrix : matrix;

	// extract position (translation)
	position = glm::vec3(scaled[3]);

	// extract scale from basis vectors
	scale.x = glm::length(glm::vec3(scaled[0]));
	scale.y = glm::length(glm::vec3(scaled[1]));
	scale.z = glm::length(glm::vec3(scaled[2]));

	// avoid division by zero
	if (scale.x == 0.0f) scale.x = 1.0f;
	if (scale.y == 0.0f) scale.y = 1.0f;
	if (scale.z == 0.0f) scale.z = 1.0f;

	// extract rotation matrix by removing scale
	glm::mat3 rotationMatrix(
	    glm::vec3(scaled[0]) / scale.x,
	    glm::vec3(scaled[1]) / scale.y,
	    glm::vec3(scaled[2]) / scale.z);

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
