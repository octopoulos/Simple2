// ShaderManager.cpp
// @author octopoulos
// @version 2025-07-26

#include "stdafx.h"
#include "core/ShaderManager.h"

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
// FUNCTIONS
////////////

static bgfx::ShaderHandle LoadShader_(bx::FileReaderI* reader, std::string_view name)
{
	std::filesystem::path path = "runtime/shaders";

	const auto shaderName = FindDefault(RENDER_SHADERS, bgfx::getRendererType(), "");
	if (shaderName.empty()) BX_ASSERT(false, "You should not be here!");

	path /= shaderName;
	path /= fmt::format("{}.bin", name);

	bgfx::ShaderHandle handle = bgfx::createShader(BgfxLoadMemory(reader, path.string().c_str()));
	bgfx::setName(handle, name.data(), name.size());

	ui::Log("LoadShader: {} : {}", path, (void*)&handle);
	return handle;
}

static bgfx::ShaderHandle LoadShader_(std::string_view name)
{
	return LoadShader_(entry::getFileReader(), name);
}

static bgfx::ProgramHandle LoadProgram_(bx::FileReaderI* reader, std::string_view vsName, std::string_view fsName)
{
	bgfx::ShaderHandle vsh = LoadShader_(reader, vsName);
	bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
	if (fsName.size()) fsh = LoadShader_(reader, fsName);

	return bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);
}

static bgfx::ProgramHandle LoadProgram_(std::string_view vsName, std::string_view fsName)
{
	return LoadProgram_(entry::getFileReader(), vsName, fsName);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
///////

void ShaderManager::Destroy()
{
	for (auto& [name, program] : programs)
		if (bgfx::isValid(program)) bgfx::destroy(program);
	programs.clear();

	for (auto& [name, shader] : shaders)
		if (bgfx::isValid(shader)) bgfx::destroy(shader);
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

bgfx::ProgramHandle ShaderManager::LoadProgram(std::string_view vsName, std::string_view fsName)
{
	std::lock_guard<std::mutex> lock(programMutex);

	std::string key = fmt::format("{}|{}", vsName, fsName);
	if (const auto& it = programs.find(key); it != programs.end())
		return it->second;

	bgfx::ShaderHandle vs = LoadShader(vsName);
	bgfx::ShaderHandle fs = LoadShader(fsName);
	if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
	{
		ui::LogError("Failed to load shaders for program: {}", key);
		return BGFX_INVALID_HANDLE;
	}

	bgfx::ProgramHandle program = bgfx::createProgram(vs, fs);
	if (!bgfx::isValid(program))
	{
		ui::LogError("Failed to create program: {}", key);
		return BGFX_INVALID_HANDLE;
	}

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

ShaderManager& GetShaderManager()
{
	static ShaderManager shaderManager;
	return shaderManager;
}
