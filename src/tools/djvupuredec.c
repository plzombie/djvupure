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

#include "../all2ppm/include/ppm_save.h"

#ifndef _WIN32
#include "../unixsupport/wtoi.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

bool RenderPageToFile(djvupure_chunk_t *page, djvupure_chunk_t *document, int format, wchar_t *fname);

enum {
	DJVUPUREDEC_FORMAT_PNM
};

int wmain(int argc, wchar_t **argv)
{
	djvupure_io_callback_t io;
	djvupure_chunk_t *document = 0, *page;
	size_t index = 0;
	void *fctx = 0;
	int format = DJVUPUREDEC_FORMAT_PNM;
	int result = EXIT_FAILURE, arg_start = 2;

	setlocale(LC_CTYPE, "");

	if(argc <= 3) {
		wchar_t *command, *_command;
		
		command = argv[0];
		_command = wcsrchr(command, '\\');
		if(_command) command = _command+1;
		_command = wcsrchr(command, '/');
		if(_command) command = _command+1;

		wprintf(L"%ls -format=fmt [-page=pagenum] document.djvu output.fmt\n"
			L"\tfmt is a file format. The only file format supported is pnm\n"
			L"\tpagenum is a single page number. Default is 1\n",
			command);

		return EXIT_SUCCESS;
	}
	
	if(!wcsncmp(argv[1], L"-format=", 8)) {
		if(!wcscmp(argv[1]+8, L"pnm")) {
			format = DJVUPUREDEC_FORMAT_PNM;
		} else {
			wprintf(L"Error: format can be only pnm\n");

			return EXIT_FAILURE;
		}
	} else {
		wprintf(L"Please specify output file format\n");

		return EXIT_FAILURE;
	}

	if(!wcsncmp(argv[2], L"-page=", 6)) {
		index = _wtoi(argv[1]+6)-1;
		arg_start++;
	}

	if(argc-arg_start < 2) {
		wprintf(L"Please specify document name and output filename\n");

		return EXIT_FAILURE;
	}

	djvupureFileSetIoCallbacks(&io);
	
	fctx = djvupureFileOpenW(argv[arg_start], false);
	if(!fctx) goto FINAL;
	
	document = djvupureDocumentRead(&io, fctx);
	djvupureFileClose(fctx);
	fctx = 0;
	if(!document) goto FINAL;
	
	page = djvupureDocumentGetPage(document, index, djvupureFileOpenU8, djvupureFileClose);
	if(!page) goto FINAL;
	
	if(!RenderPageToFile(page, document, format, argv[arg_start+1])) {
		wprintf(L"Can't decode page to file\n");
	}

	if(!djvupureDocumentPutPage(document, page, false, djvupureFileOpenU8, djvupureFileClose)) {
		wprintf(L"Can't put page back\n");
	}
	
	result = EXIT_SUCCESS;
	
FINAL:
	if(fctx) djvupureFileClose(fctx);
	if(document) djvupureChunkFree(document);
	
	return result;
}

bool RenderPageToFile(djvupure_chunk_t *page, djvupure_chunk_t *document, int format, wchar_t *fname)
{
	void *image_renderer_ctx = 0, *image_buffer = 0;
	uint16_t image_width, image_height;
	uint8_t image_channels;
	bool result = false;

	image_renderer_ctx = djvupurePageImageRendererCreate(page, document, &image_width, &image_height, &image_channels);
	if(!image_renderer_ctx) return false;

	if(SIZE_MAX/image_width < image_height) goto FINAL;
	if(SIZE_MAX/((size_t)image_width*(size_t)image_height) < image_channels) goto FINAL;

	image_buffer = malloc((size_t)image_width*(size_t)image_height*(size_t)image_channels);
	if(!image_buffer) goto FINAL;

	while(1) {
		int step;

		step = djvupurePageImageRendererNext(image_renderer_ctx, image_buffer);
		
		if(step == DJVUPURE_IMAGE_RENDERER_ERROR) goto FINAL;
		else if(step == DJVUPURE_IMAGE_RENDERER_LAST_STAGE) break;
		else if(step != DJVUPURE_IMAGE_RENDERER_NEXT_STAGE) goto FINAL;
	}

	if(format == DJVUPUREDEC_FORMAT_PNM) {
		djvupure_io_callback_t io;
		void *fctx = 0;

		if(image_channels != 1 && image_channels != 3) goto FINAL;

		djvupureFileSetIoCallbacks(&io);

		fctx = djvupureFileOpenW(fname, true);
		if(!fctx) goto FINAL;

		if(image_channels == 1) {
			if(pbmSave(image_width, image_height, image_buffer, (FILE *)fctx)) result = true;
		} else if(image_channels == 3) {
			if(ppmSave(image_width, image_height, image_channels, image_buffer, (FILE *)fctx)) result = true;
		}

		djvupureFileClose(fctx);
	}

FINAL:
	if(image_renderer_ctx) djvupurePageImageRendererDestroy(image_renderer_ctx);
	if(image_buffer) free(image_buffer);

	return result;
}
