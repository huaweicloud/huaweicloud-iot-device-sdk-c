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

#ifndef RULE_UTIL
#define RULE_UTIL

#include <stdint.h>
#include "hw_type.h"
#include "cJSON.h"
#include "log_util.h"

#define CHECK_NAN_RETURN_ERR(object, errorCode)   \
    do {                                          \
        if ((object) == NAN) {                    \
            printf("err: %s is NAN\n", #object);  \
            return errorCode;                     \
        }                                         \
    } while (0)

#define CHECK_NULL_RETURN_ERR(object, errorCode)  \
    do {                                          \
        if ((object) == NULL) {                   \
            printf("err: %s is NULL\n", #object); \
            return errorCode;                     \
        }                                         \
    } while (0)

#define CHECK_NULL_RETURN_VOID(object)            \
    do {                                          \
        if ((object) == NULL) {                   \
            printf("err: %s is NULL\n", #object); \
            return;                               \
        }                                         \
    } while (0)

#define ERRVAL (-1)

#define CHECK_ERRVAL_RETURN_ERRVAL(object)        \
    do {                                          \
        if ((object) == ERRVAL) {                 \
            printf("err: %s failed\n", #object);  \
            return ERRVAL;                        \
        }                                         \
    } while (0)

#define CHECK_NULL_RETURN_ERRVAL(object)          \
    do {                                          \
        if ((object) == NULL) {                   \
            printf("err: %s is None\n", #object); \
            return ERRVAL;                        \
        }                                         \
    } while (0)

#define CHECK_FALSE_RETURN_ERRVAL(object)         \
    do {                                          \
        if (!(object)) {                          \
            printf("err: %s failed\n", #object);  \
            return ERRVAL;                        \
        }                                         \
    } while (0)


#define DEVICE_RULE_DEBUG(FMT, ...) PrintfLog(EN_LOG_LEVEL_DEBUG, FMT "\n", ##__VA_ARGS__)
#define DEVICE_RULE_INFO(FMT, ...) PrintfLog(EN_LOG_LEVEL_INFO, FMT "\n", ##__VA_ARGS__)
#define DEVICE_RULE_WARN(FMT, ...) PrintfLog(EN_LOG_LEVEL_WARNING, FMT "\n", ##__VA_ARGS__)
#define DEVICE_RULE_ERROR(FMT, ...) PrintfLog(EN_LOG_LEVEL_ERROR, FMT "\n", ##__VA_ARGS__)


HW_BOOL GetIntValueFromJson(int *value, const cJSON *item);
HW_BOOL CStrDuplicate(char **self, const char *s);
HW_BOOL CStrDuplicateNotNULL(char **self, const char *s);

#define SIZE_OF_UNDECAYED_ARRAY(arr) (sizeof(arr) / sizeof(arr[0]))

#define DECLARE_DYARRAY(TYPE_OR_OBJ_NAME, ITEM_TYPE, ITEM_NAME) \
    struct {                                                    \
        size_t size;                                            \
        size_t reservedSize;                                    \
        ITEM_TYPE *ITEM_NAME;                                   \
    } TYPE_OR_OBJ_NAME

typedef DECLARE_DYARRAY(DynamicArray, void, items);
typedef void (*DyArrayItemDtorType)(void * /* item */);
typedef HW_BOOL (*DyArrayItemCtorType)(void * /* item */);

HW_BOOL DynamicArrayCtor(DynamicArray *arr, size_t itemSize);
void DynamicArrayDtor(DynamicArray *arr, size_t itemSize, DyArrayItemDtorType funcItemDtor);
void *DynamicArrayPush(DynamicArray *arr, DyArrayItemCtorType funcItemCtor, size_t itemSize);
void *DynamicArrayGetItem(DynamicArray *arr, size_t i, size_t itemSize);
void DynamicArrayRemoveItem(DynamicArray *arr, void *element, size_t itemSize, DyArrayItemDtorType funcItemDtor);
HW_BOOL DynamicEmpty(const DynamicArray *arr);
HW_BOOL DynamicArrayPop(DynamicArray *arr, size_t itemSize, DyArrayItemDtorType funcItemDtor);
HW_BOOL DynamicArrayPopNoDtor(DynamicArray *arr, size_t itemSize);

#define DECL_DYARRY_FUNC_UTIL(TYPE_NAME_PREFIX, ITEM_TYPE, ITEM_NAME)          \
    typedef DECLARE_DYARRAY(TYPE_NAME_PREFIX, ITEM_TYPE, ITEM_NAME);           \
    HW_BOOL TYPE_NAME_PREFIX##Ctor(TYPE_NAME_PREFIX *arr);                     \
    void TYPE_NAME_PREFIX##Dtor(TYPE_NAME_PREFIX *arr);                        \
    ITEM_TYPE *TYPE_NAME_PREFIX##Push(TYPE_NAME_PREFIX *arr);                  \
    ITEM_TYPE *TYPE_NAME_PREFIX##GetItem(TYPE_NAME_PREFIX *arr, int i);        \
    void TYPE_NAME_PREFIX##RemoveItem(TYPE_NAME_PREFIX *arr, ITEM_TYPE *item); \
    HW_BOOL TYPE_NAME_PREFIX##Empty(const TYPE_NAME_PREFIX *arr);              \
    HW_BOOL TYPE_NAME_PREFIX##Pop(TYPE_NAME_PREFIX *arr);                      \
    HW_BOOL TYPE_NAME_PREFIX##PopNoDtor(TYPE_NAME_PREFIX *arr);

#define DECL_DYARRY_FUNC_UTIL_IMPL(TYPE_NAME_PREFIX, ITEM_TYPE, ITEM_NAME, ITEM_CTOR, ITEM_DTOR)               \
    HW_BOOL TYPE_NAME_PREFIX##Ctor(TYPE_NAME_PREFIX *arr)                                                      \
    {                                                                                                          \
        return DynamicArrayCtor((DynamicArray *)arr, sizeof(ITEM_TYPE));                                       \
    }                                                                                                          \
    void TYPE_NAME_PREFIX##Dtor(TYPE_NAME_PREFIX *arr)                                                         \
    {                                                                                                          \
        DynamicArrayDtor((DynamicArray *)arr, sizeof(ITEM_TYPE), (DyArrayItemDtorType)(ITEM_DTOR));            \
    }                                                                                                          \
    ITEM_TYPE *TYPE_NAME_PREFIX##Push(TYPE_NAME_PREFIX *arr)                                                   \
    {                                                                                                          \
        return DynamicArrayPush((DynamicArray *)arr, (DyArrayItemCtorType)(ITEM_CTOR), sizeof(ITEM_TYPE));     \
    }                                                                                                          \
    HW_BOOL TYPE_NAME_PREFIX##Empty(const TYPE_NAME_PREFIX *arr)                                               \
    {                                                                                                          \
        return DynamicEmpty((const DynamicArray *)arr);                                                        \
    }                                                                                                          \
    ITEM_TYPE *TYPE_NAME_PREFIX##GetItem(TYPE_NAME_PREFIX *arr, int i)                                         \
    {                                                                                                          \
        return DynamicArrayGetItem((DynamicArray *)arr, i, sizeof(ITEM_TYPE));                                 \
    }                                                                                                          \
    void TYPE_NAME_PREFIX##RemoveItem(TYPE_NAME_PREFIX *arr, ITEM_TYPE *item)                                  \
    {                                                                                                          \
        DynamicArrayRemoveItem((DynamicArray *)arr, item, sizeof(ITEM_TYPE),                                   \
            (DyArrayItemDtorType)(ITEM_DTOR));                                                                 \
    }                                                                                                          \
    HW_BOOL TYPE_NAME_PREFIX##Pop(TYPE_NAME_PREFIX *arr)                                                       \
    {                                                                                                          \
        return DynamicArrayPop((DynamicArray *)arr, sizeof(ITEM_TYPE), (DyArrayItemDtorType)(ITEM_DTOR));      \
    }                                                                                                          \
    HW_BOOL TYPE_NAME_PREFIX##PopNoDtor(TYPE_NAME_PREFIX *arr)                                                 \
    {                                                                                                          \
        return DynamicArrayPopNoDtor((DynamicArray *)arr, sizeof(ITEM_TYPE));                                  \
    }

void DynamicItemDummyDtor(void *item);
HW_BOOL DynamicItemDummyCtor(void *item);

#define DyListFor(item, ptrList)                      \
    for ((item) = ((DynamicArray *)(ptrList))->items; \
        (uintptr_t)(item) != \
        (uintptr_t)((DynamicArray *)(ptrList))->items + sizeof(*(item)) * ((ptrList)->size);++(item))

#define ConstDyListFor(item, ptrList)                       \
    for ((item) = ((const DynamicArray *)(ptrList))->items; \
        (uintptr_t)(item) != \
        (uintptr_t)((const DynamicArray *)(ptrList))->items + sizeof(*(item)) * ((ptrList)->size); ++(item))

#endif