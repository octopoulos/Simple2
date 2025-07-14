// Material.h
// @author octopoulos
// @version 2025-07-05

#pragma once

class Material
{
public:
	bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;

	Material() = default;
	explicit Material(bgfx::ProgramHandle p) : program(p) {}

	void Apply() const
	{
		// Set uniforms, textures, etc. if needed
		assert(bgfx::isValid(program));
	}
};
