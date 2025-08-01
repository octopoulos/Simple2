// Geometry.h
// @author octopoulos
// @version 2025-07-28

#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GEOMETRY
///////////

struct PosNormalUV
{
	float px, py, pz;
	float nx, ny, nz;
	float u, v;
};

class Geometry
{
public:
	bgfx::IndexBufferHandle  ibh    = BGFX_INVALID_HANDLE;  ///< indices
	bgfx::VertexBufferHandle vbh    = BGFX_INVALID_HANDLE;  ///< vertices
	btVector3                dims   = { 1.0f, 1.0f, 1.0f }; ///< aabb
	float                    radius = 0.0f;                 ///< bounding sphere

	Geometry() = default;

	Geometry(bgfx::VertexBufferHandle vbh, bgfx::IndexBufferHandle ibh, const btVector3& dims, float radius)
	    : vbh(vbh)
	    , ibh(ibh)
	    , dims(dims)
	    , radius(radius)
	{
	}

	~Geometry()
	{
		if (bgfx::isValid(ibh)) bgfx::destroy(ibh);
		if (bgfx::isValid(vbh)) bgfx::destroy(vbh);
	}
};

using uGeometry = std::shared_ptr<Geometry>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

uGeometry CreateBoxGeometry(float width = 1.0f, float height = 1.0f, float depth = 1.0f, int widthSegments = 1, int heightSegments = 1, int depthSegments = 1);
uGeometry CreateSphereGeometry(float radius = 1.0f, int widthSegments = 32, int heightSegments = 16, float phiStart = 0.0f, float phiLength = bx::kPi2, float thetaStart = 0.0f, float thetaLength = bx::kPi);
