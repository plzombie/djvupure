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

#include "djvupure_jpeg.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#include <string.h>

bool DJVUPURE_APIENTRY JpegGetInfo(djvupure_chunk_t *jpeg, uint16_t *width, uint16_t *height)
{
	int img_x, img_y, img_comp;
	void *chunk_data = 0;
	size_t chunk_data_len = 0;

	djvupureRawChunkGetDataPointer(jpeg, &chunk_data, &chunk_data_len);
	if(!chunk_data || !chunk_data_len) return false;

	if(chunk_data_len >= INT_MAX) return false;

	if(!stbi_info_from_memory((stbi_uc*)chunk_data, (int)chunk_data_len, &img_x, &img_y, &img_comp)) return false;

	if(img_x > UINT16_MAX || img_y > UINT16_MAX) return false;

	*width = (uint16_t)img_x;
	*height = (uint16_t)img_y;

	return true;
}

bool DJVUPURE_APIENTRY JpegDecode(djvupure_chunk_t *jpeg, uint16_t width, uint16_t height, void *buf)
{
	int img_x, img_y, img_comp;
	void *chunk_data = 0, *img_buf;
	size_t chunk_data_len = 0;
	bool result = false;

	if((SIZE_MAX/3)/width < height) return false;

	djvupureRawChunkGetDataPointer(jpeg, &chunk_data, &chunk_data_len);
	if(!chunk_data || !chunk_data_len) return false;

	if(chunk_data_len >= INT_MAX) return false;

	img_buf = (void *)stbi_load_from_memory((stbi_uc *)chunk_data, (int)chunk_data_len, &img_x, &img_y, &img_comp, 3);
	if(!img_buf) return false;

	if(img_x != width || img_y != height) {
		if(img_x > width || img_y > height) goto FINAL;
		if(!djvupureImageResizeFine(img_x, img_y, img_buf, width, height, buf, 3)) goto FINAL;
	} else
		memcpy(buf, img_buf, (size_t)width* (size_t)height * 3);

	free(img_buf);

	result = true;

FINAL:

	return result;
}
