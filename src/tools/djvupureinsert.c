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
#include "aux_insert.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

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
			L"\tfor INFO chunk there are special parameters \"INFO=width,height,dpi,rotation,gamma\", some parameters can be empty\n"
			L"\t\trotation 1 is 0deg, 5 - 90deg, 2 - 180deg, 6 - 270deg\n"
			L"\t\tgamma 22 stands for gamma value 2.2\n"
			L"\tfor other chunks parameter is a path to a file containing chunk data (i.e. \"Sjbz=page.sjbz\")\n"
			L"\tChunks FG44 and BG44 should be a IFF85 file with a group of PM44 subchunks (can be created with extract utility)\n"
			L"\t\tchunk FG44 extracts only one PM44 chunk from file\n"
			L"\t\tchunk BG44 can be defined like \"BG44=file.bg44,n\" where n is a number of chunks to copy\n",
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
		
		if(!InsertChunkToPage(page, sign, chunk_filename))
			wprintf(L"Can't append chunk %.4hs\n", sign);
	}

	fctx = djvupureFileOpenW(argv[1], true);
	if(!fctx) goto FINAL;

	if(!djvupureDocumentRender(document, &io, fctx)) goto FINAL;
	
	result = EXIT_SUCCESS;
	
FINAL:
	if(fctx) djvupureFileClose(fctx);
	if(document) djvupureChunkFree(document);
	
	return result;
}
