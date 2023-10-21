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

#define MMR_FLAGS_S 0x2
#define MMR_FLAGS_I 0x1

#define MMR_FLAGS_DATA_IN_STRIPES MMR_FLAGS_S
#define MMR_FLAGS_MIN_IS_BLACK MMR_FLAGS_I

static const uint8_t djvupure_smmr_sign[4] = { 'S', 'm', 'm', 'r' };

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureSmmrCheckSign(const uint8_t sign[4])
{
	if(!memcmp(sign, djvupure_smmr_sign, 4))
		return true;
	else
		return false;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureSmmrIs(djvupure_chunk_t *smmr)
{
	if(djvupureChunkGetStructHash() != smmr->hash) return false;
	if(!djvupureSmmrCheckSign(smmr->sign)) return false;

	return true;
}

static bool MMRParseHeader(uint8_t *buf, size_t size, uint16_t *width, uint16_t *height, uint8_t *flags)
{
	uint8_t temp_flags;

	if(size < 8) return false;
	if(buf[0] != 'M' || buf[1] != 'M' || buf[2] != 'R') return false;
	
	temp_flags = buf[3];
	if(temp_flags&0xFC) return false;
	if(flags) *flags = temp_flags;
	
	*width = buf[4]*256+buf[5];
	*height = buf[6]*256+buf[7];

	return true;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureSmmrGetInfo(djvupure_chunk_t *smmr, uint16_t *width, uint16_t *height)
{
	void *chunk_data = 0;
	size_t chunk_data_len = 0;

	if(!djvupureSmmrIs(smmr)) return false;
	
	djvupureRawChunkGetDataPointer(smmr, &chunk_data, &chunk_data_len);
	if(!chunk_data || !chunk_data_len) return false;
	
	return MMRParseHeader(chunk_data, chunk_data_len, width, height, 0);
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureSmmrDecode(djvupure_chunk_t *smmr, uint16_t width, uint16_t height, void *buf)
{
	void *chunk_data = 0;
	size_t chunk_data_len = 0;
	uint16_t mmr_width, mmr_height;
	uint8_t mmr_flags;
	
	if(!djvupureSmmrIs(smmr)) return false;
	
	djvupureRawChunkGetDataPointer(smmr, &chunk_data, &chunk_data_len);
	if(!chunk_data || !chunk_data_len) return false;

	if(!MMRParseHeader(chunk_data, chunk_data_len, &mmr_width, &mmr_height, &mmr_flags)) return false;
	
	if(mmr_width != width || mmr_height != height) return false;
	
	memset(buf, 255, width*height);
	
	return true;
}
