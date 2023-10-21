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

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "third_party/stb_image_resize.h"

#include <string.h>
#include <stdlib.h>

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureImageRotate(uint16_t old_width, uint16_t old_height, uint16_t new_width, uint16_t new_height, uint8_t channels, uint8_t rot, uint8_t *buffer)
{
	uint8_t *new_buffer = 0;

	if(!old_width || !old_height || !new_width || !new_height || !channels) return false;
	if(SIZE_MAX/new_width < channels) return false;
	if(SIZE_MAX/new_height < (size_t)new_width*(size_t)channels) return false;

	switch(rot) {
		case 5: // 90deg
		case 6: // 270deg
			if(old_width != new_height || old_height != new_width) return false;
			new_buffer = malloc((size_t)new_width*(size_t)new_height*(size_t)channels);
			if(!new_buffer) return false;
			break;
		case 1: // 0deg
		case 2: // 180deg
		default:
			if(old_width != new_width || old_height != new_height) return false;
	}

	if(rot == 2) {
		uint8_t *p1, *p2;

		p1 = buffer;
		p2 = buffer+((size_t)old_width*(size_t)old_height*(size_t)channels-channels);

		for(size_t y = 0; y < (size_t)old_height/2; y++) {
			for(size_t x = 0; x < old_width; x++) {
				for(size_t c = 0; c < channels; c++) {
					uint8_t temp;

					temp = p1[c];
					p1[c] = p2[c];
					p2[c] = temp;
				}
				p1 += channels;
				p2 -= channels;
			}
		}

		if(old_height%2 > 0) { // Rotate middle line
			p1 = buffer+((size_t)old_width*((size_t)old_height/2)*(size_t)channels);
			p2 = p1+((size_t)old_width*(size_t)channels-channels);

			for(size_t x = 0; x < (size_t)old_width/2; x++) {
				for(size_t c = 0; c < channels; c++) {
					uint8_t temp;

					temp = p1[c];
					p1[c] = p2[c];
					p2[c] = temp;
				}
				p1 += channels;
				p2 -= channels;
			}
		}
	}

	if(rot == 5) {
		uint8_t *p1;

		p1 = buffer;

		for(size_t y = 0; y < (size_t)old_height; y++) {
			for(size_t x = 0; x < old_width; x++) {
				uint8_t *p2;

				p2 = new_buffer+(size_t)channels*((size_t)new_width*x+((size_t)new_width-y-1));

				for(size_t c = 0; c < channels; c++) {
					*(p2++) = *(p1++);
				}
			}
		}
	}

	if(rot == 6) {
		uint8_t *p1;

		p1 = buffer;

		for(size_t y = 0; y < (size_t)old_height; y++) {
			for(size_t x = 0; x < old_width; x++) {
				uint8_t *p2;

				p2 = new_buffer+(size_t)channels*((size_t)new_width*(new_height-x-1)+y);

				for(size_t c = 0; c < channels; c++) {
					*(p2++) = *(p1++);
				}
			}
		}
	}

	if(new_buffer) {
		memcpy(buffer, new_buffer, (size_t)new_width*(size_t)new_height*(size_t)channels);
		free(new_buffer);
	}

	return true;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureImageResizeFine(uint16_t old_width, uint16_t old_height, const uint8_t *old_buffer, uint16_t new_width, uint16_t new_height, uint8_t *new_buffer, uint8_t channels)
{
	int ret;

	ret = stbir_resize_uint8(
		old_buffer, old_width, old_height, 0,
		new_buffer, new_width, new_height, 0,
		channels);

	return ret?true:false;
}

