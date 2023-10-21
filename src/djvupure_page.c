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
#include "djvupure_sign.h"

#include <string.h>
#include <stdlib.h>

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
	uint8_t *mask;
	djvupure_page_info_t info;
	uint16_t final_width;
	uint16_t final_height;
	size_t count_bg44;
	size_t count_bgjp;
	size_t count_sjbz;
	size_t count_smmr;
	size_t count_fg44;
	size_t count_fgjp;
	int render_status;
	bool is_bg_read;
} djvupure_image_renderer_ctx_t;

enum {
	DJVUPURE_RENDER_STATUS_ERROR,
	DJVUPURE_RENDER_STATUS_BG44,
	DJVUPURE_RENDER_STATUS_BGjp,
	DJVUPURE_RENDER_STATUS_Sjbz,
	DJVUPURE_RENDER_STATUS_Smmr,
	DJVUPURE_RENDER_STATUS_FG44,
	DJVUPURE_RENDER_STATUS_FGjp,
	DJVUPURE_RENDER_STATUS_LAST
};

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

	switch(ctx->info.rotation) {
		case 5: // 90deg
		case 6: // 270deg
			ctx->final_width = ctx->info.height;
			ctx->final_height = ctx->info.width;
			break;
		case 1: // 0deg
		case 2: // 180deg
		default:
			ctx->final_width = ctx->info.width;
			ctx->final_height = ctx->info.height;
	}

	ctx->mask = 0;
	ctx->is_bg_read = false;

	ctx->count_bg44 = djvupureContainerCountSubchunksBySign(page, djvupure_bg44_sign, 0);
	ctx->count_bgjp = djvupureContainerCountSubchunksBySign(page, djvupure_bgjp_sign, 0);
	ctx->count_sjbz = djvupureContainerCountSubchunksBySign(page, djvupure_sjbz_sign, 0);
	ctx->count_smmr = djvupureContainerCountSubchunksBySign(page, djvupure_smmr_sign, 0);
	ctx->count_fg44 = djvupureContainerCountSubchunksBySign(page, djvupure_fg44_sign, 0);
	ctx->count_fgjp = djvupureContainerCountSubchunksBySign(page, djvupure_fgjp_sign, 0);

	if(ctx->count_bg44) ctx->render_status = DJVUPURE_RENDER_STATUS_BG44;
	else if(ctx->count_bgjp) ctx->render_status = DJVUPURE_RENDER_STATUS_BGjp;
	else if(ctx->count_sjbz) ctx->render_status = DJVUPURE_RENDER_STATUS_Sjbz;
	else if(ctx->count_smmr) ctx->render_status = DJVUPURE_RENDER_STATUS_Smmr;
	else {
		free(ctx);

		return 0;
	}

	*width = ctx->final_width;
	*height = ctx->final_height;
	switch(ctx->render_status) {
		case DJVUPURE_RENDER_STATUS_BG44:
		case DJVUPURE_RENDER_STATUS_BGjp:
			*channels = 3;
			break;
		default:
			*channels = 1;
	}

	return ctx;
}

static void djvupurePageImageRenderBackground(djvupure_image_renderer_ctx_t* ctx, void* image_buffer)
{
	if(ctx->render_status == DJVUPURE_RENDER_STATUS_BG44) {
		// We don't have support for now
		if(ctx->count_bgjp) ctx->render_status = DJVUPURE_RENDER_STATUS_BGjp;
	}

	if(ctx->render_status == DJVUPURE_RENDER_STATUS_BGjp) {
		djvupure_chunk_t *bgjp_chunk;

		bgjp_chunk = djvupureContainerGetSubchunkBySign(ctx->page, djvupure_bgjp_sign, 0, 0);
		if(!bgjp_chunk) {
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}

		if(!djvupureBGjpDecode(bgjp_chunk, ctx->info.width, ctx->info.height, image_buffer)) {
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}

		if(!djvupureImageRotate(ctx->info.width, ctx->info.height, ctx->final_width, ctx->final_height, 3, ctx->info.rotation, (uint8_t *)image_buffer)) {
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}

		if((ctx->count_smmr || ctx->count_sjbz) && (ctx->count_fg44 || ctx->count_fgjp)) {
			if(ctx->count_sjbz) ctx->render_status = DJVUPURE_RENDER_STATUS_Sjbz;
			else ctx->render_status = DJVUPURE_RENDER_STATUS_Smmr;

			ctx->is_bg_read = true;

			return;
		}

		ctx->render_status = DJVUPURE_RENDER_STATUS_LAST;

		return;
	}
}


