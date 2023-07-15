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

static const uint8_t djvupure_info_sign[4] = { 'I', 'N', 'F', 'O' };

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureInfoCreate(djvupure_page_info_t info)
{
	uint8_t chunk_data[10];
	
	chunk_data[0] = info.width/256;
	chunk_data[1] = info.width%256;
	chunk_data[2] = info.height/256;
	chunk_data[3] = info.height%256;
	chunk_data[4] = 26; // Minor version
	chunk_data[5] = 0; // Major version
	chunk_data[6] = info.dpi%256;
	chunk_data[7] = info.dpi/256;
	chunk_data[8] = info.gamma;
	switch(info.rotation) {
		case 1:
		case 2:
		case 5:
		case 6:
			chunk_data[9] = info.rotation;
			break;
		default:
			chunk_data[9] = 1;
	}
	
	return djvupureRawChunkCreate(djvupure_info_sign, &chunk_data, 10);
}
