/*
 * Copyright (c) 2022-2022 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#include <string.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "rule_util.h"
#include "rule_condition.h"

HW_BOOL DeviceInfoCtor(DeviceInfo *self)
{
    (void)memset_s(self, sizeof(*self), 0, sizeof(*self));
    return HW_TRUE;
}

void DeviceInfoDtor(DeviceInfo *self)
{
    MemFree(&self->deviceId);
    MemFree(&self->path);
}

HW_BOOL DailyTimerConditionCtor(DailyTimerCondition *self)
{
    (void)memset_s(self, sizeof(*self), 0, sizeof(*self));
    return HW_TRUE;
}

void DailyTimerConditionDtor(DailyTimerCondition *self) {}

HW_BOOL DeviceDataConditionCtor(DeviceDataCondition *self)
{
    (void)memset_s(self, sizeof(*self), 0, sizeof(*self));
    return DeviceInfoCtor(&self->deviceInfo);
}

void DeviceDataConditionDtor(DeviceDataCondition *self)
{
    MemFree(&self->operator);
    MemFree(&self->value);
    DeviceInfoDtor(&self->deviceInfo);
}

HW_BOOL SimpleTimerConditionCtor(SimpleTimerCondition *self)
{
    (void)memset_s(self, sizeof(*self), 0, sizeof(*self));
    return HW_TRUE;
}

void SimpleTimerConditionDtor(SimpleTimerCondition *self) {}

HW_BOOL ConditionCtor(Condition *self)
{
    (void)memset_s(self, sizeof(*self), 0, sizeof(*self));
    self->type = ConditionTypeEmpty;
    return HW_TRUE;
}

void ConditionDtor(Condition *self)
{
    switch (self->type) {
        case ConditionTypeEmpty:
            break;
        case ConditionTypeDailyTimer:
            DailyTimerConditionDtor(&self->data.dailyTimer);
            break;
        case ConditionTypeDeviceData:
            DeviceDataConditionDtor(&self->data.deviceData);
            break;
        case ConditionTypeSimpleTimer:
            SimpleTimerConditionDtor(&self->data.simpleTimer);
            break;
        default:
            DEVICE_RULE_ERROR("unknown condition type!");
            break;
    }
}

ConditionType ConditionTypeFromStr(const char *type)
{
    if (strcmp(type, "DEVICE_DATA") == 0) {
        return ConditionTypeDeviceData;
    } else if (strcmp(type, "SIMPLE_TIMER") == 0) {
        return ConditionTypeSimpleTimer;
    } else if (strcmp(type, "DAILY_TIMER") == 0) {
        return ConditionTypeDailyTimer;
    } else {
        return ConditionTypeEmpty;
    }
}

const char *ConditionTypeToStr(ConditionType conditionType)
{
    switch (conditionType) {
        case ConditionTypeEmpty:
            return "EMPTY";
        case ConditionTypeDailyTimer:
            return "DAILY_TIMER";
        case ConditionTypeDeviceData:
            return "DEVICE_DATA";
        case ConditionTypeSimpleTimer:
            return "SIMPLE_TIMER";
        default:
            return "UNKNOWN";
    }
}

// 8bit is enough for storing
HW_BOOL SaveDaysOfWeek(DaysOfWeekRecord *daysOfWeek, const char *s)
{
    if (s == NULL) {
        return HW_FALSE;
    }
    size_t totalLength = strlen(s);
    const char *startOfNumber = s;
    *daysOfWeek = 0;

    while (startOfNumber < s + totalLength) {
        startOfNumber += strspn(startOfNumber, " ,");
        DaysOfWeekRecord day = (DaysOfWeekRecord)(*startOfNumber);
        day -= (DaysOfWeekRecord)('0');
        if (day >= 1 && day <= 7) {
            *daysOfWeek |= (1 << day);
        } else {
            return HW_FALSE;
        }
        startOfNumber += strspn(startOfNumber, "0123456789");
    }
    return HW_TRUE;
}

#define MIN_PER_HOUR 60
#define SEC_PER_MIN 60
int HHMMSStoSecond(int hour, int minute, int second)
{
    return (hour * MIN_PER_HOUR + minute) * SEC_PER_MIN + second;
}

// convert str to seconds from 00:00:00
HW_BOOL ConvertTimeStrToSeconds(int *secondInDay, const char *s)
{
    if (s == NULL) {
        return HW_FALSE;
    }
    struct tm time;
    if (strptime(s, "%H:%M:%S", &time) != NULL) {
        *secondInDay = HHMMSStoSecond(time.tm_hour, time.tm_min, time.tm_sec);
        return HW_TRUE;
    } else if (strptime(s, "%H:%M", &time) != NULL) {
        *secondInDay = HHMMSStoSecond(time.tm_hour, time.tm_min, 0);
        return HW_TRUE;
    }
    return HW_FALSE;
}

HW_BOOL GMTTimestampFromStr(time_t *time, const char *s)
{
    struct tm timeStruct;
    if (s == NULL) {
        return HW_FALSE;
    }
    if (strptime(s, "%Y-%m-%d %H:%M:%S", &timeStruct) == NULL) {
        return HW_FALSE;
    }
    *time = timegm(&timeStruct);
    return HW_TRUE;
}

DECL_DYARRY_FUNC_UTIL_IMPL(ConditionList, Condition, elements, ConditionCtor, ConditionDtor)
