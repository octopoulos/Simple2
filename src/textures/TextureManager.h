// TextureManager.h
// @author octopoulos
// @version 2025-10-07

#pragma once

enum TextureTypes_ : int
{
	TextureType_Diffuse    = 0,
	TextureType_Normal     = 1,
	TextureType_Specular   = 2,
	TextureType_Shininess  = 3,
	TextureType_Ambient    = 4,
	TextureType_Emissive   = 5,
	TextureType_Reflection = 6,
	TextureType_Count      = 7,
};

struct TextureData
{
	int                 type   = 0;  ///< TextureTypes_
	std::string         name   = ""; ///< texture name
	bgfx::TextureHandle handle = {}; ///< bgfx handle
	bgfx::TextureInfo   info   = {}; ///< info (dimensions etc)
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TextureManager
/////////////////

class TextureManager
{
private:
	bool                  destroyed = false; ///< Destroy already called?
	UMAP_STR<TextureData> textures  = {};    ///< cached textures

public:
	TextureManager() = default;

	~TextureManager() { Destroy(); }

	/// Add a texture from raw data
	/// @returns valid texture handle or BGFX_INVALID_HANDLE.
	bgfx::TextureHandle AddRawTexture(std::string_view name, const void* data, uint32_t size);

	/// Release all textures
	void Destroy();

	/// Get texture info by name
	const bgfx::TextureInfo* GetTextureInfo(std::string_view name) const;

	/// Load or retrieve a texture by name
	/// @returns valid texture handle or BGFX_INVALID_HANDLE.
	bgfx::TextureHandle LoadTexture(std::string_view name, const std::filesystem::path& startDir = {});

	/// Show textures in ImGui
	void ShowInfoTable(bool showTitle = true) const;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

TextureManager& GetTextureManager();

/// Convert texture type: int to string
std::string TextureName(int type);

/// Convert texture type: string to int
int TextureType(std::string_view name);
