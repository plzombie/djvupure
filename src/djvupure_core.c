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

DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupureGetVersion(uint32_t *major, uint32_t *minor, uint32_t *revision)
{
	*major = DJVUPURE_VERSION_MAJOR;
	*minor = DJVUPURE_VERSION_MINOR;
	*revision = DJVUPURE_VERSION_REVISION;
}

DJVUPURE_API uint32_t DJVUPURE_APIENTRY_EXPORT djvupureChunkGetStructHash(void)
{
	return (uint32_t)(sizeof(djvupure_chunk_t)%(UINT16_MAX+1))*(UINT16_MAX+1)+DJVUPURE_VERSION_MAJOR%(UINT16_MAX+1);
}

DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupureChunkFree(djvupure_chunk_t *chunk)
{
	if(chunk->hash != djvupureChunkGetStructHash()) return;
	
	if(chunk->callback_free_aux) chunk->callback_free_aux(chunk->aux);

	chunk->callback_free(chunk->ctx);
	free(chunk);
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureChunkRender(djvupure_chunk_t *chunk, djvupure_io_callback_t *io, void *fctx)
{
	int64_t chunk_start, chunk_end, chunk_len;
	uint8_t chunk_len_be4[4] = { 0, 0, 0, 0 };
	bool result;
	
	if(chunk->hash != djvupureChunkGetStructHash()) return false;
	
	chunk_start = io->callback_tell(fctx);

	if(chunk_start % 2) {
		uint8_t dummy[1] = { 0 };

		if(chunk_start >= INT64_MAX) return false;

		chunk_start++;

		if(io->callback_write(fctx, dummy, 1) != 1) return false;
	}
	
	if(io->callback_write(fctx, chunk->sign, 4) != 4) return false;
	if(io->callback_write(fctx, chunk_len_be4, 4) != 4) return false;
	
	result = chunk->callback_render(chunk->ctx, io, fctx);
	
	chunk_end = io->callback_tell(fctx);
	
	chunk_len = chunk_end-chunk_start-8;
	
	if(chunk_len > UINT32_MAX) return 0;
	
	chunk_len_be4[0] = (chunk_len >> 24)%256;
	chunk_len_be4[1] = (chunk_len >> 16)%256;
	chunk_len_be4[2] = (chunk_len >> 8)%256;
	chunk_len_be4[3] = chunk_len%256;
	
	if(io->callback_seek(fctx, chunk_start+4, DJVUPURE_IO_SEEK_SET)) return false;
	if(io->callback_write(fctx, chunk_len_be4, 4) != 4) return false;
	if(io->callback_seek(fctx, chunk_end, DJVUPURE_IO_SEEK_SET)) return false;
	
	return result;
}

DJVUPURE_API size_t DJVUPURE_APIENTRY_EXPORT djvupureChunkSize(djvupure_chunk_t *chunk)
{
	if(chunk->hash != djvupureChunkGetStructHash()) return 0;

	return 8+chunk->callback_size(chunk->ctx);
}
