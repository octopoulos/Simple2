// Material.h
// @author octopoulos
// @version 2025-08-04

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

	/// Serialize for JSON output
	int Serialize(fmt::memory_buffer& outString, int bounds = 3);
};
