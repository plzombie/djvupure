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

#include <string.h>
#include <stdlib.h>

static const uint8_t djvupure_form_sign[4] = { 'F', 'O', 'R', 'M' };

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureContainerCheckSign(const uint8_t sign[4])
{
	if(!memcmp(sign, djvupure_form_sign, 4))
		return true;
	else
		return false;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureContainerIs(djvupure_chunk_t *container, const uint8_t subsign[4])
{
	if(!djvupureContainerCheckSign(container->sign)) return false;

	if(subsign) {
		if(container->ctx == 0) return false;

		if(memcmp(container->ctx, subsign, 4)) return false;
	}

	return true;
}

static void DJVUPURE_APIENTRY djvupureContainerCallbackFree(void *ctx)
{
	uintptr_t uctx;
	size_t nof_subchunks;
	djvupure_chunk_t **subchunk;

	if(!ctx) return;

	uctx = (uintptr_t)ctx;
	nof_subchunks = *((size_t *)(uctx+4+4+sizeof(size_t)));
	subchunk = (djvupure_chunk_t **)(uctx+4+4+sizeof(size_t)*2);
	
	for(size_t i = 0; i < nof_subchunks; i++) {
		djvupureChunkFree(*subchunk);
		subchunk++;
	}
	
	free(ctx);
}

static bool DJVUPURE_APIENTRY djvupureContainerCallbackRender(void *ctx, djvupure_io_callback_t *io, void *fctx)
{
	uintptr_t uctx;
	size_t nof_subchunks;
	djvupure_chunk_t **subchunk;

	if(!ctx) return false;

	uctx = (uintptr_t)ctx;
	nof_subchunks = *((size_t *)(uctx+4+4+sizeof(size_t)));
	subchunk = (djvupure_chunk_t **)(uctx+4+4+sizeof(size_t)*2);
	
	if(io->callback_write(fctx, ctx, 4) != 4) return false;
	
	for(size_t i = 0; i < nof_subchunks; i++) {
		if(!djvupureChunkRender(*subchunk, io, fctx)) return false;
		
		subchunk++;
	}
	
	return true;
}

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureContainerCreate(const uint8_t subsign[4])
{
	djvupure_chunk_t *container = 0;
	size_t ctx_size;
	
	container = malloc(sizeof(djvupure_chunk_t));
	if(!container) return 0;
	
	container->callback_free = djvupureContainerCallbackFree;
	container->callback_render = djvupureContainerCallbackRender;
	container->hash = djvupureChunkGetStructHash();
	ctx_size = 4+4+sizeof(size_t)*2;
	container->ctx = malloc(ctx_size);
	if(!container->ctx) {
		free(container);
		
		return 0;
	}
	memset(container->ctx, 0, 4+4+sizeof(size_t)*2);
	
	memcpy(container->sign, djvupure_form_sign, 4);
	memcpy(container->ctx, subsign, 4);
	
	return container;
}

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureContainerRead(djvupure_io_callback_t *io, void *fctx)
{
	djvupure_chunk_t *container = 0;
	void *ctx;
	size_t ctx_size;
	int64_t chunk_start, chunk_end, chunk_len;
	uint8_t chunk_len_be4[4];
	
	container = malloc(sizeof(djvupure_chunk_t));
	if(!container) return 0;
	
	container->callback_free = djvupureContainerCallbackFree;
	container->callback_render = djvupureContainerCallbackRender;
	container->hash = djvupureChunkGetStructHash();
	ctx_size = 4+4+sizeof(size_t)*2;
	container->ctx = malloc(ctx_size);
	if(!container->ctx) goto FAILURE;
	memset(container->ctx, 0, 4+4+sizeof(size_t)*2);
	
	chunk_start = io->callback_tell(fctx);
	ctx = container->ctx;
	
	if(io->callback_read(fctx, container->sign, 4) != 4) goto FAILURE;
	if(!djvupureContainerCheckSign(container->sign)) goto FAILURE;
	if(io->callback_read(fctx, chunk_len_be4, 4) != 4) goto FAILURE;
	if(io->callback_read(fctx, ctx, 4) != 4) goto FAILURE;
	
	chunk_len = (((int64_t)(chunk_len_be4[0]))<<24)+
		(((int64_t)(chunk_len_be4[1]))<<16)+
		(((int64_t)(chunk_len_be4[2]))<<8)+
		chunk_len_be4[3];
	
	chunk_end = chunk_start;
	if(INT64_MAX-8 < chunk_end) goto FAILURE;
	chunk_end += 8;
	if(INT64_MAX-chunk_len < chunk_end) goto FAILURE;
	chunk_end += chunk_len;
	
	while(io->callback_tell(fctx) < chunk_end) {
		uint8_t sign[4];
		djvupure_chunk_t *subchunk;
		size_t index;

		if(io->callback_read(fctx, sign, 4) != 4) goto FAILURE;
		if(io->callback_seek(fctx, -4, DJVUPURE_IO_SEEK_CUR)) goto FAILURE;
		
		if(djvupureContainerCheckSign(sign))
			subchunk = djvupureContainerRead(io, fctx);
		else
			subchunk = djvupureRawChunkRead(io, fctx);
		if(!subchunk) goto FAILURE;
		
		index = djvupureContainerSize(container);
		if(!djvupureContainerInsertChunk(container, subchunk, index)) {
			djvupureChunkFree(subchunk);

			goto FAILURE;
		}
	}
	
	return container;

FAILURE:
	if(container) djvupureChunkFree(container);
	
	return 0;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureContainerInsertChunk(djvupure_chunk_t *container, djvupure_chunk_t *chunk, size_t index)
{
	uintptr_t uctx;
	size_t nof_allocsubchunks, nof_subchunks;
	djvupure_chunk_t **subchunk;

	if(!djvupureContainerCheckSign(container->sign)) return false;
	if(djvupureChunkGetStructHash() != container->hash) return false;

	uctx = (uintptr_t)(container->ctx);
	nof_allocsubchunks = *((size_t *)(uctx+4+4));
	nof_subchunks = *((size_t *)(uctx+4+4+sizeof(size_t)));
	if(index > nof_subchunks) return false;

	if(nof_subchunks >= nof_allocsubchunks) {
		void *_ctx;

		if(nof_allocsubchunks == 0) nof_allocsubchunks = 2;
		else nof_allocsubchunks *= 2;

		if((SIZE_MAX-4-4-sizeof(size_t)*2)/sizeof(void *) < nof_allocsubchunks) return false;

		_ctx = realloc(container->ctx, 4+4+sizeof(size_t)*2+nof_allocsubchunks*sizeof(void *));
		if(!_ctx) return false;

		container->ctx = _ctx;
		uctx = (uintptr_t)(container->ctx);
		*((size_t *)(uctx+4+4)) = nof_allocsubchunks;
	}

	subchunk = (djvupure_chunk_t **)(uctx+4+4+sizeof(size_t)*2);

	for(size_t i = nof_subchunks; i > index; i--) {
		subchunk[i] = subchunk[i-1];
	}

	subchunk[index] = chunk;
	nof_subchunks++;
	 *((size_t *)(uctx+4+4+sizeof(size_t))) = nof_subchunks;
	 
	 return true;
}

DJVUPURE_API size_t DJVUPURE_APIENTRY_EXPORT djvupureContainerSize(djvupure_chunk_t *container)
{
	uintptr_t uctx;
	
	if(!djvupureContainerCheckSign(container->sign)) return 0;
	if(djvupureChunkGetStructHash() != container->hash) return 0;

	uctx = (uintptr_t)(container->ctx);
	return *((size_t *)(uctx+4+4+sizeof(size_t)));
}

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureContainerGetSubchunk(djvupure_chunk_t *container, size_t index)
{
	uintptr_t uctx;
	size_t nof_subchunks;
	djvupure_chunk_t **subchunk;
	
	if(!djvupureContainerCheckSign(container->sign)) return 0;
	if(djvupureChunkGetStructHash() != container->hash) return 0;

	uctx = (uintptr_t)(container->ctx);
	nof_subchunks = *((size_t *)(uctx+4+4+sizeof(size_t)));
	subchunk = (djvupure_chunk_t **)(uctx+4+4+sizeof(size_t)*2);
	
	if(index >= nof_subchunks) return 0;
	
	return *(subchunk+index);
}

