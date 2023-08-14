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

static void PrintChunks(djvupure_chunk_t *chunk, size_t level, size_t offset);

int wmain(int argc, wchar_t **argv)
{
	wchar_t *filename;
	void *fctx;
	djvupure_io_callback_t io;
	djvupure_chunk_t *document;

#ifdef _DEBUG
	uint32_t major = 0, minor = 0, revision = 0;

	wprintf(L"io struct hash %u\n", djvupureIOGetStructHash());
	wprintf(L"chunk struct hash %u\n", djvupureChunkGetStructHash());
	djvupureGetVersion(&major, &minor, &revision);
	wprintf(L"Version %u.%u.%u (lib %u.%u.%u)\n",
		DJVUPURE_VERSION_MAJOR, DJVUPURE_VERSION_MINOR, DJVUPURE_VERSION_REVISION,
		major, minor, revision);
#endif

	setlocale(LC_CTYPE, "");

	if(argc <= 1) {
		wchar_t *command, *_command;
		
		command = argv[0];
		_command = wcsrchr(command, '\\');
		if(_command) command = _command+1;
		_command = wcsrchr(command, '/');
		if(_command) command = _command+1;
		
		wprintf(L"%ls filename.djvu\n"
			L"\tprints filename.djvu content with a tree\n",
			command);
		
		return EXIT_SUCCESS;
	}
	
	filename = argv[1];
	
	wprintf(L"Opening \"%ls\"\n", filename);
	
	fctx = djvupureFileOpenW(filename, false);
	if(!fctx) {
		wprintf(L"Can't open file\n");
		
		return EXIT_FAILURE;
	}
	
	djvupureFileSetIoCallbacks(&io);
	
	document = djvupureDocumentRead(&io, fctx);
	djvupureFileClose(fctx);
	if(!document) {
		wprintf(L"Can't open document\n");
		
		return EXIT_FAILURE;
	}
	
	PrintChunks(document, 0, 4);
	
	
	
#ifdef _DEBUG
	if(argc > 2) {
		bool result;
		fctx = djvupureFileOpenW(argv[2], true);
		if(!fctx) return EXIT_FAILURE;
		
		result = djvupureDocumentRender(document, &io, fctx);
		djvupureFileClose(fctx);
		
		if(result)
			wprintf(L"Document saved\n");
		else {
			wprintf(L"Can't save document\n");
			djvupureChunkFree(document);
			
			return EXIT_FAILURE;
		}
	}
#endif

	djvupureChunkFree(document);

	return EXIT_SUCCESS;
}

static void PrintChunks(djvupure_chunk_t *chunk, size_t level, size_t offset)
{
	for(size_t i = 0; i < level; i++) wprintf(L"-");
	
	if(djvupureContainerCheckSign(chunk->sign)) {
		size_t container_size;
		uint8_t *subsign;
		
		subsign = (uint8_t *)(chunk->ctx);
		wprintf(L"Chunk %.4hs:%.4hs (size %u offset %u)\n", chunk->sign, subsign, (unsigned int)djvupureChunkSize(chunk), (unsigned int)offset);
		level++;
		
		container_size = djvupureContainerSize(chunk);

		offset += 12;
		
		for(size_t index = 0; index < container_size; index++) {
			djvupure_chunk_t *subchunk;

			if(offset%2) offset++;
			
			subchunk = djvupureContainerGetSubchunk(chunk, index);
			if(subchunk) PrintChunks(subchunk, level, offset);

			offset += djvupureChunkSize(subchunk);
		}
	} else {
		wprintf(L"Chunk %.4hs (size %u offset %u)\n", chunk->sign, (unsigned int)djvupureChunkSize(chunk), (unsigned int)offset);

		if(djvupureDirCheckSign(chunk->sign)) {
			wprintf(L"\t# of pages = %u\n", (unsigned int)(djvupureDirCountPages(chunk)));
		} else if(djvupureInfoCheckSign(chunk->sign)) {
			djvupure_page_info_t info_struct;

			if(djvupureInfoGet(chunk, &info_struct)) {
				wprintf(L"\twidth = %hu\n", info_struct.width);
				wprintf(L"\theignt = %hu\n", info_struct.height);
				wprintf(L"\tdpi = %hu\n", info_struct.dpi);
				wprintf(L"\tgamma = %u.%u\n", (unsigned int)(info_struct.gamma/10), (unsigned int)(info_struct.gamma%10));
				wprintf(L"\trotation %hhu\n", info_struct.rotation);
			}
		}
	}
}
