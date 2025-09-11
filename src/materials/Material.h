// Material.h
// @author octopoulos
// @version 2025-09-07

#pragma once

using sMaterial = std::shared_ptr<class Material>;

#define VALUE_VEC4(...) glm::value_ptr(glm::vec4(__VA_ARGS__))

enum AlphaModes_ : int
{
	AlphaMode_Opaque = 0,
	AlphaMode_Mask   = 1,
	AlphaMode_Blend  = 2,
};

class Material
{
public:
	// shaders + program
	std::string         fsName                = "";                  ///< fragment shader
	std::string         name                  = "";                  ///< material name
	bgfx::ProgramHandle program               = BGFX_INVALID_HANDLE; ///< compiled program
	std::string         vsName                = "";                  ///< vertex shader
	uint64_t            state                 = 0;                   ///< if !=0: override default state
	// modes
	float               alphaCutoff           = 0.5f;             ///< alpha cutoff for Mask mode
	int                 alphaMode             = AlphaMode_Opaque; ///< transparency mode
	bool                doubleSided           = false;            ///< disable back-face culling
	float               occlusionStrength     = 1.0f;             ///<
	bool                unlit                 = false;            ///< unlit material (KHR_materials_unlit)
	// PBR
	glm::vec4           baseColorFactor       = glm::vec4(1.0f); ///< PBR base color factor
	glm::vec3           emissiveFactor        = glm::vec3(0.0f); ///< emissive color factor
	float               metallicFactor        = 1.0f;            ///< PBR metallic factor
	float               roughnessFactor       = 1.0f;            ///< PBR roughness factor
	// textures
	bgfx::UniformHandle sTexColor             = BGFX_INVALID_HANDLE; ///< uniform: color texture
	bgfx::UniformHandle sTexEmissive          = BGFX_INVALID_HANDLE; ///
	bgfx::UniformHandle sTexMetallicRoughness = BGFX_INVALID_HANDLE; ///
	bgfx::UniformHandle sTexNormal            = BGFX_INVALID_HANDLE; ///< uniform: normal texture
	bgfx::UniformHandle sTexOcclusion         = BGFX_INVALID_HANDLE; ///
	std::string         texNames[8]           = {};                  ///< texture names: 0=diffuse, 1=normal
	bgfx::TextureHandle textures[8]           = {};                  ///< all found textures

	Material()
	{
		Initialize();
	}

	Material(std::string_view vsName, std::string_view fsName);

	/// Apply textures, uniforms, ...
	void Apply() const;

	/// Find and load model textures
	void FindModelTextures(std::string_view modelName, const VEC_STR& texFiles);

	/// Initialize textures and uniforms
	void Initialize();

	/// Load the shaders + compile program + remember shader names
	void LoadProgram(std::string_view vsName, std::string_view fsName);

	/// Load a color and normal texture if available
	void LoadTextures(const VEC_STR& texFiles);

	/// Serialize for JSON output
	int Serialize(fmt::memory_buffer& outString, int depth, int bounds = 3);

	/// Set PBR properties
	void SetPbrProperties(const glm::vec4& baseColor, float metallic, float roughness)
	{
		baseColorFactor = baseColor;
		metallicFactor  = metallic;
		roughnessFactor = roughness;
	}

	/// Show settings in ImGui
	/// @param show: ShowObjects_
	void ShowSettings(bool isPopup, int show);
};
