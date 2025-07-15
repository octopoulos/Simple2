// Mesh.cpp
// @author octopoulos
// @version 2025-07-11

#include "stdafx.h"
#include "Mesh.h"

#include <meshoptimizer.h>

namespace bgfx
{
	int32_t read(bx::ReaderI* reader, bgfx::VertexLayout& layout, bx::Error* err);
}

void Mesh::Destroy()
{
	bx::AllocatorI* allocator = entry::getAllocator();
	for (const auto& group : groups)
	{
		bgfx::destroy(group.m_vbh);
		if (bgfx::isValid(group.m_ibh)) bgfx::destroy(group.m_ibh);

		if (group.m_vertices) bx::free(allocator, group.m_vertices);
		if (group.m_indices) bx::free(allocator, group.m_indices);
	}
	groups.clear();
}

void Mesh::Load(bx::ReaderSeekerI* reader, bool ramcopy)
{
	constexpr uint32_t kChunkVertexBuffer           = BX_MAKEFOURCC('V', 'B', ' ', 0x1);
	constexpr uint32_t kChunkVertexBufferCompressed = BX_MAKEFOURCC('V', 'B', 'C', 0x0);
	constexpr uint32_t kChunkIndexBuffer            = BX_MAKEFOURCC('I', 'B', ' ', 0x0);
	constexpr uint32_t kChunkIndexBufferCompressed  = BX_MAKEFOURCC('I', 'B', 'C', 0x1);
	constexpr uint32_t kChunkPrimitive              = BX_MAKEFOURCC('P', 'R', 'I', 0x0);

	Group group;

	bx::AllocatorI* allocator = entry::getAllocator();

	uint32_t  chunk;
	bx::Error err;
	while (bx::read(reader, chunk, &err) == 4 && err.isOk())
	{
		switch (chunk)
		{
		case kChunkVertexBuffer:
		{
			read(reader, group.m_sphere, &err);
			read(reader, group.m_aabb, &err);
			read(reader, group.m_obb, &err);

			read(reader, layout, &err);

			uint16_t stride = layout.getStride();

			read(reader, group.m_numVertices, &err);
			const bgfx::Memory* mem = bgfx::alloc(group.m_numVertices * stride);
			read(reader, mem->data, mem->size, &err);

			if (ramcopy)
			{
				group.m_vertices = (uint8_t*)bx::alloc(allocator, group.m_numVertices * stride);
				bx::memCopy(group.m_vertices, mem->data, mem->size);
			}

			group.m_vbh = bgfx::createVertexBuffer(mem, layout);
		}
		break;

		case kChunkVertexBufferCompressed:
		{
			read(reader, group.m_sphere, &err);
			read(reader, group.m_aabb, &err);
			read(reader, group.m_obb, &err);

			read(reader, layout, &err);

			uint16_t stride = layout.getStride();

			read(reader, group.m_numVertices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.m_numVertices * stride);

			uint32_t compressedSize;
			bx::read(reader, compressedSize, &err);

			void* compressedVertices = bx::alloc(allocator, compressedSize);
			bx::read(reader, compressedVertices, compressedSize, &err);

			meshopt_decodeVertexBuffer(mem->data, group.m_numVertices, stride, (uint8_t*)compressedVertices, compressedSize);

			bx::free(allocator, compressedVertices);

			if (ramcopy)
			{
				group.m_vertices = (uint8_t*)bx::alloc(allocator, group.m_numVertices * stride);
				bx::memCopy(group.m_vertices, mem->data, mem->size);
			}

			group.m_vbh = bgfx::createVertexBuffer(mem, layout);
		}
		break;

		case kChunkIndexBuffer:
		{
			read(reader, group.m_numIndices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.m_numIndices * 2);
			read(reader, mem->data, mem->size, &err);

			if (ramcopy)
			{
				group.m_indices = (uint16_t*)bx::alloc(allocator, group.m_numIndices * 2);
				bx::memCopy(group.m_indices, mem->data, mem->size);
			}

			group.m_ibh = bgfx::createIndexBuffer(mem);
		}
		break;

		case kChunkIndexBufferCompressed:
		{
			bx::read(reader, group.m_numIndices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.m_numIndices * 2);

			uint32_t compressedSize;
			bx::read(reader, compressedSize, &err);

			void* compressedIndices = bx::alloc(allocator, compressedSize);

			bx::read(reader, compressedIndices, compressedSize, &err);

			meshopt_decodeIndexBuffer(mem->data, group.m_numIndices, 2, (uint8_t*)compressedIndices, compressedSize);

			bx::free(allocator, compressedIndices);

			if (ramcopy)
			{
				group.m_indices = (uint16_t*)bx::alloc(allocator, group.m_numIndices * 2);
				bx::memCopy(group.m_indices, mem->data, mem->size);
			}

			group.m_ibh = bgfx::createIndexBuffer(mem);
		}
		break;

		case kChunkPrimitive:
		{
			uint16_t len;
			read(reader, len, &err);

			std::string material;
			material.resize(len);
			read(reader, const_cast<char*>(material.c_str()), len, &err);

			uint16_t num;
			read(reader, num, &err);

			for (uint32_t ii = 0; ii < num; ++ii)
			{
				read(reader, len, &err);

				std::string name;
				name.resize(len);
				read(reader, const_cast<char*>(name.c_str()), len, &err);

				Primitive prim;
				read(reader, prim.m_startIndex, &err);
				read(reader, prim.m_numIndices, &err);
				read(reader, prim.m_startVertex, &err);
				read(reader, prim.m_numVertices, &err);
				read(reader, prim.m_sphere, &err);
				read(reader, prim.m_aabb, &err);
				read(reader, prim.m_obb, &err);

				group.m_prims.push_back(prim);
			}

			groups.push_back(group);
			group.Reset();
		}
		break;

		default:
			ui::LogWarning("{:08x} at {}", chunk, bx::skip(reader, 0));
			break;
		}
	}
}

