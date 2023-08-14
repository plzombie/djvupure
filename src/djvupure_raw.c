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

#include "../include/djvupure.h"

#include <stdlib.h>
#include <string.h>

static void DJVUPURE_APIENTRY djvupureRawChunkCallbackFree(void *ctx)
{
	if(!ctx) return;
	
	free(ctx);
}

static bool DJVUPURE_APIENTRY djvupureRawChunkCallbackRender(void *ctx, djvupure_io_callback_t *io, void *fctx)
{
	uintptr_t uctx;
	size_t chunk_len;
	
	if(!ctx) return false;
	
	uctx = (uintptr_t)ctx;
	chunk_len = *((size_t *)ctx);
	
	if(io->callback_write(fctx, (void *)(uctx+sizeof(size_t)), chunk_len) != chunk_len) return false;
	
	return true;
}

static size_t DJVUPURE_APIENTRY djvupureRawChunkCallbackSize(void *ctx)
{
	if(!ctx) return 0;

	return *((size_t*)ctx);
}

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureRawChunkCreate(const uint8_t sign[4], void *data, size_t data_len)
{
	djvupure_chunk_t *chunk = 0;
	uintptr_t uctx;
	
	if(data_len > SIZE_MAX-sizeof(size_t)) return 0;
	
	chunk = malloc(sizeof(djvupure_chunk_t));
	if(!chunk) return 0;
	memset(chunk, 0, sizeof(djvupure_chunk_t));
	chunk->callback_free = djvupureRawChunkCallbackFree;
	chunk->callback_render = djvupureRawChunkCallbackRender;
	chunk->callback_size = djvupureRawChunkCallbackSize;
	chunk->hash = djvupureChunkGetStructHash();
	chunk->ctx = 0;
	memcpy(chunk->sign, sign, 4);
	
	chunk->ctx = malloc(data_len+sizeof(size_t));
	if(!chunk->ctx) {
		free(chunk);
		
		return 0;
	}
	
	uctx = (uintptr_t)(chunk->ctx);
	*((size_t *)(chunk->ctx)) = data_len;
	memcpy((void *)(uctx+sizeof(size_t)), data, data_len);
	
	return chunk;
}

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureRawChunkRead(djvupure_io_callback_t *io, void *fctx)
{
	djvupure_chunk_t *chunk = 0;
	uintptr_t uctx;
	int64_t chunk_len;
	uint8_t chunk_len_be4[4];
	
	chunk = malloc(sizeof(djvupure_chunk_t));
	if(!chunk) return 0;
	memset(chunk, 0, sizeof(djvupure_chunk_t));
	chunk->callback_free = djvupureRawChunkCallbackFree;
	chunk->callback_render = djvupureRawChunkCallbackRender;
	chunk->callback_size = djvupureRawChunkCallbackSize;
	chunk->hash = djvupureChunkGetStructHash();
	chunk->ctx = 0;

	if(io->callback_tell(fctx) % 2)
		if(io->callback_seek(fctx, 1, DJVUPURE_IO_SEEK_CUR)) goto FAILURE;
	
	if(io->callback_read(fctx, chunk->sign, 4) != 4) goto FAILURE;
	if(io->callback_read(fctx, chunk_len_be4, 4) != 4) goto FAILURE;
	
	chunk_len = (((int64_t)(chunk_len_be4[0]))<<24)+
		(((int64_t)(chunk_len_be4[1]))<<16)+
		(((int64_t)(chunk_len_be4[2]))<<8)+
		chunk_len_be4[3];
	
	if(chunk_len > SIZE_MAX-sizeof(size_t)) goto FAILURE;
	
	chunk->ctx = malloc((size_t)(chunk_len+sizeof(size_t)));
	if(!chunk->ctx) goto FAILURE;
	
	uctx = (uintptr_t)(chunk->ctx);
	*((size_t *)(chunk->ctx)) = (size_t)chunk_len;
	
	if(io->callback_read(fctx, (void *)(uctx+sizeof(size_t)), (size_t)chunk_len) != chunk_len) goto FAILURE;

	return chunk;
	
FAILURE:
	if(chunk) djvupureChunkFree(chunk);
	
	return 0;
}

DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupureRawChunkGetDataPointer(djvupure_chunk_t *chunk, void **data, size_t *data_len)
{
	*data = 0;
	*data_len = 0;

	if(chunk->hash != djvupureChunkGetStructHash()) return;

	*data_len = *((size_t *)(chunk->ctx));
	*data = (void *)((uintptr_t)(chunk->ctx)+sizeof(size_t));
}