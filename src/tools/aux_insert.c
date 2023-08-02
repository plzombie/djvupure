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

#include "aux_insert.h"
#include "aux_create.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

bool InsertChunkToPage(djvupure_chunk_t *page, uint8_t sign[4], wchar_t *param)
{
	djvupure_chunk_t *chunk = 0;
	
	if(!memcmp(sign, "INFO", 4)) { // Process INFO chunk
		chunk = CreateInfoChunkFromParams(param);
	} else if(!memcmp(sign, "FG44", 4)) { // Process FG44 chunk
		CreateIW44ChunkFromFile(page, "FG44", param, 1);

		return true;
	} else if(!memcmp(sign, "BG44", 4)) { // Process BG44 chunk
		size_t n = 0;
		wchar_t *p;
			
		p = wcsrchr(param, ',');
		if(p) {
			*p = 0;
			p++;
			n = _wtoi(p);
		}

		CreateIW44ChunkFromFile(page, "BG44", param, n);

		return true;
	} else { // Process others
		chunk = CreateRawChunkFromFile(sign, param);
	}
	
	if(chunk) {
		size_t index;

		index = djvupureContainerSize(page);

		if(!djvupureContainerInsertChunk(page, chunk, index)) {
			djvupureChunkFree(chunk);
			
			return false;
		} else
			return true;
	} else
		return false;
}
