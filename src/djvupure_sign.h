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
/*Internal module chunk signatures*/

#ifndef DJVUPURE_SIGN_H
#define DJVUPURE_SIGN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern const uint8_t djvupure_atnt_sign[4];

extern const uint8_t djvupure_form_sign[4];

extern const uint8_t djvupure_document_sign[4];
extern const uint8_t djvupure_page_sign[4];

extern const uint8_t djvupure_dir_sign[4];

extern const uint8_t djvupure_info_sign[4];

extern const uint8_t djvupure_bg44_sign[4];
extern const uint8_t djvupure_bgjp_sign[4];
extern const uint8_t djvupure_sjbz_sign[4];
extern const uint8_t djvupure_smmr_sign[4];
extern const uint8_t djvupure_fg44_sign[4];
extern const uint8_t djvupure_fgjp_sign[4];

#ifdef __cplusplus
}
#endif

#endif