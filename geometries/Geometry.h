// Geometry.h
// @author octopoulos
// @version 2025-07-21

#pragma once

class Geometry
{
public:
	bgfx::IndexBufferHandle  ibh = BGFX_INVALID_HANDLE;
	bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;

	Geometry() = default;

	Geometry(bgfx::VertexBufferHandle vbh, bgfx::IndexBufferHandle ibh)
	    : vbh(vbh)
	    , ibh(ibh)
	{
	}
	~Geometry()
	{
		//if (bgfx::isValid(ibh)) bgfx::destroy(ibh);
		//if (bgfx::isValid(vbh)) bgfx::destroy(vbh);
	}
};
