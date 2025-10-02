// Material.cpp
// @author octopoulos
// @version 2025-09-29

#include "stdafx.h"
#include "materials/Material.h"
//
#include "app/App.h"                 // App
#include "loaders/writer.h"          // WRITE_INIT, WRITE_KEY_xxx
#include "materials/ShaderManager.h" // GetShaderManager
#include "objects/Object3d.h"        // ShowObject_xxx
#include "textures/TextureManager.h" // GetTextureManager
#include "ui/ui.h"                   // ui::

Material::Material(std::string_view vsName, std::string_view fsName)
{
	Initialize();
	LoadProgram(vsName, fsName);
}

void Material::Apply() const
{
	assert(bgfx::isValid(program));

	// set textures
	// clang-format off
	if (bgfx::isValid(textures[0])) bgfx::setTexture(0, sTexColor            , textures[0]);
	if (bgfx::isValid(textures[1])) bgfx::setTexture(1, sTexNormal           , textures[1]);
	if (bgfx::isValid(textures[2])) bgfx::setTexture(2, sTexMetallicRoughness, textures[2]);
	if (bgfx::isValid(textures[3])) bgfx::setTexture(3, sTexEmissive         , textures[3]);
	if (bgfx::isValid(textures[4])) bgfx::setTexture(4, sTexOcclusion        , textures[4]);
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

void Material::FindModelTextures(std::string_view modelName, const VEC_STR& texFiles)
{
	// 1) textures are specified
	if (texFiles.size())
		LoadTextures(texFiles);
	// 2) scan directory to find textures
	else
	{
		const auto parent      = std::filesystem::path(modelName).parent_path();
		const auto texturePath = std::filesystem::path("runtime/textures") / parent;

		if (!parent.empty() && IsDirectory(texturePath))
		{
			VEC_STR texFiles;
			for (const auto& dirEntry : std::filesystem::directory_iterator { texturePath })
			{
				const auto& path     = dirEntry.path();
				const auto  filename = path.filename();
				std::string texFile  = Format("%s/%s", PathStr(parent), PathStr(filename));
				texFiles.push_back(std::move(texFile));
			}

			// load all texture variations
			if (texFiles.size())
			{
				for (int id = -1; const auto& texFile : texFiles)
				{
					++id;
					const auto& texture = GetTextureManager().LoadTexture(texFile);
					if (bgfx::isValid(texture))
					{
						texNames[id] = texFile;
						textures[id] = texture;
					}
					ui::Log("=> {} {} {}", id, texFile, bgfx::isValid(texture));
				}
			}
		}
	}
}

void Material::Initialize()
{
	for (int i = 0; i < 8; ++i)
		textures[i] = BGFX_INVALID_HANDLE;

	// create uniforms
	// clang-format off
	sTexColor             = bgfx::createUniform("s_texColor" , bgfx::UniformType::Sampler);
	sTexNormal            = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
	sTexMetallicRoughness = bgfx::createUniform("s_texColor" , bgfx::UniformType::Sampler);
	sTexEmissive          = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
	sTexOcclusion         = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
	// clang-format on
}

void Material::LoadProgram(std::string_view vsName, std::string_view fsName)
{
	this->fsName = fsName;
	this->vsName = vsName;
	program      = GetShaderManager().LoadProgram(vsName, fsName);
}

void Material::LoadTexture(int texId, std::string_view name)
{
	if (name.size())
	{
		const auto proxy = RelativeName(name, { "runtime/textures" });
		texNames[texId]  = proxy;
		textures[texId]  = GetTextureManager().LoadTexture(proxy);
	}
}

void Material::LoadTextures(const VEC_STR& texFiles)
{
	for (int id = -1; const auto& texFile : texFiles)
		LoadTexture(++id, texFile);
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

void Material::ShowInfoTable(bool showTitle) const
{
	if (showTitle) ImGui::TextUnformatted("Material");

	// clang-format off
	ui::ShowTable({
		{ "state", std::to_string(state) },
	});
	// clang-format on
}

void Material::ShowSettings(bool isPopup, int show)
{
	int mode = 3;
	if (isPopup) mode |= 4;

	auto app = App::GetApp();
	if (!app) return;

	// shaders
	if (show & ShowObject_MaterialShaders)
	{
		ui::AddInputText(mode, ".name", "Name", 256, 0, &name);
		ui::AddInputText(mode | 32, ".vsName", "Vertex Shader", 256, 0, &vsName);
		ImGui::SameLine();
		if (ImGui::Button(Format("...##vert%s", Cstr(vsName))))
		{
			ui::Log("Vertex{}", vsName);
			app->OpenFile(OpenAction_ShaderVert);
		}
		ui::AddInputText(mode | 32, ".fsName", "Fragment Shader", 256, 0, &fsName);
		ImGui::SameLine();
		if (ImGui::Button(Format("...##frag%s", Cstr(fsName))))
		{
			ui::Log("Fragment{}", fsName);
			app->OpenFile(OpenAction_ShaderFrag);
		}
	}

	// textures
	if (show & ShowObject_MaterialTextures)
	{
		for (int id = 0; id < TextureType_Count; ++id)
		{
			ui::AddInputText(mode | 32, Format(".textName:%d", id), TextureName(id).c_str(), 256, 0, &texNames[id]);
			ImGui::SameLine();
			if (ImGui::Button(Format("...##Tex%d", id)))
			{
				ui::Log("Tex{} {}", id, TextureName(id));
				app->OpenFile(OpenAction_Image, id);
			}
		}
	}
}
