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
#include "rule_parse.h"
#include "log_util.h"

#define INDENT_GROWTH 4

void PrintTime(time_t timestamp)
{
    struct tm *time = localtime(&timestamp);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "%s", buffer);
}

void PirntIndent(int n)
{
    int i;
    for (i = 0; i < n; ++i) {
        putchar(' ');
    }
}

void PrintDeviceInfo(const DeviceInfoType *deviceInfo, int indent)
{
    PirntIndent(indent);
    PrintfLog(EN_LOG_LEVEL_DEBUG, " deviceId = \"%s\", path = \"%s\"\n", deviceInfo->deviceId, deviceInfo->path);
}

void PrintConditionList(const ConditionList *list, int indent)
{
    const ConditionType *one;
    int i = 0;
    DyListFor (one, list) {
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "-----> %d: \n", i);
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "type: %s\n", ConditionDataTypeToStr(one->type));
        ++i;
        switch (one->type) {
            case ConditionTypeDeviceData:
                PirntIndent(indent);
                PrintfLog(EN_LOG_LEVEL_DEBUG, "operator: %s\n", one->data.deviceData.operator);
                PirntIndent(indent);
                PrintfLog(EN_LOG_LEVEL_DEBUG, "value: %s\n", one->data.deviceData.value);
                PirntIndent(indent);
                PrintfLog(EN_LOG_LEVEL_DEBUG, "deviceInfo:");
                PrintDeviceInfo(&one->data.deviceData.deviceInfo, indent + INDENT_GROWTH);
                break;
            case ConditionTypeSimpleTimer: {
                time_t timestamp = one->data.simpleTimer.startTime;
                struct tm *time = localtime(&timestamp);
                char buffer[32];
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time);
                PirntIndent(indent);
                PrintfLog(EN_LOG_LEVEL_DEBUG, "startTime: %s\n", buffer);
                PirntIndent(indent);
                PrintfLog(EN_LOG_LEVEL_DEBUG, "repeatInterval: %d\n", one->data.simpleTimer.repeatInterval);
                PirntIndent(indent);
                PrintfLog(EN_LOG_LEVEL_DEBUG, "repeatCount: %d\n", one->data.simpleTimer.repeatCount);
                break;
            }
            case ConditionTypeDailyTimer:
                PirntIndent(indent);
                PrintfLog(EN_LOG_LEVEL_DEBUG, "time: %s", one->data.dailyTimer.time);
                PirntIndent(indent);
                PrintfLog(EN_LOG_LEVEL_DEBUG, "daysOfWeek: %s\n", one->data.dailyTimer.daysOfWeek);
                break;
            default:
                PirntIndent(indent);
                PrintfLog(EN_LOG_LEVEL_DEBUG, "unknown condition type\n");
        }
    }
}

void PrintActionList(const ActionList *list, int indent)
{
    const Action *one;
    DyListFor (one, list) {
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "type = %s\n", one->type);
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "status = %s\n", one->status);
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "deviceId = %s\n", one->deviceId);
    }
}

void PrintRuleInfoList(const RuleInfoListType *list, int indent)
{
    const RuleInfoType *oneRule;
    DyListFor (oneRule, list) {
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "ruleId: %s\n", oneRule->ruleId);
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "ruleName: %s\n", oneRule->ruleName);
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "logic: %s\n", oneRule->logic);
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "timeRange:\n");
        if (TimeRangeIsNuLL(&oneRule->timeRange)) {
            PirntIndent(indent + INDENT_GROWTH);
            PrintfLog(EN_LOG_LEVEL_DEBUG, "(NULL)\n");
        } else {
            PirntIndent(indent + INDENT_GROWTH);
            PrintfLog(EN_LOG_LEVEL_DEBUG, "startTime: ");
            PrintTime(oneRule->timeRange.startTime);
            PrintfLog(EN_LOG_LEVEL_DEBUG, "\n");
            PirntIndent(indent + INDENT_GROWTH);
            PrintfLog(EN_LOG_LEVEL_DEBUG, "endTime: ");
            PrintTime(oneRule->timeRange.endTime);
            PrintfLog(EN_LOG_LEVEL_DEBUG, "\n");
            PirntIndent(indent + INDENT_GROWTH);
            PrintfLog(EN_LOG_LEVEL_DEBUG, "daysOfWeek: %d\n", oneRule->timeRange.daysOfWeek);
        }
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "status: %s\n", oneRule->status);
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "vesrion: %d\n", oneRule->ruleVersionInShadow);
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "conditions:\n");
        PrintCondtionList(&oneRule->conditions, indent + INDENT_GROWTH);
        PirntIndent(indent);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "actions:\n");
        PrintActionList(&oneRule->actions, indent + INDENT_GROWTH);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "\n");
    }
}

void PrintRuleIdVersion(RuleInfoType *delList, int deCount)
{
    int i;
    for (i = 0; i < deCount; ++i) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "ruleId = \"%s\", vserion = %d\n", delList[i].ruleId, delList[i].ruleVersionInShadow);
    }
}

void PrintRuleInfoListIdVersion(const RuleInfoListType *list)
{
    PrintRuleIdVersion(list->elements, list->size);
}
