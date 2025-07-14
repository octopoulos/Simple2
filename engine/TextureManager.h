// TextureManager.h
// @author octopoulos
// @version 2025-07-05

#pragma once

#include <bimg/bimg.h>

class TextureManager
{
private:
	UMAP_STR<bgfx::TextureHandle> textures;

public:
	TextureManager() = default;
	~TextureManager() { Destroy(); }

	/// Release all loaded shaders and programs
	void Destroy();

	/// Load or retrieve a texture by name
	/// @returns valid texture handle or BGFX_INVALID_HANDLE.
	bgfx::TextureHandle LoadTexture(std::string_view name);
};