void Mesh::Render(uint8_t viewId)
{
	if (bgfx::isValid(program))
		Submit(viewId, program, transform, BGFX_STATE_MASK);
	else if (geometry && material)
	{
		bgfx::setTransform(glm::value_ptr(worldMatrix));
		bgfx::setVertexBuffer(0, geometry->vbh);
		bgfx::setIndexBuffer(geometry->ibh);
		material->Apply();
		bgfx::submit(viewId, material->program);
	}
}

void Mesh::Submit(uint16_t id, bgfx::ProgramHandle program, const float* mtx, uint64_t state) const
{
	if (state == BGFX_STATE_MASK)
		state = 0
		    | BGFX_STATE_WRITE_RGB
		    | BGFX_STATE_WRITE_A
		    | BGFX_STATE_WRITE_Z
		    | BGFX_STATE_DEPTH_TEST_LESS
		    | BGFX_STATE_CULL_CCW
		    | BGFX_STATE_MSAA;

	bgfx::setTransform(mtx);
	bgfx::setState(state);

	for (const auto& group : groups)
	{
		bgfx::setIndexBuffer(group.m_ibh);
		bgfx::setVertexBuffer(0, group.m_vbh);
		bgfx::submit(id, program, 0, BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS);
	}

	bgfx::discard();
}

void Mesh::Submit(const MeshState* const* _state, uint8_t numPasses, const float* mtx, uint16_t numMatrices) const
{
	const uint32_t cached = bgfx::setTransform(mtx, numMatrices);

	for (uint32_t pass = 0; pass < numPasses; ++pass)
	{
		bgfx::setTransform(cached, numMatrices);

		const MeshState& state = *_state[pass];
		bgfx::setState(state.m_state);

		for (uint8_t tex = 0; tex < state.m_numTextures; ++tex)
		{
			const MeshState::Texture& texture = state.m_textures[tex];
			bgfx::setTexture(texture.m_stage, texture.m_sampler, texture.m_texture, texture.m_flags);
		}

		for (const auto& group : groups)
		{
			bgfx::setIndexBuffer(group.m_ibh);
			bgfx::setVertexBuffer(0, group.m_vbh);
			bgfx::submit(state.m_viewId, state.m_program, 0, BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS);
		}

		bgfx::discard(0 | BGFX_DISCARD_BINDINGS | BGFX_DISCARD_STATE | BGFX_DISCARD_TRANSFORM);
	}

	bgfx::discard();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

static sMesh MeshLoad(bx::ReaderSeekerI* reader, bool ramcopy)
{
	auto mesh = std::make_shared<Mesh>();
	mesh->Load(reader, ramcopy);
	return mesh;
}

sMesh MeshLoad(const bx::FilePath& filePath, bool ramcopy)
{
	bx::FileReaderI* reader = entry::getFileReader();
	if (bx::open(reader, filePath))
	{
		auto mesh = MeshLoad(reader, ramcopy);
		bx::close(reader);
		return mesh;
	}
	return nullptr;
}

MeshState* MeshStateCreate()
{
	MeshState* state = (MeshState*)bx::alloc(entry::getAllocator(), sizeof(MeshState));
	return state;
}

void MeshStateDestroy(MeshState* meshState)
{
	bx::free(entry::getAllocator(), meshState);
}
