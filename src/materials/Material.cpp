// Material.cpp
// @author octopoulos
// @version 2025-08-27

#include "stdafx.h"
#include "materials/Material.h"
//
#include "loaders/writer.h"          // WRITE_INIT, WRITE_KEY_xxx
#include "materials/ShaderManager.h" // GetShaderManager
#include "textures/TextureManager.h" // GetTextureManager

Material::Material(std::string_view vsName, std::string_view fsName)
{
	LoadProgram(vsName, fsName);
}

void Material::Apply() const
{
	assert(bgfx::isValid(program));

	// set textures
	// clang-format off
	if (bgfx::isValid(texColor))             bgfx::setTexture(0, sTexColor            , texColor);
	if (bgfx::isValid(texNormal))            bgfx::setTexture(1, sTexNormal           , texNormal);
	if (bgfx::isValid(texMetallicRoughness)) bgfx::setTexture(2, sTexMetallicRoughness, texMetallicRoughness);
	if (bgfx::isValid(texEmissive))          bgfx::setTexture(3, sTexEmissive         , texEmissive);
	if (bgfx::isValid(texOcclusion))         bgfx::setTexture(4, sTexOcclusion        , texOcclusion);
	// clang-format on

	// set PBR uniforms
	const ShaderManager& shaderManager = GetShaderManager();
	// clang-format off
	bgfx::setUniform(shaderManager.uBaseColor        , VALUE_VEC4(baseColorFactor));
	bgfx::setUniform(shaderManager.uEmissive         , VALUE_VEC4(emissiveFactor, 1.0f));
	bgfx::setUniform(shaderManager.uMaterialFlags    , VALUE_VEC4(unlit ? 1.0f : 0.0f, TO_FLOAT(alphaMode), alphaCutoff, 0.0f));
	bgfx::setUniform(shaderManager.uMetallicRoughness, VALUE_VEC4(metallicFactor, roughnessFactor, 0.0f, 0.0f));
	bgfx::setUniform(shaderManager.uOcclusion        , VALUE_VEC4(occlusionStrength, 0.0f, 0.0f, 0.0f));
	// clang-format on
}

void Material::FindModelTextures(std::string_view modelName, std::string_view textureName)
{
	// 1) texture is specified
	if (textureName.size())
	{
		const auto& texture = GetTextureManager().LoadTexture(textureName);
		if (bgfx::isValid(texture)) textures.push_back(texture);
	}
	// 2) scan directory to find textures
	else
	{
		const auto parent      = std::filesystem::path(modelName).parent_path();
		const auto texturePath = std::filesystem::path("runtime/textures") / parent;

		if (!parent.empty() && IsDirectory(texturePath))
		{
			VEC_STR texNames;
			for (const auto& dirEntry : std::filesystem::directory_iterator { texturePath })
			{
				const auto& path     = dirEntry.path();
				const auto  filename = path.filename();
				const auto  texName  = fmt::format("{}/{}", parent.string(), filename.string());
				texNames.push_back(texName);
			}

			// load all texture variations
			if (const int size = TO_INT(texNames.size()))
			{
				for (int id = -1; const auto& texName : texNames)
				{
					const auto& texture = GetTextureManager().LoadTexture(texName);
					if (bgfx::isValid(texture)) textures.push_back(texture);
					ui::Log("=> {} {} {}", id, texName, bgfx::isValid(texture));
				}
			}
		}
	}

	// 3) assign current texture
	if (textures.size()) texColor = textures[0];

}

void Material::LoadProgram(std::string_view vsName, std::string_view fsName)
{
	this->fsName = fsName;
	this->vsName = vsName;
	program      = GetShaderManager().LoadProgram(vsName, fsName);

	// create uniforms
	// clang-format off
	sTexColor  = bgfx::createUniform("s_texColor" , bgfx::UniformType::Sampler);
	sTexNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
	// clang-format on
}

void Material::LoadTextures(std::string_view colorName, std::string_view normalName)
{
	if (colorName.size())
	{
		texColor    = GetTextureManager().LoadTexture(colorName);
		texNames[0] = colorName;
	}
	if (normalName.size())
	{
		texNormal   = GetTextureManager().LoadTexture(normalName);
		texNames[1] = normalName;
	}
}

int Material::Serialize(fmt::memory_buffer& outString, int depth, int bounds)
{
	if (bounds & 1) WRITE_CHAR('{');
	WRITE_INIT();

	// 1) shaders + program
	WRITE_KEY_STRING(fsName);
	WRITE_KEY_STRING(vsName);
	if (state) WRITE_KEY_INT(state);

	// 2) modes
	if (alphaMode)
	{
		if (alphaMode == AlphaMode_Mask) WRITE_KEY_FLOAT(alphaCutoff);
		WRITE_KEY_INT(alphaMode);
	}
	if (doubleSided) WRITE_KEY_BOOL(doubleSided);
	if (unlit) WRITE_KEY_BOOL(unlit);

	// 3) PBR
	WRITE_KEY_VEC4(baseColorFactor);
	WRITE_KEY_VEC3(emissiveFactor);
	WRITE_KEY_FLOAT(metallicFactor);
	WRITE_KEY_FLOAT(roughnessFactor);

	// 4) textures
	{
		int texNum = 4;
		while (texNum > 0 && texNames[texNum - 1].empty()) --texNum;
		if (texNum > 0)
		{
			WRITE_KEY("texNames");
			WRITE_CHAR('[');
			for (int i = 0; i < texNum; ++i)
			{
				if (i > 0) WRITE_CHAR(',');
				WRITE_JSON_STRING(texNames[i]);
			}
			WRITE_CHAR(']');
		}
	}

	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}
