// Material.h
// @author octopoulos
// @version 2025-08-25

#pragma once

using sMaterial = std::shared_ptr<class Material>;

class Material
{
public:
	std::string              fsName      = "";                  ///< fragment shader
	std::string              name        = "";                  ///< material name
	bgfx::ProgramHandle      program     = BGFX_INVALID_HANDLE; ///< compiled program
	uint64_t                 state       = 0;                   ///< if !=0: override default state
	bgfx::UniformHandle      sTexColor   = BGFX_INVALID_HANDLE; ///< uniform: color texture
	bgfx::UniformHandle      sTexNormal  = BGFX_INVALID_HANDLE; ///< uniform: normal texture
	bgfx::TextureHandle      texColor    = BGFX_INVALID_HANDLE; ///< texture: color
	std::string              texNames[4] = {};                  ///< texture names: 0=diffuse, 1=normal
	bgfx::TextureHandle      texNormal   = BGFX_INVALID_HANDLE; ///< texture: normal
	VEC<bgfx::TextureHandle> textures    = {};                  ///< all found textures
	std::string              vsName      = "";                  ///< vertex shader

	Material() = default;

	Material(std::string_view vsName, std::string_view fsName);

	/// Apply textures, uniforms, ...
	void Apply() const;

	/// Find and load model textures
	void FindModelTextures(std::string_view modelName, std::string_view textureName);

	/// Load the shaders + compile program + remember shader names
	void LoadProgram(std::string_view vsName, std::string_view fsName);

	/// Load a color and normal texture if available
	void LoadTextures(std::string_view colorName, std::string_view normalName = "");

	/// Serialize for JSON output
	int Serialize(fmt::memory_buffer& outString, int depth, int bounds = 3);
};