static void djvupurePageImageRenderMask(djvupure_image_renderer_ctx_t *ctx, void *image_buffer)
{
	if(ctx->render_status == DJVUPURE_RENDER_STATUS_Sjbz) {
		// We don't have support for now
		if(ctx->count_smmr) ctx->render_status = DJVUPURE_RENDER_STATUS_Smmr;
	}

	if(ctx->render_status == DJVUPURE_RENDER_STATUS_Smmr) {
		void *smmr_buffer;
		djvupure_chunk_t *smmr_chunk;

		if(ctx->is_bg_read) {
			if(SIZE_MAX/ctx->final_width < ctx->final_height) {
				ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

				return;
			}
			ctx->mask = malloc((size_t)ctx->final_width*(size_t)ctx->final_height);
			if(!ctx->mask) {
				ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

				return;
			}
			smmr_buffer = ctx->mask;
		} else smmr_buffer = image_buffer;

		smmr_chunk = djvupureContainerGetSubchunkBySign(ctx->page, djvupure_smmr_sign, 0, 0);
		if(!smmr_chunk) {
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}

		if(!djvupureSmmrDecode(smmr_chunk, ctx->info.width, ctx->info.height, smmr_buffer)) {
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}

		if(!djvupureImageRotate(ctx->info.width, ctx->info.height, ctx->final_width, ctx->final_height, 1, ctx->info.rotation, (uint8_t *)smmr_buffer)) {
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}

		if(ctx->count_fg44 || ctx->count_fgjp) {
			if(ctx->count_fg44) ctx->render_status = DJVUPURE_RENDER_STATUS_FG44;
			else ctx->render_status = DJVUPURE_RENDER_STATUS_FGjp;

			return;
		} else if(ctx->is_bg_read) {
			free(ctx->mask);
			ctx->mask = 0;
			ctx->is_bg_read = 0;
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}

		ctx->render_status = DJVUPURE_RENDER_STATUS_LAST;

		return;
	}
}

static void djvupurePageImageRenderForeground(djvupure_image_renderer_ctx_t* ctx, void* image_buffer)
{
	if(ctx->render_status == DJVUPURE_RENDER_STATUS_FG44 || ctx->render_status == DJVUPURE_RENDER_STATUS_FGjp) {
		uint8_t *fg_buffer;
		bool is_fg_read = false;

		if(SIZE_MAX/ctx->final_width < 3) {
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}
		if(SIZE_MAX/ctx->final_height*3 < (size_t)ctx->final_width*3) {
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}

		fg_buffer = malloc((size_t)ctx->final_width*(size_t)ctx->final_height*3);
		if(!fg_buffer) {
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}

		if(!(ctx->mask) || !(ctx->is_bg_read)) {
			ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

			return;
		}

		if(ctx->render_status == DJVUPURE_RENDER_STATUS_FG44) {
			// We don't have support for now
			if(ctx->count_bgjp) ctx->render_status = DJVUPURE_RENDER_STATUS_FGjp;
		}
	
		if(ctx->render_status == DJVUPURE_RENDER_STATUS_FGjp) {
			djvupure_chunk_t *fgjp_chunk;

			fgjp_chunk = djvupureContainerGetSubchunkBySign(ctx->page, djvupure_fgjp_sign, 0, 0);
			if(!fgjp_chunk) {
				ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

				return;
			}

			if(!djvupureFGjpDecode(fgjp_chunk, ctx->info.width, ctx->info.height, fg_buffer)) {
				ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

				return;
			}

			if(!djvupureImageRotate(ctx->info.width, ctx->info.height, ctx->final_width, ctx->final_height, 3, ctx->info.rotation, fg_buffer)) {
				ctx->render_status = DJVUPURE_RENDER_STATUS_ERROR;

				return;
			}

			is_fg_read = true;
		}

		if(is_fg_read) { // All is OK. Now creating layered document
			uint8_t *p_fg, *p_bg, *p_mask;
			uint16_t x, y, width, height;

			p_fg = fg_buffer;
			p_bg = (uint8_t *)image_buffer;
			p_mask = ctx->mask;
			width = ctx->final_width;
			height = ctx->final_height;

			for(y = 0; y < height; y++)
				for(x = 0; x < width; x++) {
					if(*(p_mask++)) {
						*(p_bg++) = *(p_fg++);
						*(p_bg++) = *(p_fg++);
						*(p_bg++) = *(p_fg++);
					} else {
						p_fg += 3;
						p_bg += 3;
					}
				}

			ctx->render_status = DJVUPURE_RENDER_STATUS_LAST;

			free(fg_buffer);

			return;
		}
	}
}

DJVUPURE_API int DJVUPURE_APIENTRY_EXPORT djvupurePageImageRendererNext(void *image_renderer_ctx, void *image_buffer)
{
	djvupure_image_renderer_ctx_t *ctx;

	ctx = (djvupure_image_renderer_ctx_t *)image_renderer_ctx;

	if(ctx->render_status == DJVUPURE_RENDER_STATUS_BG44 || ctx->render_status == DJVUPURE_RENDER_STATUS_BGjp) djvupurePageImageRenderBackground(ctx, image_buffer);
	if(ctx->render_status == DJVUPURE_RENDER_STATUS_Sjbz || ctx->render_status == DJVUPURE_RENDER_STATUS_Smmr) djvupurePageImageRenderMask(ctx, image_buffer);
	if(ctx->render_status == DJVUPURE_RENDER_STATUS_FG44 || ctx->render_status == DJVUPURE_RENDER_STATUS_FGjp) djvupurePageImageRenderForeground(ctx, image_buffer);
	if(ctx->render_status == DJVUPURE_RENDER_STATUS_LAST) return DJVUPURE_IMAGE_RENDERER_LAST_STAGE;
	if(ctx->render_status == DJVUPURE_RENDER_STATUS_ERROR) return DJVUPURE_IMAGE_RENDERER_ERROR;

	return DJVUPURE_IMAGE_RENDERER_NEXT_STAGE;
}

DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupurePageImageRendererDestroy(void *image_renderer_ctx)
{
	djvupure_image_renderer_ctx_t* ctx;

	if(!image_renderer_ctx) return;

	ctx = (djvupure_image_renderer_ctx_t*)image_renderer_ctx;
	
	if(ctx->mask) free(ctx->mask);

	free(image_renderer_ctx);
}
