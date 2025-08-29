// Material.cpp
// @author octopoulos
// @version 2025-08-25

#include "stdafx.h"
#include "materials/Material.h"
//
#include "core/ShaderManager.h"
#include "loaders/writer.h"
#include "textures/TextureManager.h"

Material::Material(std::string_view vsName, std::string_view fsName)
{
	LoadProgram(vsName, fsName);
}

void Material::Apply() const
{
	assert(bgfx::isValid(program));
	if (bgfx::isValid(texColor)) bgfx::setTexture(0, sTexColor, texColor);
	if (bgfx::isValid(texNormal)) bgfx::setTexture(1, sTexNormal, texNormal);
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
	sTexColor  = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	sTexNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
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
	WRITE_KEY_STRING(fsName);
	WRITE_KEY_STRING(vsName);

	// textures
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
