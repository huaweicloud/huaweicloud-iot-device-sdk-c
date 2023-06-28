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

#include "cJSON.h"
#include "json_util.h"

JSON *JSON_CreateObject()
{
    return (JSON *)cJSON_CreateObject();
}

JSON *JSON_CreateArray()
{
    return (JSON *)cJSON_CreateArray();
}

JSON *JSON_CreateIntArray(const int *numbers, int count)
{
    return (JSON *)cJSON_CreateIntArray(numbers, count);
}

JSON *JSON_CreateFloatArray(const float *numbers, int count)
{
    return (JSON *)cJSON_CreateFloatArray(numbers, count);
}

JSON *JSON_CreateDoubleArray(const double *numbers, int count)
{
    return (JSON *)cJSON_CreateDoubleArray(numbers, count);
}

JSON *JSON_CreateStringArray(const char **strings, int count)
{
    return (JSON *)cJSON_CreateStringArray(strings, count);
}

void JSON_AddStringToObject(JSON *object, const char *key, const char *value)
{
    cJSON *item = cJSON_CreateString(value);
    cJSON_AddItemToObject((cJSON *)object, key, item);
}

void JSON_AddNumberToObject(JSON *object, const char *key, double value)
{
    cJSON *item = cJSON_CreateNumber(value);
    cJSON_AddItemToObject((cJSON *)object, key, item);
}

void JSON_AddBoolToObject(JSON *object, const char *key, JSON_BOOL value)
{
    cJSON *item = cJSON_CreateBool(value);
    cJSON_AddItemToObject((cJSON *)object, key, item);
}

void JSON_AddObjectToObject(JSON *object, const char *key, JSON *value)
{
    cJSON_AddItemToObject((cJSON *)object, key, (cJSON *)value);
}

void JSON_AddObjectToArray(JSON *arrayObject, JSON *value)
{
    cJSON_AddItemToArray((cJSON *)arrayObject, (cJSON *)value);
}

char *JSON_Print(const JSON *object)
{
    return cJSON_Print((const cJSON *)object);
}

JSON *JSON_Parse(const char *value)
{
    return (JSON *)cJSON_Parse(value);
}

void JSON_Delete(JSON *object)
{
    cJSON_Delete((cJSON *)object);
}

int JSON_GetIntFromObject(const JSON *object, const char *key, const int defaultValue)
{
    JSON *t_object = (JSON *)cJSON_GetObjectItem((const cJSON *)object, key);
    return t_object ? t_object->valueint : defaultValue;
}

JSON_BOOL JSON_GetBoolFromObject(const JSON *object, const char *key, const JSON_BOOL defaultValue)
{
    JSON *t_object = (JSON *)cJSON_GetObjectItem((const cJSON *)object, key);
    return t_object ? t_object->valueint : defaultValue;
}

double JSON_GetDoubleFromObject(const JSON *object, const char *key, const double defaultValue)
{
    JSON *t_object = (JSON *)cJSON_GetObjectItem((const cJSON *)object, key);
    return t_object ? t_object->valuedouble : defaultValue;
}

char *JSON_GetStringFromObject(const JSON *object, const char *key, const char *defaultValue)
{
    JSON *t_object = (JSON *)cJSON_GetObjectItem((const cJSON *)object, key);
    return t_object ? t_object->valuestring : (char *)defaultValue;
}

JSON *JSON_GetObjectFromObject(const JSON *object, const char *key)
{
    return (JSON *)cJSON_GetObjectItem((const cJSON *)object, key);
}

int JSON_GetArraySize(const JSON *array)
{
    if (array == NULL) {
        return 0;
    }
    return cJSON_GetArraySize((const cJSON *)array);
}

int JSON_GetIntFromArray(const JSON *array, int index, const int defaultValue)
{
    JSON *t_object = (JSON *)cJSON_GetArrayItem((const cJSON *)array, index);
    return t_object ? t_object->valueint : defaultValue;
}

JSON_BOOL JSON_GetBoolFromArray(const JSON *array, int index, const JSON_BOOL defaultValue)
{
    JSON *t_object = (JSON *)cJSON_GetArrayItem((const cJSON *)array, index);
    return t_object ? t_object->valueint : defaultValue;
}

double JSON_GetDoubleFromArray(const JSON *array, int index, const double defaultValue)
{
    JSON *t_object = (JSON *)cJSON_GetArrayItem((const cJSON *)array, index);
    return t_object ? t_object->valuedouble : defaultValue;
}

char *JSON_GetStringFromArray(const JSON *array, int index, const char *defaultValue)
{
    JSON *t_object = (JSON *)cJSON_GetArrayItem((const cJSON *)array, index);
    return t_object ? t_object->valuestring : (char *)defaultValue;
}

JSON *JSON_GetObjectFromArray(const JSON *array, int index)
{
    return (JSON *)cJSON_GetArrayItem((const cJSON *)array, index);
}
