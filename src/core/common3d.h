// common3d.h
// @author octopoulos
// @version 2025-09-11

#pragma once

// clang-format off
inline glm::vec3    BulletToGlm(const btVector3& vec)  { return glm::vec3(vec.x(), vec.y(), vec.z()); }
inline glm::vec4    BulletToGlm(const btVector4& vec)  { return glm::vec4(vec.x(), vec.y(), vec.z(), vec.w()); }
inline btVector3    BxToBullet (const bx::Vec3& vec )  { return btVector3(vec.x, vec.y, vec.z); }
inline glm::vec3    BxToGlm    (const bx::Vec3& vec )  { return glm::vec3(vec.x, vec.y, vec.z); }
inline btVector3    GlmToBullet(const glm::vec3& vec)  { return btVector3(vec.x, vec.y, vec.z); }
inline btQuaternion GlmToBullet(const glm::quat& quat) { return btQuaternion(quat.x, quat.y, quat.z, quat.w); }
// clang-format on

/// Easing function: cubic in-out for sharper start and end
inline float EaseInOutCubic(float t)
{
	t = glm::clamp(t, 0.0f, 1.0f);
	return t < 0.5f ? 4.0f * t * t * t : 1.0f - glm::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

/// Easing function: quadratic in-out for smooth start and end
inline float EaseInOutQuad(float t)
{
	t = glm::clamp(t, 0.0f, 1.0f);
	return t * t / (2.0f * (t * t - t) + 1.0f);
}

/// Easing function: quadratic out for slow end
inline float EaseOutQuad(float t)
{
	t = glm::clamp(t, 0.0f, 1.0f);
	return 1.0f - (1.0f - t) * (1.0f - t);
}

/// Print a 4x4 matrix
void PrintMatrix(const glm::mat4& mat, std::string_view name = "");
