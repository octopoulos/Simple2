// ShaderManager.h
// @author octopoulos
// @version 2025-07-26

#pragma once

class ShaderManager
{
private:
	UMAP_STR<bgfx::ProgramHandle> programs = {}; /// cached programs
	UMAP_STR<bgfx::ShaderHandle>  shaders  = {}; /// cached shaders

public:
	ShaderManager() = default;

	~ShaderManager() { Destroy(); }

	/// Release all loaded shaders and programs
	void Destroy();

	/// Retrieve a loaded program by key
	/// @param name "vsName|fsName"
	/// @returns valid program handle or BGFX_INVALID_HANDLE
	bgfx::ProgramHandle GetProgram(std::string_view name) const;

	/// Load or retrieve a program composed of vertex + fragment shaders
	/// + caches program internally
	/// @returns valid program handle or BGFX_INVALID_HANDLE
	bgfx::ProgramHandle LoadProgram(std::string_view vsName, std::string_view fsName);

	/// Load or retrieve a shader by name
	/// + caches shader internally
	/// @returns valid shader handle or BGFX_INVALID_HANDLE.
	bgfx::ShaderHandle LoadShader(std::string_view name);
};

ShaderManager& GetShaderManager();
