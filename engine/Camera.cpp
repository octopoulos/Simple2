// Camera.cpp
// @author octopoulos
// @version 2025-07-25

#include "stdafx.h"
#include "Camera.h"
#include "ui/xsettings.h"

void Camera::Initialize()
{
	orbit[0] = 0.0f;
	orbit[1] = 0.0f;
	pos      = { 0.0f, 0.0f, -0.3f };
	pos2     = { 0.0f, 0.0f, -0.3f };
	target   = { 0.0f, 0.0f, 0.0f };
	target2  = { 0.0f, 0.0f, 0.0f };
}

void Camera::ConsumeOrbit(float amount)
{
	float consume[2];
	consume[0] = orbit[0] * amount;
	consume[1] = orbit[1] * amount;
	orbit[0] -= consume[0];
	orbit[1] -= consume[1];

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

void Camera::Orbit(float dx, float dy)
{
	orbit[0] += dx;
	orbit[1] += dy;
}

void Camera::Update(float delta)
{
	const float amount = bx::min(delta * 20.0f, 1.0f);
	ConsumeOrbit(amount);

	pos     = bx::lerp(pos, pos2, amount);
	target  = bx::lerp(target, target2, amount);
	forward = bx::normalize(bx::sub(target, pos));
	right   = bx::cross(up, forward);
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
	else bx::mtxProj(proj, 60.0f, fscreenX / fscreenY, 0.1f, 2000.0f, homoDepth);

	bgfx::setViewTransform(0, view, proj);
}
