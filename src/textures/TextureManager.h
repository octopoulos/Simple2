// TextureManager.h
// @author octopoulos
// @version 2025-08-27

#pragma once

#include <bimg/bimg.h>

struct TextureData
{
	bgfx::TextureHandle handle = {}; ///< bgfx handle
	bgfx::TextureInfo   info   = {}; ///< info (dimensions etc)
};

class TextureManager
{
private:
	bool                  destroyed = false; ///< Destroy already called?
	UMAP_STR<TextureData> textures  = {};    ///< cached textures

public:
	TextureManager() = default;

	~TextureManager() { Destroy(); }

	/// Release all textures
	void Destroy();

	/// Get texture info by name
	const bgfx::TextureInfo* GetTextureInfo(std::string_view name) const;

	/// Load or retrieve a texture by name
	/// @returns valid texture handle or BGFX_INVALID_HANDLE.
	bgfx::TextureHandle LoadTexture(std::string_view name);

	/// Show textures in ImGui
	void ShowTable();
};

TextureManager& GetTextureManager();
