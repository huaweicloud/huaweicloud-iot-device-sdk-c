/*
 * Copyright (c) 2020-2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HW_TYPE_H
#define HW_TYPE_H

#if defined(WIN32) || defined(WIN64) && defined(EXPORT_AGENTLITE)
#define HW_API_FUNC __declspec(dllexport)
#else
#define HW_API_FUNC
#endif

#define HW_TRUE      1 /**< Indicates true */
#define HW_FALSE     0 /**< Indicates false */

#define HW_SUCCESS   0 /**< Indicates success */
#define HW_FAILED    1 /**< Indicates failed */
#define HW_NULL     ((void *)0) /**< Indicates null ptr */

typedef int              HW_INT; /**< Indicates type of int. */
typedef unsigned int     HW_UINT; /**< Indicates type of unsigned int. */
typedef char             HW_CHAR; /**< Indicates type of char. */
typedef unsigned char    HW_UCHAR;/**< Indicates type of unsigned char. */
typedef int              HW_BOOL; /**< Indicates type of bool. */
typedef void             HW_VOID; /**< Indicates type of void. */
typedef long             HW_LONG; /**< Indicates type of bool. */
typedef double           HW_DOUBLE; /**< Indicates type of bool. */
typedef long long        HW_LLONG; /**< Indicates type of bool. */

/**< Indicates type of byte array. */
typedef struct {
    HW_UINT uiLen; /**< Indicates length of byte array */
    HW_CHAR *pcByte; /**< Indicates byte value of byte array */
} HW_BYTES;

#endif
