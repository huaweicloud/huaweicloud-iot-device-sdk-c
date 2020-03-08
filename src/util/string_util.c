/*Copyright (c) <2020>, <Huawei Technologies Co., Ltd>
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
 * */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "string_util.h"

int StringLength(char *str) {
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

size_t ConstStringLength(const char *_Str) {
	return strlen(_Str);
}

char* StrInStr(const char *_Str, const char *_SubStr) {
	if (_Str == NULL || _SubStr == NULL) {
		return NULL;
	}

	return strstr(_Str, _SubStr);
}

int String2Int(const char *value) {
	if (value == NULL) {
		return -1;
	}
	return atoi(value);
}

void* StrMemSet(void *_Dst, int _Val, size_t _Size) {
	memset(_Dst, _Val, _Size);
	return NULL;
}

void MemFree(char **str) {
	if (*str != NULL) {
		free(*str);
		*str = NULL;
	}
}

void StringMalloc(char **str, int length) {
	*str = malloc(length);
	if (*str == NULL) {
		return;
	}
	memset(*str, 0, length);
}

char* CombineStrings(int strAmount, char *str1, ...) {
	int length = StringLength(str1) + 1;
	if (length == 1) {
		return NULL;
	}

	char *result = malloc(length);
	if (result == NULL) {
		return NULL;
	}
	char *temStr;

	strcpy(result, str1);

	va_list args;
	va_start(args, str1);

	while (--strAmount > 0) {
		temStr = va_arg(args, char*);
		if (temStr == NULL) {
			continue;
		}
		length = length + StringLength(temStr);
		result = realloc(result, length);
		if (result == NULL) {
			return NULL;
		}
		strcat(result, temStr);
	}
	va_end(args);

	return result;
}

/**NOTE: "*dst" will be "malloc" inside this function, and the invocation needs to free it after used.
 * If this function is recalled with the same "**dst", you should free the pointer "*dst" before invoking this function in case of memory leak.
 */
int CopyStrValue(char **dst, const char *src, int length) {
	*dst = malloc(length + 1);
	if (*dst == NULL) {
		return -1;
	}
	StrMemSet(*dst, 0, length);
	strncat(*dst, src, length);
	return 0;
}

//NOTE: the invocation need to free the return char pointer.
//return parameter e.g. 20190531T011540Z
char* GetEventTimesStamp() {
	time_t t;
	struct tm *lt;

	time(&t); //get Unix time stamp
	lt = gmtime(&t); //transform into time struct
	if (lt == NULL) {
		return NULL;
	}
	char *dest_str = malloc(EVENT_TIME_LENGTH + 1); //length of yyyyMMDDThhmmssZ + 1
	if (dest_str == NULL) {
		return NULL;
	} else {
		StrMemSet(dest_str, 0, EVENT_TIME_LENGTH + 1);
		snprintf(dest_str, EVENT_TIME_LENGTH + 1, "%d%.2d%.2dT%.2d%.2d%.2dZ", lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
		return dest_str;
	}

}

//NOTE: the invocation need to free the return char pointer.
//return parameter e.g. 2019053101
char* GetClientTimesStamp() {
	time_t t;
	struct tm *lt;

	time(&t); //get Unix time stamp
	lt = gmtime(&t); //transform into time struct
	if (lt == NULL) {
		return NULL;
	}
	char *dest_str = malloc(CLIENT_TIME_LENGTH + 1); //length of yyyyMMDDhh + 1
	if (dest_str == NULL) {
		return NULL;
	} else {
		StrMemSet(dest_str, 0, CLIENT_TIME_LENGTH + 1);
		snprintf(dest_str, CLIENT_TIME_LENGTH + 1, "%d%.2d%.2d%.2d", lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour);
		return dest_str;
	}
}

/*
 * the max length of substring is SUB_STERING_MAX_LENGTH,
 * */
int GetSubStrIndex(const char *str, const char *substr) {
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

	while (len - n >= subLen) {
		strncpy(tmp, str + n, subLen);
		tmp[subLen] = '\0';
		if (strcmp(substr, tmp) == 0)
			return n;
		n++;
	}

	return -1;

}

