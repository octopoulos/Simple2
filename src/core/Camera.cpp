// Camera.cpp
// @author octopoulos
// @version 2025-09-29

#include "stdafx.h"
#include "core/Camera.h"
//
#include "loaders/writer.h" // WRITE_CHAR, WRITE_KEY_VEC3
#include "ui/ui.h"          // ui::
#include "ui/xsettings.h"   // xsettings

constexpr float orthoZoomMax = 20.0f;
constexpr float orthoZoomMin = 0.0002f;

void Camera::Initialize()
{
	distance = 0.1f;
	orbit[0] = 0.0f;
	orbit[1] = 0.0f;
	pos      = bx::load<bx::Vec3>(xsettings.cameraEye);
	pos2     = bx::load<bx::Vec3>(xsettings.cameraEye);
	target   = bx::load<bx::Vec3>(xsettings.cameraAt);
	target2  = bx::load<bx::Vec3>(xsettings.cameraAt);
}

void Camera::ConsumeOrbit(float amount)
{
	float consume[2];
	consume[0] = orbit[0] * amount;
	consume[1] = orbit[1] * amount;
	orbit[0] -= consume[0];
	orbit[1] -= consume[1];

	// ui::Log("orbit=%7.3f %7.3f", orbit[0], orbit[1]);
	if (bx::abs(orbit[0]) < 1e-5f && bx::abs(orbit[1]) < 1e-5f) return;

	const bx::Vec3 toPos       = bx::sub(pos, target);
	const float    toPosLen    = bx::length(toPos);
	const float    invToPosLen = 1.0f / (toPosLen + bx::kFloatSmallest);
	const bx::Vec3 toPosNorm   = bx::mul(toPos, invToPosLen);

	float ll[2];
	bx::toLatLong(&ll[0], &ll[1], toPosNorm);
	ll[0] += consume[0];
	ll[1] -= consume[1];
	ll[1] = bx::clamp(ll[1], 0.02f, 0.98f);

	const bx::Vec3 tmp  = bx::fromLatLong(ll[0], ll[1]);
	const bx::Vec3 diff = bx::mul(bx::sub(tmp, toPosNorm), toPosLen);

	pos  = bx::add(pos, diff);
	pos2 = bx::add(pos2, diff);
}

void Camera::GetViewMatrix(float* viewMtx)
{
	bx::mtxLookAt(viewMtx, pos, target, up);
}

void Camera::Move(int cameraDir, float speed)
{
	const auto& dir = (cameraDir == CameraDir_Forward) ? forward : ((cameraDir == CameraDir_Right) ? right : up);

	distance = 0.1f;
	pos2     = bx::mad(dir, speed, pos2);
	target2  = bx::mad(forward, distance, pos2);
	follow &= ~CameraFollow_Active;
}

void Camera::Orbit(float dx, float dy)
{
	orbit[0] += dx;
	orbit[1] += dy;
}

void Camera::RotateAroundAxis(const bx::Vec3& axis, float angle)
{
	const auto quat    = bx::fromAxisAngle(axis, angle);
	const auto rotated = bx::mul(forward, quat);

	pos2 = bx::mad(rotated, -distance, target2);
}

int Camera::Serialize(fmt::memory_buffer& outString, int depth, int bounds, bool addChildren) const
{
	int keyId = Object3d::Serialize(outString, depth, (bounds & 1) ? 1 : 0, addChildren);
	if (keyId < 0) return keyId;

	WRITE_KEY_VEC3(pos);
	WRITE_KEY_VEC3(target);

	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}

void Camera::SetOrthographic(const bx::Vec3& axis)
{
	pos2 = bx::mad(bx::normalize(axis), distance, target2);

	xsettings.projection = Projection_Orthographic;
}

void Camera::ShowInfoTable(bool showTitle) const
{
	if (showTitle) ImGui::TextUnformatted("Camera");

	// clang-format off
	ui::ShowTable({
		{ "follow"  , std::to_string(follow)                                       },
		{ "forward" , Format("%.2f:%.2f:%.2f", forward.x, forward.y, forward.z)    },
		{ "pos"     , Format("%.2f:%.2f:%.2f", pos.x, pos.y, pos.z)                },
		{ "pos2"    , Format("%.2f:%.2f:%.2f", pos2.x, pos2.y, pos2.z)             },
		{ "position", Format("%.2f:%.2f:%.2f", position.x, position.y, position.z) },
		{ "right"   , Format("%.2f:%.2f:%.2f", right.x, right.y, right.z)          },
		{ "target"  , Format("%.2f:%.2f:%.2f", target.x, target.y, target.z)       },
		{ "target2" , Format("%.2f:%.2f:%.2f", target2.x, target2.y, target2.z)    },
		{ "up"      , Format("%.2f:%.2f:%.2f", up.x, up.y, up.z)                   },
		{ "worldUp" , Format("%.2f:%.2f:%.2f", worldUp.x, worldUp.y, worldUp.z)    },
	});
	// clang-format on
}

void Camera::Update(float delta)
{
	const float amount = bx::min(delta * 20.0f, 1.0f);
	ConsumeOrbit(amount);

	pos      = bx::lerp(pos, pos2, amount);
	target   = bx::lerp(target, target2, amount);
	forward  = bx::normalize(bx::sub(target, pos));
	forward2 = bx::normalize(bx::sub(target2, pos2));

	// fallback to Z-up if forward is too close to worldUp (camera looking straight up/down)
	const bx::Vec3 up2 = (bx::abs(bx::dot(forward, worldUp)) > 0.999f) ? bx::Vec3(0.0f, 0.0f, 1.0f) : worldUp;

	right = bx::normalize(bx::cross(up2, forward));
	up    = bx::cross(forward, right);
}

void Camera::UpdateViewProjection(uint8_t viewId, float fscreenX, float fscreenY)
{
	// view
	float view[16];
	GetViewMatrix(view);

	// projection
	const bool homoDepth = bgfx::getCaps()->homogeneousDepth;
	float      proj[16];

	if (xsettings.projection == Projection_Orthographic)
	{
		const float zoomX = fscreenX * xsettings.orthoZoom;
		const float zoomY = fscreenY * xsettings.orthoZoom;
		bx::mtxOrtho(proj, -zoomX, zoomX, -zoomY, zoomY, -1000.0f, 1000.0f, 0.0f, homoDepth);
	}
	else bx::mtxProj(proj, xsettings.fov, fscreenX / fscreenY, 0.1f, 2000.0f, homoDepth);

	bgfx::setViewTransform(0, view, proj);
}

void Camera::Zoom(float ratio)
{
	// 1) update xsettings
	xsettings.orthoZoom = bx::clamp(xsettings.orthoZoom * ratio, orthoZoomMin, orthoZoomMax);
	xsettings.distance  = xsettings.orthoZoom * xsettings.windowSize[1] / bx::tan(glm::radians(fovY) * 0.5f);

	// 2) update pos2
	distance = xsettings.distance;
	pos2     = bx::mad(forward2, -distance, target2);
	follow |= CameraFollow_Active;
}
