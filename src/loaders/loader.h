// loader.h
// @author octopoulos
// @version 2025-09-27

#pragma once

void*               BgfxLoad(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const bx::FilePath& _filePath, uint32_t* _size);
void*               BgfxLoad(const bx::FilePath& _filePath, uint32_t* _size = nullptr);
const bgfx::Memory* BgfxLoadMemory(bx::FileReaderI* _reader, const bx::FilePath& _filePath);
void*               BgfxLoadMemory(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const bx::FilePath& _filePath, uint32_t* _size);
void                BgfxUnload(void* _ptr);
