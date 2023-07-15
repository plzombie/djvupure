/*
BSD 2-Clause License

Copyright (c) 2023, Mikhail Morozov

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DJVUPURE_H
#define DJVUPURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#ifdef DJVUPURE_EXPORT_DLL
#ifdef _WIN32
#define DJVUPURE_API __declspec(dllexport)
#else
#define DJVUPURE_API __attribute__ ((visibility("default")))
#endif
#else
#define DJVUPURE_API
#endif

#ifdef _WIN32
#define DJVUPURE_APIENTRY __cdecl
#else
#define DJVUPURE_APIENTRY
#endif

#define DJVUPURE_APIENTRY_EXPORT DJVUPURE_APIENTRY

#define DJVUPURE_VERSION_MAJOR ((uint32_t)0)
#define DJVUPURE_VERSION_MINOR ((uint32_t)0)
#define DJVUPURE_VERSION_REVISION ((uint32_t)0)

#define DJVUPURE_IO_SEEK_CUR 0
#define DJVUPURE_IO_SEEK_END 1
#define DJVUPURE_IO_SEEK_SET 2

typedef size_t (DJVUPURE_APIENTRY * djvupure_io_callback_read_t)(void *fctx, void *buf, size_t size);
typedef size_t (DJVUPURE_APIENTRY * djvupure_io_callback_write_t)(void *fctx, const void *buf, size_t size);
typedef int (DJVUPURE_APIENTRY * djvupure_io_callback_seek_t)(void *fctx, int64_t offset, int origin);
typedef int64_t (DJVUPURE_APIENTRY * djvupure_io_callback_tell_t)(void *fctx);

typedef struct {
	uint32_t hash;
	djvupure_io_callback_read_t callback_read;
	djvupure_io_callback_write_t callback_write;
	djvupure_io_callback_seek_t callback_seek;
	djvupure_io_callback_tell_t callback_tell;
} djvupure_io_callback_t;

typedef void (DJVUPURE_APIENTRY * djvupure_chunk_callback_free_t)(void *ctx);
typedef bool (DJVUPURE_APIENTRY * djvupure_chunk_callback_render_t)(void *ctx, djvupure_io_callback_t *io, void *fctx);

typedef struct {
	uint32_t hash;
	uint8_t sign[4];
	void *ctx;
	djvupure_chunk_callback_free_t callback_free;
	djvupure_chunk_callback_render_t callback_render;
} djvupure_chunk_t;

typedef struct {
	uint16_t width;
	uint16_t height;
	uint16_t dpi;
	uint8_t gamma; // Defaults to 22
	uint8_t rotation; // 1 - without, 5 - 90deg, 2 - 180deg, 6 - 270deg
} djvupure_page_info_t;

#ifdef __cplusplus
}
#endif

DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupureGetVersion(uint32_t *major, uint32_t *minor, uint32_t *revision);

DJVUPURE_API uint32_t DJVUPURE_APIENTRY_EXPORT djvupureChunkGetStructHash(void);
DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupureChunkFree(djvupure_chunk_t *chunk);
DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureChunkRender(djvupure_chunk_t *chunk, djvupure_io_callback_t *io, void *fctx);

DJVUPURE_API uint32_t DJVUPURE_APIENTRY_EXPORT djvupureIOGetStructHash(void);

DJVUPURE_API void * DJVUPURE_APIENTRY_EXPORT djvupureFileOpenA(char *filename, bool write);
DJVUPURE_API void * DJVUPURE_APIENTRY_EXPORT djvupureFileOpenW(wchar_t *filename, bool write);
DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupureFileClose(void *fctx);
DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupureFileSetIoCallbacks(djvupure_io_callback_t *io);

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureRawChunkCreate(const uint8_t sign[4], void *data, size_t data_len);
DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureRawChunkRead(djvupure_io_callback_t *io, void *fctx);
DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupureRawChunkGetDataPointer(djvupure_chunk_t *chunk, void **data, size_t *data_len);

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureContainerCheckSign(const uint8_t sign[4]);
DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureContainerIs(djvupure_chunk_t *container, const uint8_t subsign[4]);
DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureContainerCreate(const uint8_t subsign[4]);
DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureContainerRead(djvupure_io_callback_t *io, void *fctx);
DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureContainerInsertChunk(djvupure_chunk_t *container, djvupure_chunk_t *chunk, size_t index);
DJVUPURE_API size_t DJVUPURE_APIENTRY_EXPORT djvupureContainerSize(djvupure_chunk_t *container);
DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureContainerGetSubchunk(djvupure_chunk_t *container, size_t index);

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureInfoCreate(djvupure_page_info_t info);

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupurePageCreate(void);

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureDocumentRead(djvupure_io_callback_t *io, void *fctx);
DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureDocumentRender(djvupure_chunk_t *chunk, djvupure_io_callback_t *io, void *fctx);

#endif