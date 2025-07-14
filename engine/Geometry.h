// Geometry.h
// @author octopoulos
// @version 2025-07-05

#pragma once

class Geometry
{
public:
	bgfx::IndexBufferHandle  ibh = BGFX_INVALID_HANDLE;
	bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;

	Geometry() = default;
	~Geometry()
	{
		if (bgfx::isValid(ibh)) bgfx::destroy(ibh);
		if (bgfx::isValid(vbh)) bgfx::destroy(vbh);
	}
};
