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

#include "../../include/djvupure.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

bool ExtractChunkFromPage(djvupure_chunk_t *page, const uint8_t sign[4], wchar_t *param);

int wmain(int argc, wchar_t **argv)
{
	djvupure_io_callback_t io;
	djvupure_chunk_t *document = 0, *page;
	void *fctx = 0;
	int result = EXIT_FAILURE;

	setlocale(LC_CTYPE, "");

	if(argc <= 1) {
		wchar_t *command, *_command;
		
		command = argv[0];
		_command = wcsrchr(command, '\\');
		if(_command) command = _command+1;
		_command = wcsrchr(command, '/');
		if(_command) command = _command+1;

		wprintf(L"%ls document.djvu CHUNK1=param1 CHUNK2=param2 ...\n"
			L"\tParameter is a path to a file containing chunk data (i.e. \"Sjbz=page.sjbz\")\n"
			L"\tFor chunks FG44 and BG44 an IFF85 file with a group of PM44 subchunks will be produced\n",
			command);

		return EXIT_SUCCESS;
	}
	
	djvupureFileSetIoCallbacks(&io);
	
	fctx = djvupureFileOpenW(argv[1], false);
	if(!fctx) goto FINAL;
	
	document = djvupureDocumentRead(&io, fctx);
	djvupureFileClose(fctx);
	fctx = 0;
	if(!document) goto FINAL;
	
	if(!djvupurePageIs(document)) goto FINAL;
	page = document;
	
	for(int i = 2; i < argc; i++) {
		uint8_t sign[4];
		wchar_t *chunk_filename;
		size_t j;

		// Copy Signature
		for(j = 0; j < 4; j++) {
			if(argv[i][j] == 0) break;

			sign[j] = (uint8_t)(argv[i][j]);
		}

		if(argv[i][j] != '=') {
			wprintf(L"Unknown argument: \"%ls\"\n", argv[i]);

			continue;
		}

		chunk_filename = argv[i]+j+1;
		
		if(!ExtractChunkFromPage(page, sign, chunk_filename))
			wprintf(L"Can't extract chunk %.4hs\n", sign);
	}
	
	result = EXIT_SUCCESS;
	
FINAL:
	if(fctx) djvupureFileClose(fctx);
	if(document) djvupureChunkFree(document);
	
	return result;
}

bool ExtractIW44ChunkFromPage(djvupure_chunk_t *page, const uint8_t sign[4], wchar_t *param)
{
	const uint8_t pm44_sign[4] = { 'P', 'M', '4', '4' };
	djvupure_io_callback_t io;
	void *fctx = 0;
	djvupure_chunk_t *chunk;
	size_t nof_subchunks;

	nof_subchunks = djvupureContainerCountSubchunksBySign(page, sign, 0);
	if(!nof_subchunks) return false;

	chunk = djvupureContainerCreate(pm44_sign);

	for(size_t index = 0; index < nof_subchunks; index++) {
		djvupure_chunk_t *old_subchunk, *new_subchunk;
		void *data;
		size_t data_len;

		old_subchunk = djvupureContainerGetSubchunkBySign(page, sign, 0, index);
		if(!old_subchunk) {
			djvupureChunkFree(chunk);

			return false;
		}

		djvupureRawChunkGetDataPointer(old_subchunk, &data, &data_len);
		if(!data || !data_len) continue;

		new_subchunk = djvupureRawChunkCreate(pm44_sign, data, data_len);
		if(!new_subchunk) {
			djvupureChunkFree(chunk);

			return false;
		}

		if(!djvupureContainerInsertChunk(chunk, new_subchunk, djvupureContainerSize(chunk))) {
			djvupureChunkFree(chunk);
			djvupureChunkFree(new_subchunk);

			return false;
		}
	}

	djvupureFileSetIoCallbacks(&io);

	fctx = djvupureFileOpenW(param, true);
	if(!fctx) {
		djvupureChunkFree(chunk);

		return false;
	}

	if(!djvupureChunkRender(chunk, &io, fctx)) {
		djvupureChunkFree(chunk);
		djvupureFileClose(fctx);

		return false;
	}

	djvupureChunkFree(chunk);
	djvupureFileClose(fctx);

	return true;
}

bool ExtractChunkFromPage(djvupure_chunk_t *page, const uint8_t sign[4], wchar_t *param)
{

	if(!memcmp(sign, "FG44", 4) || !memcmp(sign, "BG44", 4)) { // Process FG44 or BG44 chunk
		return ExtractIW44ChunkFromPage(page, sign, param);
	} else { // Process others
		djvupure_io_callback_t io;
		void *fctx = 0;
		djvupure_chunk_t *chunk;
		void *data;
		size_t data_len;

		chunk = djvupureContainerGetSubchunkBySign(page, sign, 0, 0);
		if(!chunk) return false;

		djvupureRawChunkGetDataPointer(chunk, &data, &data_len);

		if(!data || !data_len) return false;
	
		djvupureFileSetIoCallbacks(&io);

		fctx = djvupureFileOpenW(param, true);
		if(!fctx) return false;

		if(io.callback_write(fctx, data, data_len) != data_len) {
			djvupureFileClose(fctx);

			return false;
		}

		djvupureFileClose(fctx);
	}

	return true;
}
