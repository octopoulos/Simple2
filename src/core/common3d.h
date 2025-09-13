// common3d.h
// @author octopoulos
// @version 2025-09-08

#pragma once

// clang-format off
inline glm::vec3    BulletToGlm(const btVector3& vec)  { return glm::vec3(vec.x(), vec.y(), vec.z()); }
inline glm::vec4    BulletToGlm(const btVector4& vec)  { return glm::vec4(vec.x(), vec.y(), vec.z(), vec.w()); }
inline btVector3    BxToBullet (const bx::Vec3& vec )  { return btVector3(vec.x, vec.y, vec.z); }
inline glm::vec3    BxToGlm    (const bx::Vec3& vec )  { return glm::vec3(vec.x, vec.y, vec.z); }
inline btVector3    GlmToBullet(const glm::vec3& vec)  { return btVector3(vec.x, vec.y, vec.z); }
inline btQuaternion GlmToBullet(const glm::quat& quat) { return btQuaternion(quat.x, quat.y, quat.z, quat.w); }
// clang-format on

/// Print a 4x4 matrix
void PrintMatrix(const glm::mat4& mat, std::string_view name = "");
