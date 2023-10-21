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
#include "../../src/djvupure_sign.h"
#include "aux_insert.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

int wmain(int argc, wchar_t **argv)
{
	djvupure_io_callback_t io;
	djvupure_chunk_t *page;
	size_t nof_chunks;
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

		wprintf(L"%ls page.djvu CHUNK1=param1 CHUNK2=param2 ...\n"
			L"\tfor INFO chunk there are special parameters \"INFO=width,height,dpi,rotation,gamma\", some parameters can be empty\n"
			L"\t\trotation 1 is 0deg, 5 - 90deg, 2 - 180deg, 6 - 270deg\n"
			L"\t\tgamma 22 stands for gamma value 2.2\n"
			L"\t\tIf no chunk presented it will be generated from the following chunks in following order\n"
			L"\t\t\tSjbz, Smmr, BG44, BGjp\n"
			L"\tfor other chunks parameter is a path to a file containing chunk data (i.e. \"Sjbz=page.sjbz\")\n"
			L"\tChunks FG44 and BG44 should be a IFF85 file with a group of PM44 subchunks (can be created with extract utility)\n"
			L"\t\tchunk FG44 extracts only one PM44 chunk from file\n"
			L"\t\tchunk BG44 can be defined like \"BG44=file.bg44,n\" where n is a number of chunks to copy\n",
			command);

		return EXIT_SUCCESS;
	}

	page = djvupurePageCreate();
	if(!page) goto FINAL;

	djvupureFileSetIoCallbacks(&io);

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
		
		if(!InsertChunkToPage(page, sign, chunk_filename))
			wprintf(L"Can't append chunk %.4hs\n", sign);
	}

	// Check if INFO chunk existed. If not - generate it
	nof_chunks = djvupureContainerSize(page);

	if(djvupureContainerFindSubchunkBySign(page, djvupure_info_sign, 0, 0) == nof_chunks) {
		djvupure_page_info_t info;
		djvupure_chunk_t *info_chunk;
		bool info_found = false;
		size_t chunk_no;

		if((chunk_no = djvupureContainerFindSubchunkBySign(page, djvupure_sjbz_sign, 0, 0)) != nof_chunks) {
			wprintf(L"Generate INFO chunk: Found Sjbz chunk, but it is unsupported\n");
		}
		if(!info_found && (chunk_no = djvupureContainerFindSubchunkBySign(page, djvupure_smmr_sign, 0, 0)) != nof_chunks) {
			djvupure_chunk_t *smmr;

			smmr = djvupureContainerGetSubchunk(page, chunk_no);

			if(smmr) {
				if(!djvupureSmmrGetInfo(smmr, &(info.width), &(info.height)))
					wprintf(L"Generate INFO chunk: Can't get info from Smmr chunk\n");
				else
					info_found = true;
			} else {
				wprintf(L"Generate INFO chunk: Can't retrieve Smmr chunk\n");
			}
		}
		if(!info_found && (chunk_no = djvupureContainerFindSubchunkBySign(page, djvupure_bg44_sign, 0, 0)) != nof_chunks) {
			wprintf(L"Generate INFO chunk: Found BG44 chunk, but it is unsupported\n");
		}
		if(!info_found && (chunk_no = djvupureContainerFindSubchunkBySign(page, djvupure_bgjp_sign, 0, 0)) != nof_chunks) {
			djvupure_chunk_t *bgjp;

			bgjp = djvupureContainerGetSubchunk(page, chunk_no);

			if(bgjp) {
				if(!djvupureBGjpGetInfo(bgjp, &(info.width), &(info.height)))
					wprintf(L"Generate INFO chunk: Can't get info from BGjp chunk\n");
				else
					info_found = true;
			} else {
				wprintf(L"Generate INFO chunk: Can't retrieve BGjp chunk\n");
			}
		}

		if(info_found) {
			info.dpi = 300; 
			info.gamma = 22;
			info.rotation = 1;

			info_chunk = djvupureInfoCreate(info);
			if(info_chunk) {
				if(!djvupureContainerInsertChunk(page, info_chunk, 0))
					wprintf(L"Can't insert default INFO chunk\n");
			} else
				wprintf(L"Can't create default INFO chunk\n");
		} else
			wprintf(L"No INFO chunk generated\n");
	}

	fctx = djvupureFileOpenW(argv[1], true);
	if(!fctx) goto FINAL;

	if(!djvupureDocumentRender(page, &io, fctx)) goto FINAL;

	result = EXIT_SUCCESS;

FINAL:
	if(fctx) djvupureFileClose(fctx);
	if(page) djvupureChunkFree(page);

	return result;
}
