/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#ifndef BGFX_UTILS_H_HEADER_GUARD
#define BGFX_UTILS_H_HEADER_GUARD

#include <bx/bounds.h>
#include <bx/pixelformat.h>
#include <bx/filepath.h>
#include <bgfx/bgfx.h>
#include <bimg/bimg.h>

#include "AI/stdafx.h"
#include "engine/Mesh.h"

#include <vector>

// #include <tinystl/allocator.h>
// #include <tinystl/vector.h>
// namespace stl = tinystl;


///
void* load(const bx::FilePath& _filePath, uint32_t* _size = NULL);

///
void unload(void* _ptr);

///
bgfx::ShaderHandle loadShader(const bx::StringView& _name);

///
bgfx::ProgramHandle loadProgram(const bx::StringView& _vsName, const bx::StringView& _fsName);

///
bgfx::TextureHandle loadTexture(const bx::FilePath& _filePath, uint64_t _flags = BGFX_TEXTURE_NONE|BGFX_SAMPLER_NONE, uint8_t _skip = 0, bgfx::TextureInfo* _info = NULL, bimg::Orientation::Enum* _orientation = NULL);

///
bimg::ImageContainer* imageLoad(const bx::FilePath& _filePath, bgfx::TextureFormat::Enum _dstFormat);

///
void calcTangents(void* _vertices, uint16_t _numVertices, bgfx::VertexLayout _layout, const uint16_t* _indices, uint32_t _numIndices);

/// Returns true if both internal transient index and vertex buffer have
/// enough space.
///
/// @param[in] _numVertices Number of vertices.
/// @param[in] _layout Vertex layout.
/// @param[in] _numIndices Number of indices.
///
inline bool checkAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexLayout& _layout, uint32_t _numIndices)
{
	return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _layout)
		&& (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices) )
		;
}

///
inline uint32_t encodeNormalRgba8(float _x, float _y = 0.0f, float _z = 0.0f, float _w = 0.0f)
{
	const float src[] =
	{
		_x * 0.5f + 0.5f,
		_y * 0.5f + 0.5f,
		_z * 0.5f + 0.5f,
		_w * 0.5f + 0.5f,
	};
	uint32_t dst;
	bx::packRgba8(&dst, src);
	return dst;
}

#if 0
///
struct MeshState
{
	struct Texture
	{
		uint32_t            m_flags;
		bgfx::UniformHandle m_sampler;
		bgfx::TextureHandle m_texture;
		uint8_t             m_stage;
	};

	Texture             m_textures[4];
	uint64_t            m_state;
	bgfx::ProgramHandle m_program;
	uint8_t             m_numTextures;
	uint16_t            m_viewId;
};

struct Primitive
{
	uint32_t m_startIndex;
	uint32_t m_numIndices;
	uint32_t m_startVertex;
	uint32_t m_numVertices;

	bx::Sphere m_sphere;
	bx::Aabb   m_aabb;
	bx::Obb    m_obb;
};

struct Group
{
	Group();
	void Reset();

	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh;
	uint16_t m_numVertices;
	uint8_t* m_vertices;
	uint32_t m_numIndices;
	uint16_t* m_indices;
	bx::Sphere m_sphere;
	bx::Aabb   m_aabb;
	bx::Obb    m_obb;
	std::vector<Primitive> m_prims;
};
#endif

struct Mesh2
{
	void Destroy();
	void Load(bx::ReaderSeekerI* reader, bool ramcopy);
	void Submit(uint16_t id, bgfx::ProgramHandle program, const float* mtx, uint64_t state) const;
	void Submit(const MeshState* const* state, uint8_t numPasses, const float* mtx, uint16_t numMatrices) const;

	bgfx::VertexLayout  layout;
	std::vector<Group> groups;
};

///
Mesh2* meshLoad(const bx::FilePath& filePath, bool ramcopy = false);

///
void meshUnload(Mesh2* mesh);

///
MeshState* meshStateCreate();

///
void meshStateDestroy(MeshState* meshState);

///
void meshSubmit(const Mesh2* mesh, bgfx::ViewId id, bgfx::ProgramHandle program, const float* mtx, uint64_t state = BGFX_STATE_MASK);

///
void meshSubmit(const Mesh2* mesh, const MeshState* const* state, uint8_t numPasses, const float* mtx, uint16_t numMatrices = 1);

/// bgfx::RendererType::Enum to name.
bx::StringView getName(bgfx::RendererType::Enum _type);

/// Name to bgfx::RendererType::Enum.
bgfx::RendererType::Enum getType(const bx::StringView& _name);

///
struct Args
{
	Args(int _argc, const char* const* _argv);

	bgfx::RendererType::Enum m_type;
	uint16_t m_pciId;
};

#endif // BGFX_UTILS_H_HEADER_GUARD
