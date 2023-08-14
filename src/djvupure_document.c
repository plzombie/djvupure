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

static const uint8_t djvupure_document_sign[4] = { 'D', 'J', 'V', 'M' };
static const uint8_t djvupure_page_sign[4] = { 'D', 'J', 'V', 'U' };

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureDocumentIs(djvupure_chunk_t *document)
{
	if(djvupureChunkGetStructHash() != document->hash) return false;
	if(djvupureContainerIs(document, djvupure_document_sign)) return true;
	if(djvupureContainerIs(document, djvupure_page_sign)) return true;
	return false;
}

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureDocumentRead(djvupure_io_callback_t *io, void *fctx)
{
	uint8_t atnt_sign[4] = { 'A', 'T', '&', 'T' }, sign[4];
	djvupure_chunk_t *document;
	
	if(io->callback_read(fctx, sign, 4) != 4) return 0;
	if(memcmp(sign, atnt_sign, 4)) return 0;
	
	document = djvupureContainerRead(io, fctx);
	if(!document) return 0;

	if(djvupureContainerIs(document, djvupure_document_sign)) {
		djvupure_chunk_t *dir;

		dir = djvupureContainerGetSubchunk(document, 0);
		if(!dir) {
			djvupureChunkFree(document);

			return 0;
		}

		if(!djvupureDirIs(dir)) {
			djvupureChunkFree(document);

			return 0;
		}

		if(!djvupureDirInit(dir, document)) {
			djvupureChunkFree(document);

			return 0;
		}
	}

	return document;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureDocumentRender(djvupure_chunk_t *chunk, djvupure_io_callback_t *io, void *fctx)
{
	uint8_t atnt_sign[4] = { 'A', 'T', '&', 'T' };
	
	if(io->callback_write(fctx, atnt_sign, 4) != 4) return false;

	return djvupureChunkRender(chunk, io, fctx);
}

DJVUPURE_API size_t DJVUPURE_APIENTRY_EXPORT djvupureDocumentCountPages(djvupure_chunk_t *document)
{
	djvupure_chunk_t *dir;

	if(!djvupureDocumentIs(document)) return 0;
	if(djvupureContainerIs(document, djvupure_page_sign)) return 1;

	dir = djvupureContainerGetSubchunk(document, 0);
	if(!dir) return 0;

	if(!djvupureDirIs(dir)) return 0;

	return djvupureDirCountPages(dir);
}

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureDocumentGetPage(djvupure_chunk_t *document, size_t index, djvupure_io_callback_openu8_t openu8, djvupure_io_callback_close_t close)
{
	djvupure_chunk_t *dir;

	if(!djvupureDocumentIs(document)) return 0;
	if(djvupureContainerIs(document, djvupure_page_sign)) return document;

	dir = djvupureContainerGetSubchunk(document, 0);
	if(!dir) return 0;

	if(!djvupureDirIs(dir)) return 0;

	return djvupureDirGetPage(dir, index, openu8, close);
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureDocumentPutPage(djvupure_chunk_t *document, djvupure_chunk_t *page, bool changed, djvupure_io_callback_openu8_t openu8, djvupure_io_callback_close_t close)
{
	djvupure_chunk_t *dir;

	if(!djvupureDocumentIs(document)) return false;

	if(djvupureContainerIs(document, djvupure_page_sign)) {
		if(document == page) return true;

		return false;
	}

	dir = djvupureContainerGetSubchunk(document, 0);
	if(!dir) return false;

	if(!djvupureDirIs(dir)) return false;

	return djvupureDirPutPage(dir, page, changed, openu8, close);
}
