/* Copyright (c) <2020>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * &Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "securec.h"
#include <time.h>
#include "string_util.h"
#include "zlib.h"


int StringLength(char *str)
{
    if (str == NULL) {
        return 0;
    }
    int len = 0;
    char *temp_str = str;
    while (*temp_str++ != '\0') {
        len++;
    }
    return len;
}

size_t ConstStringLength(const char *_Str)
{
    return strlen(_Str);
}

char *StrInStr(const char *_Str, const char *_SubStr)
{
    if (_Str == NULL || _SubStr == NULL) {
        return NULL;
    }

    return strstr(_Str, _SubStr);
}

int String2Int(const char *value)
{
    if (value == NULL) {
        return -1;
    }
    return atoi(value);
}


void MemFreeConst(const char **str)
{
    if (*str != NULL) {
        free((void *)(*str));
        *str = NULL;
    }
}
void MemFree(char **str)
{
    if (*str != NULL) {
        free(*str);
        *str = NULL;
    }
}

void StringMalloc(char **str, int length)
{
    if (length <= 0) {
        return;
    }
    *str = malloc(length);
    if (*str == NULL) {
        return;
    }
    memset_s(*str, length, 0, length);
}

char *CombineStrings(int strAmount, char *str1, ...)
{
    int length = StringLength(str1) + 1;
    if (length == 1) {
        return NULL;
    }

    char *result = malloc(length);
    if (result == NULL) {
        return NULL;
    }
    char *temStr;

    int ret = strcpy_s(result, length, str1);
    if (ret != 0) {
        return NULL;
    }

    va_list args;
    va_start(args, str1);

    while (--strAmount > 0) {
        temStr = va_arg(args, char *);
        if (temStr == NULL) {
            continue;
        }
        length = length + StringLength(temStr);

        char *p = (char *)malloc(length);
        if (p == NULL) {
            MemFree(&result);
            return NULL;
        }

        ret = strcpy_s(p, length, result);
        if (ret != 0) {
            MemFree(&result);
            MemFree(&p);
            return NULL;
        }

        ret = strcat_s(p, length, temStr);
        if (ret != 0) {
            MemFree(&p);
            return NULL;
        }
        MemFree(&result);
        result = p;
    }
    va_end(args);

    return result;
}

/* *NOTE: "*dst" will be "malloc" inside this function, and the invocation needs to free it after used.
 * If this function is recalled with the same "**dst", you should free the pointer "*dst" before invoking this function
 * in case of memory leak.
 */
int CopyStrValue(char **dst, const char *src, int length)
{
    if (length <= 0) {
        return 0;
    }
    *dst = malloc(length + 1);
    if (*dst == NULL) {
        return -1;
    }
    int ret = memset_s(*dst, length + 1, 0, length);
    if (ret != 0) {
        return -1;
    }

    ret = strncat_s(*dst, length + 1, src, length);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

// NOTE: the invocation need to free the return char pointer.
// return parameter e.g. 20190531T011540Z
char *GetEventTimesStamp()
{
    time_t t;
    struct tm *lt;

    time(&t);        // get Unix time stamp
    lt = gmtime(&t); // transform into time struct
    if (lt == NULL) {
        return NULL;
    }
    char *dest_str = malloc(EVENT_TIME_LENGTH + 1); // length of yyyyMMDDThhmmssZ + 1
    if (dest_str == NULL) {
        return NULL;
    } else {
        memset_s(dest_str, EVENT_TIME_LENGTH + 1, 0, EVENT_TIME_LENGTH + 1);
        snprintf_s(dest_str, EVENT_TIME_LENGTH + 1, EVENT_TIME_LENGTH, "%d%.2d%.2dT%.2d%.2d%.2dZ", lt->tm_year + 1900,
            lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
        return dest_str;
    }
}

unsigned long long getTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


// NOTE: the invocation need to free the return char pointer.
// return parameter e.g. 2019053101
char *GetClientTimesStamp()
{
    time_t t;
    struct tm *lt;

    time(&t);        // get Unix time stamp
    lt = gmtime(&t); // transform into time struct
    if (lt == NULL) {
        return NULL;
    }
    char *dest_str = malloc(CLIENT_TIME_LENGTH + 1); // length of yyyyMMDDhh + 1
    if (dest_str == NULL) {
        return NULL;
    } else {
        memset_s(dest_str, CLIENT_TIME_LENGTH + 1, 0, CLIENT_TIME_LENGTH + 1);
        snprintf_s(dest_str, CLIENT_TIME_LENGTH + 1, CLIENT_TIME_LENGTH, "%d%.2d%.2d%.2d", lt->tm_year + 1900,
            lt->tm_mon + 1, lt->tm_mday, lt->tm_hour);
        return dest_str;
    }
}

/*
 * the max length of substring is SUB_STERING_MAX_LENGTH,
 *  */
int GetSubStrIndex(const char *str, const char *substr)
{
    if (str == NULL || substr == NULL) {
        return -1;
    }
    int len = strlen(str);
    int subLen = strlen(substr);
    if (len == 0 || substr == 0 || subLen >= SUB_STERING_MAX_LENGTH) {
        return -1;
    }
    int n = 0;
    char tmp[SUB_STERING_MAX_LENGTH] = { "" };

    while (len - n >= subLen && subLen >= 0) {
        int ret = strncpy_s(tmp, sizeof(tmp), str + n, subLen);
        if (ret != 0) {
            return -1;
        }
        tmp[subLen] = '\0';
        if (strcmp(substr, tmp) == 0)
            return n;
        n++;
    }

    return -1;
}

/*
 * get the value of long long type from string
*  */
long long getLLongValueFromStr(const * str, const * subStr)
{
    char *version_tmp = strstr(str, subStr);
    char buf[LONG_LONG_MAX_LENGTH+1] = {0};
    uInt i = 0;
    uInt j = 0;
    for (i = 0; i < strlen(version_tmp); i++) {
        if (version_tmp[i] >= '0' && version_tmp[i] <= '9') {
            buf[j] = version_tmp[i];
            j++;
            if (j > LONG_LONG_MAX_LENGTH) {
                return -1;
            }
        } else {
            if (j > 0) {
                break;
            }
        }
    }

    buf[j] = '\0';
    char *end = NULL;
    long long version = strtoll(buf, &end, 10);
    return version;
}


int gZIPCompress(const char *src, int srcLength, unsigned char *dest, int destLength)
{
    z_stream c_stream;
    unsigned int encoding = 16;
    int ret = 0;

    if (src != NULL && srcLength > 0 && destLength >= 0) {
        c_stream.zalloc = NULL;
        c_stream.zfree = NULL;
        c_stream.opaque = NULL;
        if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, (int)(MAX_WBITS | encoding), 8,
            Z_DEFAULT_STRATEGY) != Z_OK)
            return -1;
        c_stream.next_in = (Bytef *)src;
        c_stream.avail_in = srcLength;
        c_stream.next_out = (Bytef *)dest;
        c_stream.avail_out = destLength;
        while (c_stream.avail_in != 0 && c_stream.total_out < (uInt)destLength) {
            if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK) {
                return -1;
            }
        }
        if (c_stream.avail_in != 0) {
            return (int)c_stream.avail_in;
        }
        for (;;) {
            if ((ret = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END) {
                break;
            }
            if (ret != Z_OK) {
                return -1;
            }
        }
        if (deflateEnd(&c_stream) != Z_OK) {
            return -1;
        }
        return (int)c_stream.total_out;
    }
    return -1;
}
