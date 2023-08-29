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

static const uint8_t djvupure_page_sign[4] = { 'D', 'J', 'V', 'U' };

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupurePageCreate(void)
{
	return djvupureContainerCreate(djvupure_page_sign);
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupurePageCheckSubsign(const uint8_t sign[4])
{
	if(!memcmp(sign, djvupure_page_sign, 4))
		return true;
	else
		return false;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupurePageIs(djvupure_chunk_t *page)
{
	if(djvupureChunkGetStructHash() != page->hash) return false;
	if(!djvupureContainerIs(page, djvupure_page_sign)) return false;

	return true;
}

typedef struct {
	djvupure_chunk_t *page;
	djvupure_page_info_t info;
} djvupure_image_renderer_ctx_t;

DJVUPURE_API void * DJVUPURE_APIENTRY_EXPORT djvupurePageImageRendererCreate(djvupure_chunk_t *page, djvupure_chunk_t *document, uint16_t *width, uint16_t *height, uint8_t *channels)
{
	djvupure_image_renderer_ctx_t *ctx;
	djvupure_chunk_t *info_chunk;

	if(!djvupurePageIs(page)) return 0;

	if(document)
		if(!djvupureDocumentIs(document)) return 0;

	if(!page) return 0;

	if(!width || !height || !channels) return 0;

	ctx = (djvupure_image_renderer_ctx_t *)malloc(sizeof(djvupure_image_renderer_ctx_t));
	if(!ctx) return 0;

	ctx->page = page;

	info_chunk = djvupureContainerGetSubchunk(page, 0);
	if(!djvupureInfoIs(info_chunk)) {
		free(ctx);

		return 0;
	}

	djvupureInfoGet(info_chunk, &(ctx->info));

	*width = ctx->info.width;
	*height = ctx->info.height;
	*channels = 3;

	return ctx;
}

DJVUPURE_API int DJVUPURE_APIENTRY_EXPORT djvupurePageImageRendererNext(void *image_renderer_ctx, void *image_buffer)
{
	djvupure_image_renderer_ctx_t *ctx;
	uint8_t bgjp_sign[4] = { 'B', 'G', 'j', 'p' };
	djvupure_chunk_t *bgjp_chunk;

	ctx = (djvupure_image_renderer_ctx_t *)image_renderer_ctx;

	bgjp_chunk = djvupureContainerGetSubchunkBySign(ctx->page, bgjp_sign, 0, 0);
	if(!bgjp_chunk) return DJVUPURE_IMAGE_RENDERER_ERROR;

	if(!djvupureBGjpDecode(bgjp_chunk, ctx->info.width, ctx->info.height, image_buffer)) return DJVUPURE_IMAGE_RENDERER_ERROR;

	return DJVUPURE_IMAGE_RENDERER_LAST_STAGE;
}

DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupurePageImageRendererDestroy(void *image_renderer_ctx)
{
	if(!image_renderer_ctx) return;

	free(image_renderer_ctx);
}
