// Material.h
// @author octopoulos
// @version 2025-08-23

#pragma once

class Material
{
public:
	std::string         fsName  = "";                  ///< fragment shader
	std::string         vsName  = "";                  ///< vertex shader
	bgfx::ProgramHandle program = BGFX_INVALID_HANDLE; ///< compiled program

	Material() = default;

	Material(std::string_view vsName, std::string_view fsName);

	void Apply() const
	{
		// Set uniforms, textures, etc. if needed
		assert(bgfx::isValid(program));
	}

	/// Load the shaders + compile program + remember shader names
	void LoadProgram(std::string_view vsName, std::string_view fsName);

	/// Serialize for JSON output
	int Serialize(fmt::memory_buffer& outString, int depth, int bounds = 3);
};
