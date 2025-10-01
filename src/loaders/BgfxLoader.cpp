// BgfxLoader.cpp
// @author octopoulos
// @version 2025-09-27

#include "stdafx.h"
#include "loaders/MeshLoader.h"

#include <meshoptimizer.h>

namespace bgfx
{
	int32_t read(bx::ReaderI* reader, bgfx::VertexLayout& layout, bx::Error* err);
}

sMesh LoadBgfx(const std::filesystem::path& path, bool ramcopy, std::string_view texPath)
{
	const bx::FilePath filePath = Cstr(path);

	bx::FileReaderI* reader = entry::getFileReader();
	if (!bx::open(reader, filePath)) return nullptr;

	sMesh mesh   = std::make_shared<Mesh>("bgfx");
	auto& layout = mesh->layout;

	constexpr uint32_t kChunkVertexBuffer           = BX_MAKEFOURCC('V', 'B', ' ', 0x1);
	constexpr uint32_t kChunkVertexBufferCompressed = BX_MAKEFOURCC('V', 'B', 'C', 0x0);
	constexpr uint32_t kChunkIndexBuffer            = BX_MAKEFOURCC('I', 'B', ' ', 0x0);
	constexpr uint32_t kChunkIndexBufferCompressed  = BX_MAKEFOURCC('I', 'B', 'C', 0x1);
	constexpr uint32_t kChunkPrimitive              = BX_MAKEFOURCC('P', 'R', 'I', 0x0);

	bx::AllocatorI* allocator = entry::getAllocator();

	uint32_t  chunk;
	bx::Error err;
	Group     group;
	while (bx::read(reader, chunk, &err) == 4 && err.isOk())
	{
		switch (chunk)
		{
		case kChunkVertexBuffer:
		{
			// clang-format off
			read(reader, group.sphere, &err);
			read(reader, group.aabb  , &err);
			read(reader, group.obb   , &err);
			// clang-format on

			read(reader, layout, &err);

			const uint16_t stride = layout.getStride();

			uint16_t numVertices;
			read(reader, numVertices, &err);
			group.numVertices = numVertices;

			const bgfx::Memory* mem = bgfx::alloc(group.numVertices * stride);
			read(reader, mem->data, mem->size, &err);

			if (ramcopy)
			{
				group.vertices = (uint8_t*)bx::alloc(allocator, group.numVertices * stride);
				bx::memCopy(group.vertices, mem->data, mem->size);
			}

			group.vbh = bgfx::createVertexBuffer(mem, layout);
		}
		break;

		case kChunkVertexBufferCompressed:
		{
			// clang-format off
			read(reader, group.sphere, &err);
			read(reader, group.aabb  , &err);
			read(reader, group.obb   , &err);
			// clang-format on

			read(reader, layout, &err);

			const uint16_t stride = layout.getStride();

			read(reader, group.numVertices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.numVertices * stride);

			uint32_t compressedSize;
			bx::read(reader, compressedSize, &err);

			void* compressedVertices = bx::alloc(allocator, compressedSize);
			bx::read(reader, compressedVertices, compressedSize, &err);

			meshopt_decodeVertexBuffer(mem->data, group.numVertices, stride, (uint8_t*)compressedVertices, compressedSize);

			bx::free(allocator, compressedVertices);

			if (ramcopy)
			{
				group.vertices = (uint8_t*)bx::alloc(allocator, group.numVertices * stride);
				bx::memCopy(group.vertices, mem->data, mem->size);
			}
			group.vbh = bgfx::createVertexBuffer(mem, layout);
		}
		break;

		case kChunkIndexBuffer:
		{
			read(reader, group.numIndices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.numIndices * 2);
			read(reader, mem->data, mem->size, &err);

			if (ramcopy)
			{
				const uint16_t* src = reinterpret_cast<const uint16_t*>(mem->data);
				uint32_t*       dst = (uint32_t*)bx::alloc(allocator, group.numIndices * sizeof(uint32_t));
				for (uint32_t i = 0; i < group.numIndices; ++i) dst[i] = TO_UINT32(src[i]);
				// bx::memCopy(group.indices, mem->data, mem->size);
				group.indices = dst;
			}
			group.ibh = bgfx::createIndexBuffer(mem);
		}
		break;

		case kChunkIndexBufferCompressed:
		{
			bx::read(reader, group.numIndices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.numIndices * sizeof(uint16_t));

			uint32_t compressedSize;
			bx::read(reader, compressedSize, &err);

			void* compressedIndices = bx::alloc(allocator, compressedSize);

			bx::read(reader, compressedIndices, compressedSize, &err);

			meshopt_decodeIndexBuffer(mem->data, group.numIndices, 2, (uint8_t*)compressedIndices, compressedSize);

			bx::free(allocator, compressedIndices);

			if (ramcopy)
			{
				const uint16_t* src = reinterpret_cast<const uint16_t*>(mem->data);
				uint32_t*       dst = (uint32_t*)bx::alloc(allocator, group.numIndices * sizeof(uint32_t));
				for (uint32_t i = 0; i < group.numIndices; ++i) dst[i] = TO_UINT32(src[i]);
				// bx::memCopy(group.indices, mem->data, mem->size);
				group.indices = dst;
			}
			group.ibh = bgfx::createIndexBuffer(mem);
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
				// clang-format off
				read(reader, prim.startIndex , &err);
				read(reader, prim.numIndices , &err);
				read(reader, prim.startVertex, &err);
				read(reader, prim.numVertices, &err);
				read(reader, prim.sphere     , &err);
				read(reader, prim.aabb       , &err);
				read(reader, prim.obb        , &err);
				// clang-format on

				group.prims.push_back(prim);
			}

			mesh->groups.push_back(group);
			group.Reset();
		}
		break;

		default:
			ui::LogWarning("{:08x} at {}", chunk, bx::skip(reader, 0));
			break;
		}
	}

	bx::close(reader);
	return mesh;
}
