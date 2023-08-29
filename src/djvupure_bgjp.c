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

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#include <string.h>

static const uint8_t djvupure_bgjp_sign[4] = { 'B', 'G', 'j', 'p' };

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureBGjpCheckSign(const uint8_t sign[4])
{
	if(!memcmp(sign, djvupure_bgjp_sign, 4))
		return true;
	else
		return false;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureBGjpIs(djvupure_chunk_t *dir)
{
	if(djvupureChunkGetStructHash() != dir->hash) return false;
	if(!djvupureBGjpCheckSign(dir->sign)) return false;

	return true;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureBGjpDecode(djvupure_chunk_t *bgjp, uint16_t width, uint16_t height, void *buf)
{
	int img_x, img_y, img_comp;
	void *chunk_data = 0, *img_buf;
	size_t chunk_data_len = 0;

	if(!djvupureBGjpIs(bgjp)) return false;

	if((SIZE_MAX/3)/width < height) return false;

	djvupureRawChunkGetDataPointer(bgjp, &chunk_data, &chunk_data_len);
	if(!chunk_data || !chunk_data_len) return false;

	if(chunk_data_len >= INT_MAX) return false;

	img_buf = (void *)stbi_load_from_memory((stbi_uc *)chunk_data, (int)chunk_data_len, &img_x, &img_y, &img_comp, 3);
	if(!img_buf) return false;

	if(img_x != width || img_y != height) {
		free(img_buf);

		return false;
	}

	memcpy(buf, img_buf, (size_t)width*(size_t)height*3);

	free(img_buf);

	return true;
}
