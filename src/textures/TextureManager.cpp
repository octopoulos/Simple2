// TextureManager.cpp
// @author octopoulos
// @version 2025-08-30

#include "stdafx.h"
#include "textures/TextureManager.h"
//
#include "imgui.h" // ImGui::

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

bgfx::TextureHandle TextureManager::AddTexture(std::string_view name, const void* data, uint32_t size)
{
	std::lock_guard<std::mutex> lock(textureMutex);

	// check cache
	if (const auto& it = textures.find(name); it != textures.end())
		return it->second.handle;

	// parse image data
	bimg::ImageContainer imageContainer;
	bx::Error            err;
	if (!bimg::imageParse(imageContainer, data, size, &err))
	{
		ui::LogError("Failed to parse texture data for {}: {}", name, err.getMessage().getCPtr());
		return BGFX_INVALID_HANDLE;
	}

	// create texture
	bgfx::TextureInfo info;
	bgfx::calcTextureSize(info, imageContainer.m_width, imageContainer.m_height, imageContainer.m_depth, imageContainer.m_cubeMap, 1 < imageContainer.m_numMips, imageContainer.m_numLayers, bgfx::TextureFormat::Enum(imageContainer.m_format));

	const bgfx::Memory* mem = bgfx::makeRef(imageContainer.m_data, imageContainer.m_size, ImageReleaseCb, &imageContainer);
	bgfx::TextureHandle handle;

	const uint16_t depth16  = TO_UINT16(imageContainer.m_depth);
	const auto     format   = bgfx::TextureFormat::Enum(imageContainer.m_format);
	const bool     hasMips  = (imageContainer.m_numMips > 1);
	const uint16_t height16 = TO_UINT16(imageContainer.m_height);
	const uint16_t width16  = TO_UINT16(imageContainer.m_width);

	if (imageContainer.m_cubeMap)
		handle = bgfx::createTextureCube(width16, hasMips, imageContainer.m_numLayers, format, 0, mem);
	else if (depth16 > 1)
		handle = bgfx::createTexture3D(width16, height16, depth16, hasMips, format, 0, mem);
	else if (bgfx::isTextureValid(0, false, imageContainer.m_numLayers, format, 0))
		handle = bgfx::createTexture2D(width16, height16, hasMips, imageContainer.m_numLayers, format, 0, mem);

	if (!bgfx::isValid(handle))
	{
		ui::LogError("Failed to create texture for {}", name);
		bimg::imageFree(&imageContainer);
		return BGFX_INVALID_HANDLE;
	}

	bgfx::setName(handle, name.data(), static_cast<int>(name.size()));
	textures.emplace(std::string(name), TextureData { handle, info });
	return handle;
}

void TextureManager::Destroy()
{
	DESTROY_GUARD();

	for (const auto& [name, texture] : textures)
		BGFX_DESTROY(texture.handle);

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

	// 1) check for cached texture
	const auto clean = NormalizeFilename(name);
	if (const auto& it = textures.find(clean); it != textures.end())
		return it->second.handle;

	// 2) find the texture by filename
	// + check a few directories
	std::filesystem::path path;
	int                   step = 0;

	auto TryPath = [&](const std::filesystem::path& cand) -> bool
	{
		ui::Log("LoadTexture/{}: Trying {}", ++step, cand.string());
		if (IsFile(cand))
		{
			path = cand;
			return true;
		};
		return false;
	};

	if (TryPath(clean))
	{
	}
	else
	{
		std::filesystem::path cleanPath  = clean;
		std::filesystem::path textureDir = "runtime/textures";

		if (TryPath(textureDir / clean))
		{
		}
		else if (cleanPath.has_relative_path() && TryPath(textureDir / cleanPath.parent_path().filename() / cleanPath.filename()))
		{
		}
		else if (TryPath(textureDir / cleanPath.filename()))
		{
		}
		else
		{
			ui::LogError("LoadTexture: Cannot find: {}", clean);
			return BGFX_INVALID_HANDLE;
		}
	}

	// 3) create the BGFX texture
	bgfx::TextureInfo   info    = {};
	bgfx::TextureHandle texture = LoadTexture_(bx::FilePath(path.string().c_str()), 0, 0, &info);
	if (!bgfx::isValid(texture))
	{
		ui::LogError("LoadTexture: Cannot load: {}", clean);
		return BGFX_INVALID_HANDLE;
	}

	// 4) cache the texture
	textures.emplace(clean, TextureData { texture, info });
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
