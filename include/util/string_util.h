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

#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <stdlib.h>

#define SUB_STERING_MAX_LENGTH      (20)
#define EVENT_TIME_LENGTH           (16)
#define CLIENT_TIME_LENGTH          (10)
#define LONG_LONG_MAX_LENGTH        (20)
#define LOCAL_TIME_LENGTH           (19)
#define LOCAL_TIME_WITH_MS_LENGTH   (LOCAL_TIME_LENGTH + 4)
#define LOCAL_TIME_COMPACT_LENGTH   (14)

int StringLength(const char *str);
char *StrInStr(const char *str, const char *subStr);
int String2Int(const char *value);
char *CombineStrings(int strAmount, const char *str1, ...);
void StringMalloc(char **str, int length);
int CopyStrValue(char **dst, const char *src, int length);
char *GetClientTimesStamp(void);
char *GetEventTimesStamp(void);
char *Timeval2Str(const struct timeval *tv);
char *Timeval2CompactStr(const struct timeval *tv);
char *GetLocalTimeWithMs(void);
char *PrependLocalTimeWithMs(const char *);
int GetSubStrIndex(const char *str, const char *substr);
unsigned long long getTime(void);
int StrEndWith(const char *str, const char *suffix);
long long getLLongValueFromStr(const char *str, const char *subStr);
int gZIPCompress(const char *src, int srcLength, unsigned char *dest, int destLength);
char *ReassignMemory(char *oldMemory, unsigned int resultLen);

#define MemFree(ptr)    \
do {                    \
        free(*(ptr));   \
        *(ptr) = NULL;  \
} while (0)

#endif /* STRING_UTIL_H */
