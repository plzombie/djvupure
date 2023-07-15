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

djvupure_chunk_t * CreateInfoChunkFromParams(wchar_t *params);
djvupure_chunk_t * CreateRawChunkFromFile(uint8_t sign[4], wchar_t *chunk_filename);

int wmain(int argc, wchar_t **argv)
{
	djvupure_io_callback_t io;
	djvupure_chunk_t *page;
	void *fctx = 0;
	int result = EXIT_FAILURE;

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
			L"\tfor other chunks parameter is a path to a file containing chunk data (i.e. \"Sjbz=page.sjbz\")\n",
			command);

		return EXIT_SUCCESS;
	}

	page = djvupurePageCreate();
	if(!page) goto FINAL;

	djvupureFileSetIoCallbacks(&io);

	for(int i = 2; i < argc; i++) {
		uint8_t sign[4];
		djvupure_chunk_t *chunk = 0;
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

		if(!memcmp(sign, "INFO", 4)) { // Process INFO chunk
			chunk = CreateInfoChunkFromParams(chunk_filename);
		} else { // Process others
			chunk = CreateRawChunkFromFile(sign, chunk_filename);
		}

		if(chunk) {
			size_t index;

			index = djvupureContainerSize(page);

			if(!djvupureContainerInsertChunk(page, chunk, index)) {
				wprintf(L"Can't append chunk %.4hs\n", sign);

				djvupureChunkFree(chunk);
			}
		} else
			wprintf(L"Can't create chunk %.4hs\n", sign);
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

djvupure_chunk_t * CreateInfoChunkFromParams(wchar_t *params)
{
	djvupure_page_info_t info;
	size_t i = 0;
	bool is_gamma_set = false, is_rotation_set = false;

	memset(&info, 0, sizeof(djvupure_page_info_t));

	// Parameters are width,height,dpi,rotation,gamma

	// Read width
	while(params[i] != ',' && params[i] != 0) {
		info.width *= 10;
		info.width += (params[i]-'0')%10;
		i++;
	}
	if(params[i] == ',') i++;

	// Read height
	while(params[i] != ',' && params[i] != 0) {
		info.height *= 10;
		info.height += (params[i]-'0')%10;
		i++;
	}
	if(params[i] == ',') i++;

	// Read dpi
	while(params[i] != ',' && params[i] != 0) {
		info.dpi *= 10;
		info.dpi += (params[i]-'0')%10;
		i++;
	}
	if(params[i] == ',') i++;

	// Read rotation
	while(params[i] != ',' && params[i] != 0) {
		is_rotation_set = true;
		info.rotation *= 10;
		info.rotation += (params[i]-'0')%10;
		i++;
	}
	if(params[i] == ',') i++;

	// Read gamma
	while(params[i] != ',' && params[i] != 0) {
		is_gamma_set = true;
		info.gamma *= 10;
		info.gamma += (params[i]-'0')%10;
		i++;
	}
	if(params[i] == ',') i++;

	if(!is_gamma_set) info.gamma = 22;
	if(!is_rotation_set) info.rotation = 1;

	return djvupureInfoCreate(info);
}

djvupure_chunk_t * CreateRawChunkFromFile(uint8_t sign[4], wchar_t *chunk_filename)
{
	djvupure_io_callback_t io;
	djvupure_chunk_t *chunk = 0;
	void *chunk_fctx = 0;
	void *chunk_data = 0;
	int64_t chunk_data_len;

	djvupureFileSetIoCallbacks(&io);
			
	chunk_fctx = djvupureFileOpenW(chunk_filename, false);
	if(!chunk_fctx) {
		wprintf(L"Can't open file \"%ls\" for chunk\n", chunk_filename);

		goto FINAL;
	}

	if(io.callback_seek(chunk_fctx, 0, DJVUPURE_IO_SEEK_END)) {
		wprintf(L"Can't process file \"%ls\" for chunk\n", chunk_filename);
		goto FINAL;
	}

	chunk_data_len = io.callback_tell(chunk_fctx);
	if(chunk_data_len > SIZE_MAX) {
		wprintf(L"File \"%ls\" is too large\n", chunk_filename);
		goto FINAL;
	}

	if(io.callback_seek(chunk_fctx, 0, DJVUPURE_IO_SEEK_SET)) {
		wprintf(L"Can't process file \"%ls\" for chunk\n", chunk_filename);
		goto FINAL;
	}

	chunk_data = malloc((size_t)chunk_data_len);
	if(!chunk_data) {
		wprintf(L"File \"%ls\" is too large\n", chunk_filename);
		goto FINAL;
	}

	if(io.callback_read(chunk_fctx, chunk_data, (size_t)chunk_data_len) != chunk_data_len) {
		wprintf(L"Can't process file \"%ls\" for chunk\n", chunk_filename);
		goto FINAL;
	}

	chunk = djvupureRawChunkCreate(sign, chunk_data, (size_t)chunk_data_len);

FINAL:
	if(chunk_fctx) djvupureFileClose(chunk_fctx);
	if(chunk_data) free(chunk_data);

	return chunk;
}
