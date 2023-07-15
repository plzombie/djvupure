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

static void PrintChunks(djvupure_chunk_t *chunk, size_t level);

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

	if(argc <= 1) {
		wchar_t *command;
		
		command = argv[0];
		
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
	
	document = djvuDocumentRead(&io, fctx);
	djvupureFileClose(fctx);
	if(!document) {
		wprintf(L"Can't open document\n");
		
		return EXIT_FAILURE;
	}
	
	PrintChunks(document, 0);
	
	
	
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

static void PrintChunks(djvupure_chunk_t *chunk, size_t level)
{
	for(size_t i = 0; i < level; i++) wprintf(L"-");
	
	if(djvupureContainerCheckSign(chunk->sign)) {
		size_t container_size;
		uint8_t *subsign;
		
		subsign = (char *)(chunk->ctx);
		wprintf(L"Chunk %.4hs:%.4hs\n", chunk->sign, subsign);
		level++;
		
		container_size = djvupureContainerSize(chunk);
		
		for(size_t index = 0; index < container_size; index++) {
			djvupure_chunk_t *subchunk;
			
			subchunk = djvupureContainerGetSubchunk(chunk, index);
			if(subchunk) PrintChunks(subchunk, level);
		}
	} else
		wprintf(L"Chunk %.4hs\n", chunk->sign);
}
