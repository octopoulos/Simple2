// TextureManager.cpp
// @author octopoulos
// @version 2025-08-25

#include "stdafx.h"
#include "textures/TextureManager.h"
//
#include "common/imgui/imgui.h"

#include <bimg/decode.h>

static std::mutex textureMutex;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

static void ImageReleaseCb(void* _ptr, void* _userData)
{
	BX_UNUSED(_ptr);
	bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
	bimg::imageFree(imageContainer);
}

static bgfx::TextureHandle LoadTexture_(bx::FileReaderI* _reader, const bx::FilePath& _filePath, uint64_t _flags, uint8_t _skip, bgfx::TextureInfo* _info, bimg::Orientation::Enum* _orientation)
{
	BX_UNUSED(_skip);
	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

	uint32_t size;
	void*    data = BgfxLoad(_reader, entry::getAllocator(), _filePath, &size);
	if (data)
	{
		bimg::ImageContainer* imageContainer = bimg::imageParse(entry::getAllocator(), data, size);
		if (imageContainer)
		{
			if (_orientation) *_orientation = imageContainer->m_orientation;

			const bgfx::Memory* mem = bgfx::makeRef(imageContainer->m_data, imageContainer->m_size, ImageReleaseCb, imageContainer);
			BgfxUnload(data);

			if (_info) bgfx::calcTextureSize(*_info, uint16_t(imageContainer->m_width), uint16_t(imageContainer->m_height), uint16_t(imageContainer->m_depth), imageContainer->m_cubeMap, 1 < imageContainer->m_numMips, imageContainer->m_numLayers, bgfx::TextureFormat::Enum(imageContainer->m_format));

			if (imageContainer->m_cubeMap)
				handle = bgfx::createTextureCube(uint16_t(imageContainer->m_width), 1 < imageContainer->m_numMips, imageContainer->m_numLayers, bgfx::TextureFormat::Enum(imageContainer->m_format), _flags, mem);
			else if (1 < imageContainer->m_depth)
				handle = bgfx::createTexture3D(uint16_t(imageContainer->m_width), uint16_t(imageContainer->m_height), uint16_t(imageContainer->m_depth), 1 < imageContainer->m_numMips, bgfx::TextureFormat::Enum(imageContainer->m_format), _flags, mem);
			else if (bgfx::isTextureValid(0, false, imageContainer->m_numLayers, bgfx::TextureFormat::Enum(imageContainer->m_format), _flags))
				handle = bgfx::createTexture2D(uint16_t(imageContainer->m_width), uint16_t(imageContainer->m_height), 1 < imageContainer->m_numMips, imageContainer->m_numLayers, bgfx::TextureFormat::Enum(imageContainer->m_format), _flags, mem);

			if (bgfx::isValid(handle))
			{
				const bx::StringView name(_filePath);
				bgfx::setName(handle, name.getPtr(), name.getLength());
			}
		}
	}

	return handle;
}

static bgfx::TextureHandle LoadTexture_(const bx::FilePath& _filePath, uint64_t _flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, uint8_t _skip = 0, bgfx::TextureInfo* _info = nullptr, bimg::Orientation::Enum* _orientation = nullptr)
{
	return LoadTexture_(entry::getFileReader(), _filePath, _flags, _skip, _info, _orientation);
}

static bimg::ImageContainer* ImageLoad_(const bx::FilePath& _filePath, bgfx::TextureFormat::Enum _dstFormat)
{
	uint32_t size = 0;
	void*    data = BgfxLoadMemory(entry::getFileReader(), entry::getAllocator(), _filePath, &size);

	return bimg::imageParse(entry::getAllocator(), data, size, bimg::TextureFormat::Enum(_dstFormat));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
///////

void TextureManager::Destroy()
{
	for (const auto& [name, texture] : textures)
		bgfx::destroy(texture.handle);

	textures.clear();
}

const bgfx::TextureInfo* TextureManager::GetTextureInfo(std::string_view name) const
{
	std::lock_guard<std::mutex> lock(textureMutex);

	if (const auto& it = textures.find(name); it != textures.end())
		return &it->second.info;
	return nullptr;
}

bgfx::TextureHandle TextureManager::LoadTexture(std::string_view name)
{
	std::lock_guard<std::mutex> lock(textureMutex);

	if (const auto& it = textures.find(name); it != textures.end())
		return it->second.handle;

	std::string path;
	if (!IsFile(name))// && name.find('/') == std::string_view::npos)
		path = fmt::format("runtime/textures/{}", name);
	else
		path = std::string(name);

	bgfx::TextureInfo   info    = {};
	bgfx::TextureHandle texture = LoadTexture_(bx::FilePath(path.c_str()), 0, 0, &info);
	if (!bgfx::isValid(texture))
	{
		ui::LogError("Failed to load texture: {}", name);
		return BGFX_INVALID_HANDLE;
	}

	textures.emplace(std::string(name), TextureData { texture, info });
	return texture;
}

void TextureManager::ShowTable()
{
	if (ImGui::BeginTable("2ways", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti))
	{
		const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;

		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 8.0f);
		ImGui::TableHeadersRow();

		// sort data?
		if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs())
			if (sortSpecs->SpecsDirty)
			{
				ui::Log("TextureManager:Sort");
				sortSpecs->SpecsDirty = false;
			}

		for (const auto& [name, value] : textures)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(name.c_str());
			ImGui::TableNextColumn();
			const auto& info = value.info;
			ImGui::Text("%dx%d", info.width, info.height);
		}
		ImGui::EndTable();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

TextureManager& GetTextureManager()
{
	static TextureManager textureManager;
	return textureManager;
}
