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

#ifndef RULE_EXECUTE_H
#define RULE_EXECUTE_H

#include <time.h>
#include "hw_type.h"
#include "rule_info.h"

typedef enum {
    PropertyValueDataTypeInt,
    PropertyValueDataTypeDouble,
    PropertyValueDataTypeFloat,
    PropertyValueDataTypeCStr,
} PropertyValueDataType;

typedef struct {
    PropertyValueDataType type;
    union {
        int i;
        double d;
        const char *cstr;
    } data;
} PropertyValue;

typedef HW_BOOL (*GetPropertyValueCallBack)(const char *serviceId, const char *property, PropertyValue *value);
typedef void (*ExecuteCommandCallBack)(const char *deviceId, const Command *command);
void PropertyValueSetInt(PropertyValue *dest, int val);
void PropertyValueSetDouble(PropertyValue *dest, double val);
void PropertyValueSetCStr(PropertyValue *dest, const char *val);
HW_BOOL PropertyValueCompare(int *result, const PropertyValue *left, const PropertyValue *right);
void CheckRuleInfoListAndExecute(RuleInfoList *list, const GetPropertyValueCallBack propertyCallback,
    const ExecuteCommandCallBack commandCalback, HW_BOOL isTimerRule, time_t epochTime);

#endif