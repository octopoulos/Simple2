// common3d.h
// @author octopoulos
// @version 2025-10-19

#pragma once

struct Vertex
{
	glm::vec3 position = { 0.0f, 0.0f, 0.0f };
	glm::vec3 normal   = { 0.0f, 0.0f, 1.0f };
	glm::vec2 uv       = { 0.0f, 0.0f };
	glm::vec4 color    = { 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 tangent  = { 1.0f, 0.0f, 0.0f, 1.0f };
};

/// Compute tangents (call this if they're missing from fbx/gltf)
void ComputeTangentsMikktspace(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

// clang-format off
inline glm::vec3    BulletToGlm(const btVector3& vec)  { return glm::vec3(vec.x(), vec.y(), vec.z()); }
inline glm::vec4    BulletToGlm(const btVector4& vec)  { return glm::vec4(vec.x(), vec.y(), vec.z(), vec.w()); }
inline btVector3    BxToBullet (const bx::Vec3& vec )  { return btVector3(vec.x, vec.y, vec.z); }
inline glm::vec3    BxToGlm    (const bx::Vec3& vec )  { return glm::vec3(vec.x, vec.y, vec.z); }
inline btVector3    GlmToBullet(const glm::vec3& vec)  { return btVector3(vec.x, vec.y, vec.z); }
inline btQuaternion GlmToBullet(const glm::quat& quat) { return btQuaternion(quat.x, quat.y, quat.z, quat.w); }
// clang-format on

/// Convert matrix to position, quaternion, scale
void DecomposeMatrix(const glm::mat4& matrix, glm::vec3& position, glm::quat& quaternion, glm::vec3& scale, float scaleRatio = 1.0f);

/// Print a 4x4 matrix
void PrintMatrix(const glm::mat4& mat, std::string_view name = "");
