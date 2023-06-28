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

#ifndef RULE_CONDITION_H
#define RULE_CONDITION_H

#include <time.h>
#include "hw_type.h"

typedef struct {
    char *deviceId;
    char *path;
} DeviceInfo;

HW_BOOL DeviceInfoCtor(DeviceInfo *self);
void DeviceInfoDtor(DeviceInfo *self);

typedef unsigned char DaysOfWeekRecord;
#define DAYS_PER_WEEK 7

typedef struct {
    int time;
    DaysOfWeekRecord daysOfWeek;
    time_t lastExecutedTime[DAYS_PER_WEEK];
} DailyTimerCondition;

HW_BOOL DailyTimerConditionCtor(DailyTimerCondition *self);
void DailyTimerConditionDtor(DailyTimerCondition *self);

typedef struct {
    char *operator;
    char *value;
    DeviceInfo deviceInfo;
} DeviceDataCondition;

HW_BOOL DeviceDataConditionCtor(DeviceDataCondition *self);
void DeviceDataConditionDtor(DeviceDataCondition *self);

typedef struct {
    time_t startTime; // GMT/UTC+0
    int repeatInterval;
    int repeatCount;
} SimpleTimerCondition;

HW_BOOL SimpleTimerConditionCtor(SimpleTimerCondition *self);
void SimpleTimerConditionDtor(SimpleTimerCondition *self);

typedef enum {
    ConditionTypeEmpty,
    ConditionTypeDailyTimer,
    ConditionTypeDeviceData,
    ConditionTypeSimpleTimer,
} ConditionType;

typedef struct {
    ConditionType type;
    union {
        DailyTimerCondition dailyTimer;
        DeviceDataCondition deviceData;
        SimpleTimerCondition simpleTimer;
    } data;
} Condition;

HW_BOOL ConditionCtor(Condition *self);
void ConditionDtor(Condition *self);
ConditionType ConditionTypeFromStr(const char *str);
const char *ConditionTypeToStr(ConditionType conditionType);

// util
#define DaysOfWeekSunday (1 << 1)
#define DaysOfWeekMonday (1 << 2)
#define DaysOfWeekTuesday (1 << 3)
#define DaysOfWeekWednesday (1 << 4)
#define DaysOfWeekThursday (1 << 5)
#define DaysOfWeekFirday (1 << 6)
#define DaysOfWeekSaturday (1 << 7)

int HHMMSStoSecond(int HH, int MM, int SS);
HW_BOOL SaveDaysOfWeek(DaysOfWeekRecord *daysOfWeek, const char *s);
// convert HH:mm:ss or HH:mm format to second in day
HW_BOOL ConvertTimeStrToSeconds(int *secondInDay, const char *s);
HW_BOOL GMTTimestampFromStr(time_t *time, const char *s);

DECL_DYARRY_FUNC_UTIL(ConditionList, Condition, elements)

#endif