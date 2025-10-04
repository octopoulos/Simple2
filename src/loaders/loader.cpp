// loader.cpp
// @author octopoulos
// @version 2025-09-29

#include "stdafx.h"
#include "loaders/loader.h"

void* BgfxLoad(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const bx::FilePath& _filePath, uint32_t* _size)
{
	if (bx::open(_reader, _filePath))
	{
		uint32_t size = (uint32_t)bx::getSize(_reader);
		void*    data = bx::alloc(_allocator, size);
		bx::read(_reader, data, size, bx::ErrorAssert {});
		bx::close(_reader);
		if (_size) *_size = size;
		return data;
	}
	else ui::LogWarning("Failed to open: %s", _filePath.getCPtr());

	if (_size) *_size = 0;
	return nullptr;
}

void* BgfxLoad(const bx::FilePath& _filePath, uint32_t* _size)
{
	return BgfxLoad(entry::getFileReader(), entry::getAllocator(), _filePath, _size);
}

const bgfx::Memory* BgfxLoadMemory(bx::FileReaderI* _reader, const bx::FilePath& _filePath)
{
	if (bx::open(_reader, _filePath))
	{
		uint32_t            size = (uint32_t)bx::getSize(_reader);
		const bgfx::Memory* mem  = bgfx::alloc(size + 1);
		bx::read(_reader, mem->data, size, bx::ErrorAssert {});
		bx::close(_reader);
		mem->data[mem->size - 1] = '\0';
		return mem;
	}

	ui::LogWarning("Failed to load %s", _filePath.getCPtr());
	return nullptr;
}

void* BgfxLoadMemory(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const bx::FilePath& _filePath, uint32_t* _size)
{
	if (bx::open(_reader, _filePath))
	{
		uint32_t size = (uint32_t)bx::getSize(_reader);
		void*    data = bx::alloc(_allocator, size);
		bx::read(_reader, data, size, bx::ErrorAssert {});
		bx::close(_reader);

		if (size) *_size = size;
		return data;
	}

	ui::LogWarning("Failed to load %s", _filePath.getCPtr());
	return nullptr;
}

void BgfxUnload(void* _ptr)
{
	bx::free(entry::getAllocator(), _ptr);
}
