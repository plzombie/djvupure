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

#include <stdlib.h>
#include <string.h>

static const uint8_t djvupure_dir_sign[4] = { 'D', 'I', 'R', 'M' };

enum {
	DJVUPURE_DIR_FILE_TYPE_SHARED = 0,
	DJVUPURE_DIR_FILE_TYPE_PAGE = 1,
	DJVUPURE_DIR_FILE_TYPE_THUMB = 2
};

enum {
	DJVUPURE_DIR_FLAG_BUNDLED = 128
};

typedef struct {
	djvupure_chunk_t *chunk;
	char *id;
	char *name;
	char *title;
	unsigned int type;
} djvupure_dir_aux_file_t;

typedef struct {
	size_t nof_files;
	size_t nof_pages;
	djvupure_dir_aux_file_t *files;
	uint8_t flags;
} djvupure_dir_aux_t;

static void DJVUPURE_APIENTRY djvupureDirCallbackFreeAux(void *aux)
{
	djvupure_dir_aux_t *dir_aux;
	if(!aux) return;

	dir_aux = (djvupure_dir_aux_t *)aux;

	if(dir_aux->files) {
		djvupure_dir_aux_file_t *files;
		size_t nof_files;

		files = dir_aux->files;
		nof_files = dir_aux->nof_files;

		for(size_t i = 0; i < nof_files; i++) {
			if(files[i].id) free(files[i].id);
			if(files[i].name) free(files[i].name);
			if(files[i].title) free(files[i].title);
		}
		free(dir_aux->files);
	}

	free(aux);
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureDirCheckSign(const uint8_t sign[4])
{
	if(!memcmp(sign, djvupure_dir_sign, 4))
		return true;
	else
		return false;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureDirIs(djvupure_chunk_t *dir)
{
	if(djvupureChunkGetStructHash() != dir->hash) return false;
	if(!djvupureDirCheckSign(dir->sign)) return false;

	return true;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureDirInit(djvupure_chunk_t *dir, djvupure_chunk_t *document)
{
	djvupure_dir_aux_t *dir_aux;
	void *dir_data;
	size_t dir_data_len, nof_files;

	djvupureRawChunkGetDataPointer(dir, &dir_data, &dir_data_len);

	if(!dir_data || !dir_data_len) return false;

	dir_aux = malloc(sizeof(djvupure_dir_aux_t));
	if(!dir_aux) return false;

	memset(dir_aux, 0, sizeof(djvupure_dir_aux_t));

	// Decode RAW part

	dir_aux->flags = *((uint8_t *)dir_data);
	nof_files = (*((uint8_t *)dir_data+1))*256+*((uint8_t *)dir_data+2);
	dir_aux->nof_files = nof_files;
	dir_aux->files = (djvupure_dir_aux_file_t *)malloc(nof_files *sizeof(djvupure_dir_aux_file_t));
	if(!dir_aux->files) {
		free(dir_aux);

		return false;
	}
	memset(dir_aux->files, 0, nof_files *sizeof(djvupure_dir_aux_file_t));

	if(dir_aux->flags & DJVUPURE_DIR_FLAG_BUNDLED) {
		uint8_t *offset;
		size_t *offsets, nof_subchunks, document_offset = 16;

		offsets = (size_t *)malloc(nof_files*sizeof(size_t));
		if(!offsets) {
			free(dir_aux->files);
			free(dir_aux);

			return false;
		}

		offset = (uint8_t*)dir_data+3;

		for(size_t i = 0; i < nof_files; i++) {
			offsets[i] = offset[0]*16777216+offset[1]*65536+offset[2]*256+offset[3];
			offset += 4;
		}

		nof_subchunks = djvupureContainerSize(document);

		for(size_t index = 0; index < nof_subchunks; index++) {
			djvupure_chunk_t *subchunk;

			if(document_offset%2) document_offset++;

			subchunk = djvupureContainerGetSubchunk(document, index);
			if(!subchunk) continue;
			
			for(size_t i = 0; i < nof_files; i++) {
				if(offsets[i] == document_offset) {
					dir_aux->files[i].chunk = subchunk;

					break;
				}
			}

			document_offset += djvupureChunkSize(subchunk);
		}

		free(offsets);
	}

	// Decode BZ part
	// Here must be BZ decoder but for now we try to fullfill some fields manually
	for(size_t i = 0; i < nof_files; i++) {
		djvupure_dir_aux_file_t *file;

		file = dir_aux->files+i;

		if(file->chunk == 0) continue;

		if(djvupurePageIs(file->chunk)) {
			file->type = DJVUPURE_DIR_FILE_TYPE_PAGE;
			dir_aux->nof_pages++;
		}
	}

	dir->aux = (void *)dir_aux;
	dir->callback_free_aux = djvupureDirCallbackFreeAux;

	return true;
}

DJVUPURE_API size_t DJVUPURE_APIENTRY_EXPORT djvupureDirCountPages(djvupure_chunk_t *dir)
{
	djvupure_dir_aux_t *dir_aux;

	if(!djvupureDirIs(dir)) return 0;
	if(dir->aux == 0) return 0;

	dir_aux = (djvupure_dir_aux_t *)(dir->aux);

	return dir_aux->nof_pages;
}

DJVUPURE_API djvupure_chunk_t * DJVUPURE_APIENTRY_EXPORT djvupureDirGetPage(djvupure_chunk_t *dir, size_t index, djvupure_io_callback_openu8_t openu8, djvupure_io_callback_close_t close)
{
	djvupure_dir_aux_t *dir_aux;
	djvupure_dir_aux_file_t *files;
	size_t nof_files, count = 0;

	(void)openu8;
	(void)close;

	if(!djvupureDirIs(dir)) return 0;
	if(dir->aux == 0) return 0;

	dir_aux = (djvupure_dir_aux_t *)(dir->aux);

	if(index >= dir_aux->nof_pages) return 0;

	nof_files = dir_aux->nof_files;
	files = dir_aux->files;

	for(size_t i = 0; i < nof_files; i++) {
		if(files[i].type == DJVUPURE_DIR_FILE_TYPE_PAGE) {
			if(index != count) {
				count++;

				continue;
			}

			if(!djvupurePageIs(files[i].chunk)) return 0;

			return files[i].chunk;
		}
	}

	return 0;
}

DJVUPURE_API bool DJVUPURE_APIENTRY_EXPORT djvupureDirPutPage(djvupure_chunk_t *dir, djvupure_chunk_t *page, bool changed, djvupure_io_callback_openu8_t openu8, djvupure_io_callback_close_t close)
{
	djvupure_dir_aux_t *dir_aux;

	(void)openu8;
	(void)close;

	if(!djvupureDirIs(dir)) return false;

	if(!dir->aux) return false;

	dir_aux = (djvupure_dir_aux_t *)(dir->aux);

	if((dir_aux->flags & DJVUPURE_DIR_FLAG_BUNDLED) == 0) { // Indirect document
		if(changed) {
			// Put here page back to file
		}

		djvupureChunkFree(page);
	}

	return true;
}
