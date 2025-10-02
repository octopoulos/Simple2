// ShaderManager.cpp
// @author octopoulos
// @version 2025-09-29

#include "stdafx.h"
#include "materials/ShaderManager.h"
//
#include "common/config.h" // DEV_shader

// clang-format off
static UMAP_INT_STR RENDER_SHADERS = {
	{ bgfx::RendererType::Agc       , "pssl"  },
	{ bgfx::RendererType::Direct3D11, "dx11"  },
	{ bgfx::RendererType::Direct3D12, "dx11"  },
	{ bgfx::RendererType::Gnm       , "pssl"  },
	{ bgfx::RendererType::Metal     , "metal" },
	{ bgfx::RendererType::Noop      , "dx11"  },
	{ bgfx::RendererType::Nvn       , "nvn"   },
	{ bgfx::RendererType::OpenGL    , "glsl"  },
	{ bgfx::RendererType::OpenGLES  , "essl"  },
	{ bgfx::RendererType::Vulkan    , "spirv" },
};
// clang-format on

static std::mutex programMutex;
static std::mutex shaderMutex;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

static bgfx::ShaderHandle LoadShader_(bx::FileReaderI* reader, std::string_view name)
{
	std::filesystem::path path = "runtime/shaders";

	const auto shaderName = FindDefault(RENDER_SHADERS, bgfx::getRendererType(), "");
	if (shaderName.empty()) return BGFX_INVALID_HANDLE;

	path /= shaderName;
	path /= Format("%s.bin", Cstr(name));
	if (!IsFile(path)) return BGFX_INVALID_HANDLE;

	bgfx::ShaderHandle handle = bgfx::createShader(BgfxLoadMemory(reader, PathStr(path)));
	bgfx::setName(handle, name.data(), name.size());

	if (DEV_shader) ui::Log("LoadShader_: {} : {}", path, (void*)&handle);
	return handle;
}

static bgfx::ShaderHandle LoadShader_(std::string_view name)
{
	return LoadShader_(entry::getFileReader(), name);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
///////

void ShaderManager::Destroy()
{
	DESTROY_GUARD();

	// uniforms
	BGFX_DESTROY(uBaseColor);
	BGFX_DESTROY(uEmissive);
	BGFX_DESTROY(uMaterialFlags);
	BGFX_DESTROY(uMetallicRoughness);
	BGFX_DESTROY(uOcclusion);

	// programs
	for (auto& [name, program] : programs)
		BGFX_DESTROY(program);
	programs.clear();

	// shaders
	for (auto& [name, shader] : shaders)
		BGFX_DESTROY(shader);
	shaders.clear();
}

bgfx::ProgramHandle ShaderManager::GetProgram(std::string_view name) const
{
	std::lock_guard<std::mutex> lock(programMutex);

	if (const auto& it = programs.find(name); it != programs.end())
		return it->second;
	else
		return BGFX_INVALID_HANDLE;
}

void ShaderManager::InitializeUniforms()
{
	if (!bgfx::isValid(uBaseColor))
	{
		// clang-format off
		uBaseColor         = bgfx::createUniform("u_baseColorFactor"  , bgfx::UniformType::Vec4);
		uEmissive          = bgfx::createUniform("u_emissiveFactor"   , bgfx::UniformType::Vec4);
		uMaterialFlags     = bgfx::createUniform("u_materialFlags"    , bgfx::UniformType::Vec4);
		uMetallicRoughness = bgfx::createUniform("u_metallicRoughness", bgfx::UniformType::Vec4);
		uOcclusion         = bgfx::createUniform("u_occlusionStrength", bgfx::UniformType::Vec4);
		// clang-format on
	}
	else ui::LogError("InitializeUniforms: Already called");
}

bgfx::ProgramHandle ShaderManager::LoadProgram(std::string_view vsName, std::string_view fsName)
{
	// 1) check cache
	const std::string key = FormatStr("%s|%s", Cstr(vsName), Cstr(fsName));
	if (const auto exist = GetProgram(key); bgfx::isValid(exist)) return exist;

	// 2) load shaders
	bgfx::ShaderHandle vs = LoadShader(vsName);
	bgfx::ShaderHandle fs = LoadShader(fsName);
	if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
	{
		ui::LogError("Failed to load shaders for program: {}", key);
		return BGFX_INVALID_HANDLE;
	}

	// 3) create program
	bgfx::ProgramHandle program = bgfx::createProgram(vs, fs);
	if (!bgfx::isValid(program))
	{
		ui::LogError("Failed to create program: {}", key);
		return BGFX_INVALID_HANDLE;
	}

	std::lock_guard<std::mutex> lock(programMutex);
	programs.emplace(std::move(key), program);
	return program;
}

bgfx::ShaderHandle ShaderManager::LoadShader(std::string_view name)
{
	std::lock_guard<std::mutex> lock(shaderMutex);

	if (const auto& it = shaders.find(name); it != shaders.end())
		return it->second;

	bgfx::ShaderHandle shader = LoadShader_(name);
	if (!bgfx::isValid(shader))
	{
		ui::LogError("Failed to load shader: {}", name);
		return BGFX_INVALID_HANDLE;
	}

	shaders.emplace(std::string(name), shader);
	return shader;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

ShaderManager& GetShaderManager()
{
	static ShaderManager shaderManager;
	return shaderManager;
}
