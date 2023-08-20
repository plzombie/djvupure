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
	if(!djvupureBGjpIs(bgjp)) return false;

	// Here must be decoder
	memset(buf, 0xf0, (size_t)width*(size_t)height*3);

	return true;
}
