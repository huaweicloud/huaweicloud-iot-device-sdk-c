/*
 * Copyright (c) 2022-2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#include <stdint.h>
#include "hw_type.h"
#include "string_util.h"
#include "securec.h"
#include "rule_util.h"

HW_BOOL GetIntValueFromJson(int *value, const cJSON *item)
{
    if (!cJSON_IsNumber(item)) {
        return HW_FALSE;
    }
    *value = item->valueint;
    return HW_TRUE;
}

HW_BOOL CStrDuplicate(char **self, const char *s)
{
    if (s == NULL) {
        *self = NULL;
        return HW_TRUE;
    }

    char *buffer = NULL;
    size_t bufferLen = strlen(s) + 1;
    if (bufferLen >= INT32_MAX) {
        return HW_FALSE;
    }

    buffer = (char *)malloc(bufferLen);
    if (buffer == NULL) {
        return HW_FALSE;
    }
    size_t i;
    for (i = 0; i < bufferLen; i++) {
        buffer[i] = s[i];
    }

    if (*self != NULL) {
        MemFree(self);
    }
    *self = buffer;

    return HW_TRUE;
}

HW_BOOL CStrDuplicateNotNULL(char **self, const char *s)
{
    if (s == NULL) {
        return HW_FALSE;
    }

    return CStrDuplicate(self, s);
}

void DynamicItemDummyDtor(void *item)
{
    (void)item;
}

HW_BOOL DynamicItemDummyCtor(void *item)
{
    (void)item;
    return HW_TRUE;
}

#define EXPEND_SIZE 8
#define DEFAULT_RESERVE_SIZE 2
void *DynamicArrayGetItem(DynamicArray *arr, size_t i, size_t itemSize)
{
    return ((uint8_t *)arr->items) + i * itemSize;
}

HW_BOOL DynamicArrayCtor(DynamicArray *arr, size_t itemSize)
{
    arr->size = 0;
    arr->reservedSize = DEFAULT_RESERVE_SIZE;
    arr->items = calloc(DEFAULT_RESERVE_SIZE, itemSize);
    return (arr->items != NULL);
}

void DynamicArrayDtor(DynamicArray *arr, size_t itemSize, DyArrayItemDtorType funcItemDtor)
{
    size_t i;
    for (i = 0; i < arr->size; i++) {
        funcItemDtor(DynamicArrayGetItem(arr, i, itemSize));
    }
    MemFree(&(arr->items));
}

void *DynamicArrayPush(DynamicArray *arr, DyArrayItemCtorType funcItemCtor, size_t itemSize)
{
    size_t index = arr->size;
    if (index == arr->reservedSize) {
        if (arr->reservedSize > SIZE_MAX - EXPEND_SIZE) {
            return NULL;
        }
        size_t targetReservedSize = arr->reservedSize + EXPEND_SIZE;
        if (itemSize == 0) {
            return NULL;
        }
        if (SIZE_MAX / itemSize < targetReservedSize) {
            return NULL;
        }

        void *newBlock = calloc(targetReservedSize, itemSize);
        if (newBlock == NULL) {
            return NULL;
        }
        if (arr->reservedSize != 0) {
            if (memcpy_s(newBlock, targetReservedSize * itemSize, arr->items, arr->reservedSize * itemSize) != EOK) {
                DEVICE_RULE_ERROR("[push]memcpy_s failed\n");
                MemFree(&(newBlock));
                return NULL;
            }
        }

        MemFree(&(arr->items));

        arr->items = newBlock;
        arr->reservedSize = targetReservedSize;
    }
    if (!funcItemCtor(DynamicArrayGetItem(arr, index, itemSize))) {
        return NULL;
    }
    ++arr->size;
    return DynamicArrayGetItem(arr, index, itemSize);
}

HW_BOOL DynamicEmpty(const DynamicArray *arr)
{
    return (arr->size == 0);
}

void DynamicArrayRemoveItem(DynamicArray *arr, void *element, size_t itemSize, DyArrayItemDtorType funcItemDtor)
{
    funcItemDtor(element);
    void *end = DynamicArrayGetItem(arr, arr->size, itemSize);
    void *start = ((uint8_t *)element) + itemSize;
    ptrdiff_t size = (ptrdiff_t)((uintptr_t)end - (uintptr_t)start);
    arr->size -= 1;
    if (size == 0) {
        return;
    }

    errno_t result = memmove_s(element, size, start, size);
    if (result != EOK) {
        DEVICE_RULE_ERROR("memmove_s failed due to error:%d\n", result);
        return;
    }
}

HW_BOOL DynamicArrayPop(DynamicArray *arr, size_t itemSize, DyArrayItemDtorType funcItemDtor)
{
    size_t index = arr->size - 1;
    funcItemDtor(DynamicArrayGetItem(arr, index, itemSize));
    return DynamicArrayPopNoDtor(arr, itemSize);
}

HW_BOOL DynamicArrayPopNoDtor(DynamicArray *arr, size_t itemSize)
{
    if (arr->size == 0) {
        return HW_FALSE;
    }
    arr->size -= 1;
    if (arr->reservedSize - arr->size >= EXPEND_SIZE) {
        void *newBlock = calloc(arr->size, itemSize);
        if (newBlock == NULL) {
            return HW_FALSE;
        }
        if (memcpy_s(newBlock, arr->size * itemSize, arr->items, arr->size * itemSize) != EOK) {
            DEVICE_RULE_ERROR("[pop]memcpy_s failed\n");
            MemFree(&(newBlock));
            return HW_FALSE;
        }
        MemFree(&(arr->items));

        arr->reservedSize = arr->size;
        arr->items = newBlock;
    }
    return HW_TRUE;
}