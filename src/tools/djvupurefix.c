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

#ifndef _WIN32
#include "../unixsupport/wtoi.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

void FixPage(djvupure_chunk_t *page, uint16_t new_dpi);

int wmain(int argc, wchar_t **argv)
{
	djvupure_io_callback_t io;
	djvupure_chunk_t *document = 0;
	uint16_t new_dpi = 0;
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

		wprintf(L"%ls document.djvu [new_dpi]\n",
			command);

		return EXIT_SUCCESS;
	}
	
	if(argc > 2) new_dpi = (uint16_t)(_wtoi(argv[2]));
	
	djvupureFileSetIoCallbacks(&io);
	
	fctx = djvupureFileOpenW(argv[1], false);
	if(!fctx) goto FINAL;
	
	document = djvupureDocumentRead(&io, fctx);
	djvupureFileClose(fctx);
	fctx = 0;
	if(!document) goto FINAL;
	
	if(djvupurePageIs(document))
		FixPage(document, new_dpi);
	else {
		size_t nof_chunks, index = 0;
		
		nof_chunks = djvupureContainerSize(document);

		while(true) {
			djvupure_chunk_t *page;
			const uint8_t form_sign[4] = { 'F', 'O', 'R', 'M' };
			const uint8_t page_sign[4] = { 'D', 'J', 'V', 'U' };

			index = djvupureContainerFindSubchunkBySign(document, form_sign, page_sign, index);
			if(index >= nof_chunks) break;

			page = djvupureContainerGetSubchunk(document, index);
			if(page) FixPage(page, new_dpi);

			index++;
		}
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

void FixPage(djvupure_chunk_t *page, uint16_t new_dpi)
{
	djvupure_chunk_t *info_chunk;
	djvupure_page_info_t info_struct;

	info_chunk = djvupureContainerGetSubchunk(page, 0);
	if(!info_chunk) return;

	if(!djvupureInfoIs(info_chunk)) return;

	if(!djvupureInfoGet(info_chunk, &info_struct)) return;

	if(new_dpi) info_struct.dpi = new_dpi;
	if(info_struct.rotation == 0) info_struct.rotation = 1;

	djvupureInfoPut(info_chunk, &info_struct);
}
