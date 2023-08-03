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

#ifndef _WIN32
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _fseeki64 fseeko64
#define _ftelli64 ftello64
#include "../unixsupport/wfopen.h
#endif

#include "../include/djvupure.h"

#include <stdio.h>

DJVUPURE_API uint32_t DJVUPURE_APIENTRY_EXPORT djvupureIOGetStructHash(void)
{
	return (uint32_t)(sizeof(djvupure_io_callback_t)%(UINT16_MAX+1))*(UINT16_MAX+1)+DJVUPURE_VERSION_MAJOR%(UINT16_MAX+1);
}

DJVUPURE_API void * DJVUPURE_APIENTRY_EXPORT djvupureFileOpenA(char *filename, bool write)
{
#ifdef _WIN32
	FILE *f;
	
	if(fopen_s(&f, filename, (write)?"wb+":"rb+"))
		return 0;
	else
		return f;
#else
	return fopen(filename, (write)?"wb+":"rb+");
#endif
}

DJVUPURE_API void * DJVUPURE_APIENTRY_EXPORT djvupureFileOpenW(wchar_t *filename, bool write)
{
#ifdef _WIN32
	FILE *f;
	
	if(_wfopen_s(&f, filename, (write)?L"wb+":L"rb+"))
		return 0;
	else
		return f;
#else
	return _wfopen(filename, (write)?L"wb+":L"rb+");
#endif
}

DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupureFileClose(void *fctx)
{
	fclose((FILE *)fctx);
}

static size_t DJVUPURE_APIENTRY djvupureFileRead(void *fctx, void *buf, size_t size)
{
	return fread(buf, 1, size, (FILE *)fctx);
}

static size_t DJVUPURE_APIENTRY djvupureFileWrite(void *fctx, const void *buf, size_t size)
{
	return fwrite(buf, 1, size, (FILE *)fctx);
}

static int DJVUPURE_APIENTRY djvupureFileSeek(void *fctx, int64_t offset, int origin)
{
	int fseek_origin;
	
	switch(origin) {
		case DJVUPURE_IO_SEEK_CUR:
			fseek_origin = SEEK_CUR;
			break;
		case DJVUPURE_IO_SEEK_END:
			fseek_origin = SEEK_END;
			break;
		case DJVUPURE_IO_SEEK_SET:
			fseek_origin = SEEK_SET;
			break;
		default:
			return -1;
	}
	
	return _fseeki64((FILE *)fctx, offset, fseek_origin);
}

static int64_t DJVUPURE_APIENTRY djvupureFileTell(void *fctx)
{
	return _ftelli64((FILE *)fctx);
}

DJVUPURE_API void DJVUPURE_APIENTRY_EXPORT djvupureFileSetIoCallbacks(djvupure_io_callback_t *io)
{
	io->hash = djvupureIOGetStructHash();
	io->callback_read = djvupureFileRead;
	io->callback_write = djvupureFileWrite;
	io->callback_seek = djvupureFileSeek;
	io->callback_tell = djvupureFileTell;
}
