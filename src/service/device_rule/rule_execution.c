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

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>

#include "string_util.h"
#include "securec.h"
#include "rule_util.h"
#include "rule_info.h"
#include "rule_execution.h"

void PropertyValueSetInt(PropertyValue *dest, int val)
{
    dest->type = PropertyValueDataTypeInt;
    dest->data.i = val;
}

void PropertyValueSetDouble(PropertyValue *dest, double val)
{
    dest->type = PropertyValueDataTypeDouble;
    dest->data.d = val;
}

void PropertyValueSetCStr(PropertyValue *dest, const char *val)
{
    dest->type = PropertyValueDataTypeCStr;
    dest->data.cstr = val;
}

/*
MembersName = [
    "i",
    "d",
    "cstr",
]
ValueDatatype = ["PropertyValueDataTypeInt",
"PropertyValueDataTypeDouble",
"PropertyValueDataTypeCStr"]
ComparisionType = ["<",
"==",
">",
"<=",
">="]
*/
HW_BOOL PropertyValueCompare(int *result, const PropertyValue *left, const PropertyValue *right)
{
    if (left->type != right->type) {
        DEVICE_RULE_ERROR("can't compare between different type!\n");
        return HW_FALSE;
    }
    HW_BOOL isBigger = HW_FALSE;
    HW_BOOL isSmaller = HW_FALSE;
    switch (left->type) {
        case PropertyValueDataTypeInt:
            isSmaller = (left->data.i < right->data.i);
            isBigger =  (left->data.i > right->data.i);
            break;

        case PropertyValueDataTypeDouble:
            isSmaller = (left->data.d < right->data.d);
            isBigger = (left->data.d  > right->data.d);
            break;

        case PropertyValueDataTypeCStr:
            *result = strcmp(left->data.cstr, right->data.cstr);
            return HW_TRUE;

        default:
            DEVICE_RULE_ERROR("unknown property value type %d", left->type);
            return HW_FALSE;
    }

    if (isSmaller) {
        *result = -1;
    } else if (isBigger) {
        *result = 1;
    } else {
        *result = 0;
    }
    return HW_TRUE;
}

static HW_BOOL CalculateDailyTimerCondition(HW_BOOL *result, DailyTimerCondition *condition, time_t epochTime)
{
    time_t timestamp = epochTime;
    struct tm time;
    gmtime_r(&epochTime, &time);
    *result = HW_FALSE;

    // meaning of daysOfWeek
    // bit 7    6    5    4    3   2   1    0
    // bit Sat  Fri  Thu  Wen Tu   Mon Sun  x
    // but tm_wday in struct  rm starts from 0(Sun)->6(Sat),so add 1 here
    if (condition->daysOfWeek & (1 << (time.tm_wday + 1))) {
        int sec = HHMMSStoSecond(time.tm_hour, time.tm_min, time.tm_sec);
        if ((condition->time <= sec) && (sec < condition->time + 60)) {
            // make sure it doesn't stumble into 1 miniute range of  previous executed action.
            if (timestamp - condition->lastExecutedTime[time.tm_wday] >= 60) {
                *result = HW_TRUE;
            }
        }
    }
    return HW_TRUE;
}

static void DailyTimerConditionMarkDayIsExecuted(DailyTimerCondition *condition, time_t epochTime)
{
    time_t timestamp = epochTime;
    struct tm time;
    gmtime_r(&epochTime, &time);

    // meaning of daysOfWeek
    // bit 7    6    5    4    3   2   1    0
    // bit Sat  Fri  Thu  Wen Tu   Mon Sun  x
    // but tm_wday in struct  rm starts from 0(Sun)->6(Sat), so add 1 here
    if (condition->daysOfWeek & (1 << (time.tm_wday + 1))) {
        int sec = HHMMSStoSecond(time.tm_hour, time.tm_min, time.tm_sec);
        if ((sec >= condition->time) && (sec < condition->time + 60)) {
            condition->lastExecutedTime[time.tm_wday] = timestamp;
            return;
        }
    }
}

static void CalculateSimpleTimerCondition(HW_BOOL *result, SimpleTimerCondition *condition, const time_t epochTime)
{
    time_t timestamp = epochTime;

    *result = HW_FALSE;

    // meaning of daysOfWeek
    // bit 7    6    5    4    3   2   1    0
    // bit Sat  Fri  Thu  Wen Tu   Mon Sun  x
    // but tm_wday in struct  rm starts from 0(Sun)->6(Sat),so add 1 here
    if (timestamp < condition->startTime) {
        return; // this is just to tell this function execute successfully! not the reuslt of evaluation is HW_TRUE!
    }
    int diff = (int)difftime(timestamp, condition->startTime);
    if (diff % condition->repeatInterval != 0) {
        return; // this is just to tell this function execute successfully! not the reuslt of evaluation is HW_TRUE!
    }
    int count = diff / condition->repeatInterval;
    if (count >= condition->repeatCount) {
        return; // this is just to tell this function execute successfully! not the reuslt of evaluation is HW_TRUE!
    }
    *result = HW_TRUE;
}

static HW_BOOL GetPropertyValueByPath(const char *path, PropertyValue *valRead, const GetPropertyValueCallBack callback)
{
    size_t pathStrLen = strlen(path);
    size_t serviceStrLen = strcspn(path, "/");
    if (pathStrLen == serviceStrLen) {
        DEVICE_RULE_ERROR("invalid deviceId.path format, expect a slash in path");
        return HW_FALSE;
    }

    // get string of the service and of the property, then read property through the callback
    char *service = (char *)malloc(serviceStrLen + 1);
    if (service == NULL) {
        DEVICE_RULE_ERROR("malloc failed\n");
        return HW_FALSE;
    }
    const char *property = path + (serviceStrLen + 1);
    if (strncpy_s(service, serviceStrLen + 1, path, serviceStrLen) != EOK) {
        DEVICE_RULE_ERROR("strncpy_s failed\n");
        return HW_FALSE;
    }
    service[serviceStrLen] = 0;
    DEVICE_RULE_DEBUG("get service: %s, property: %s", service, property);

    if (!callback(service, property, valRead)) {
        DEVICE_RULE_ERROR("can't read property");
        MemFree(&(service));
        return HW_FALSE;
    }
    
    MemFree(&(service));
    return HW_TRUE;
}

static HW_BOOL StrToPropertyValue(const char *value, PropertyValueDataType type, PropertyValue *valExpected)
{
    // construct the coressponding compared property
    const char *numberStr = value;
    const char *AfterEnd;
    double number;
    switch (type) {
        case PropertyValueDataTypeInt:
            number = strtod(numberStr, (char **)&AfterEnd);
            if (numberStr == AfterEnd) {
                DEVICE_RULE_ERROR("parse %s to int failed", numberStr);
                return HW_FALSE;
            }
            if (number >= INT_MAX) {
                valExpected->data.i = INT_MAX;
            } else if (number <= (double)INT_MIN) {
                valExpected->data.i = INT_MIN;
            } else {
                valExpected->data.i = (int)number;
            }
            break;

        case PropertyValueDataTypeDouble:
            number = strtod(numberStr, (char **)&AfterEnd);
            if (numberStr == AfterEnd) {
                DEVICE_RULE_ERROR("parse %s to double failed\n", numberStr);
                return HW_FALSE;
            }
            valExpected->data.d = number;
            break;

        case PropertyValueDataTypeCStr:
            valExpected->data.cstr = value;
            break;

        default:
            DEVICE_RULE_ERROR("unknow property value type: %d", valExpected->type);
            return HW_FALSE;
    }
    valExpected->type = type;
    return HW_TRUE;
}

static HW_BOOL CalculateDeviceDataRangeCodition(HW_BOOL *result, const DeviceDataCondition *condition,
    const GetPropertyValueCallBack callback)
{
    PropertyValue readProperty;
    if (!GetPropertyValueByPath(condition->deviceInfo.path, &readProperty, callback)) {
        return HW_FALSE;
    }
    const char *rangeStr = condition->value;
    const char *numberEnd;
    // Don't worry, double have precision and accuracy when storing big number,
    // but it can accurately represent 32bit of intger.
    // https://stackoverflow.com/questions/13269523/can-all-32-bit-ints-be-exactly-represented-as-a-double
    
    double valueLeftBoundry = strtod(rangeStr, (char **)&numberEnd);
    if ((valueLeftBoundry == HUGE_VAL) || (numberEnd - rangeStr  == 0)) {
        DEVICE_RULE_ERROR("invalid left boundry value");
    }
    if (*numberEnd == '\0') {
        DEVICE_RULE_ERROR("right boundry is missing");
        return HW_FALSE;
    }

    rangeStr = numberEnd + 1;
    double valueRightBoundry = strtod(rangeStr, (char **)&numberEnd);
    if ((valueRightBoundry == HUGE_VAL) || (numberEnd - rangeStr  == 0)) {
        DEVICE_RULE_ERROR("invalid right boundry value");
    }

    double valueRead;
    switch (readProperty.type) {
        case PropertyValueDataTypeInt: {
            valueRead = readProperty.data.i;
            break;
        }
        case PropertyValueDataTypeDouble: {
            valueRead = readProperty.data.d;
            break;
        }
        case PropertyValueDataTypeCStr: {
            DEVICE_RULE_ERROR("can't compare string with range");
            return HW_FALSE;
        }
        default:
            DEVICE_RULE_ERROR("can't compare with unknown type");
            return HW_FALSE;
    }

    *result = ((valueRead >= valueLeftBoundry) && (valueRead <= valueRightBoundry));
    return HW_TRUE;
}

static HW_BOOL CalculateDeviceDataCompareCodition(HW_BOOL *result, const DeviceDataCondition *condition,
    const GetPropertyValueCallBack callback)
{
    PropertyValue valRead;
    PropertyValue valExpected;
    if (!GetPropertyValueByPath(condition->deviceInfo.path, &valRead, callback)) {
        return HW_FALSE;
    }
    if (!StrToPropertyValue(condition->value, valRead.type, &valExpected)) {
        return HW_FALSE;
    }
    
    // compare and get the result
    int comparisonResult = 0;
    if (!PropertyValueCompare(&comparisonResult, &valRead, &valExpected)) {
        return HW_FALSE;
    }

    if (strcmp(condition->operator, "<") == 0) {
        *result = comparisonResult < 0;
    } else if ((strcmp(condition->operator, "=") == 0) || (strcmp(condition->operator, "==") == 0)) {
        *result = comparisonResult == 0;
    } else if (strcmp(condition->operator, ">") == 0) {
        *result = comparisonResult > 0;
    } else if (strcmp(condition->operator, "<=") == 0) {
        *result = comparisonResult <= 0;
    } else if (strcmp(condition->operator, ">=") == 0) {
        *result = comparisonResult >= 0;
    } else {
        return HW_FALSE;
    }
    return HW_TRUE;
}

static HW_BOOL CalculateDeviceDataCondition(HW_BOOL *result, const DeviceDataCondition *condition,
    const GetPropertyValueCallBack callback)
{
    if (strcmp(condition->operator, "between") == 0) {
        return CalculateDeviceDataRangeCodition(result, condition, callback);
    } else {
        return CalculateDeviceDataCompareCodition(result, condition, callback);
    }
}

static HW_BOOL CalculateConditions(HW_BOOL *result, ConditionList *list, const char *logic,
    const GetPropertyValueCallBack callback, const time_t epochTime)
{
    // or or and
    HW_BOOL isLogicAnd = (strcmp(logic, "and") == 0);
    // for logic or, initial result sould be HW_TRUE; and in contrary, HW_FALSE for logic and.
    *result = isLogicAnd;

    Condition *conditionItem;
    DyListFor (conditionItem, list) {
        HW_BOOL currentResult;
        switch (conditionItem->type) {
            case ConditionTypeDeviceData: {
                if (!CalculateDeviceDataCondition(&currentResult, &conditionItem->data.deviceData, callback)) {
                    DEVICE_RULE_ERROR("CalculateDeviceDataCondition failed");
                    return HW_FALSE;
                }
                break;
            }
            case ConditionTypeSimpleTimer: {
                CalculateSimpleTimerCondition(&currentResult, &conditionItem->data.simpleTimer, epochTime);
                break;
            }
            case ConditionTypeDailyTimer: {
                if (!CalculateDailyTimerCondition(&currentResult, &conditionItem->data.dailyTimer, epochTime)) {
                    DEVICE_RULE_ERROR("CalculateDailyTimerCondition failed");
                    return HW_FALSE;
                }
                break;
            }
            default:
                DEVICE_RULE_WARN("unknown condition type, ignore \"%s\"", ConditionTypeToStr(conditionItem->type));
        }

        if (isLogicAnd) {
            *result = (*result) && currentResult;
        } else {
            *result = (*result) || currentResult;
        }
    }
    if (*result == HW_TRUE) {
        DyListFor (conditionItem, list) {
            switch (conditionItem->type) {
                case ConditionTypeDailyTimer: {
                    DailyTimerConditionMarkDayIsExecuted(&conditionItem->data.dailyTimer, epochTime);
                    break;
                }
                default:
                    break;
            }
        }
    }
    return HW_TRUE;
}

static void ExecuteActionList(const ActionList *list, const ExecuteCommandCallBack commandCallback)
{
    const Action *one;
    ConstDyListFor (one, list) {
        if (strcmp(one->status, "enable") != 0) {
            continue;
        }
        if (strcmp(one->type, "DEVICE_CMD") == 0) {
            commandCallback(one->deviceId, &one->command);
        } else {
            DEVICE_RULE_WARN("unknown action type \"%s\"", one->type);
        }
    }
}

static HW_BOOL RuleInfoHasTimer(const RuleInfo *self)
{
    const Condition *condition;
    ConstDyListFor (condition, &self->conditions) {
        switch (condition->type) {
            case ConditionTypeDailyTimer:
            case ConditionTypeSimpleTimer:
                return HW_TRUE;
            default:
                return HW_FALSE;
        }
    }
    return HW_FALSE;
}

static HW_BOOL IsTriggeredByTimer(HW_BOOL *result, RuleInfo *ruleInfo, time_t epochTime)
{
    Condition *conditionItem;
    *result = HW_FALSE;
    DyListFor (conditionItem, &ruleInfo->conditions) {
        switch (conditionItem->type) {
            case ConditionTypeSimpleTimer: {
                CalculateSimpleTimerCondition(result, &conditionItem->data.simpleTimer, epochTime);
                if (*result == HW_TRUE) {
                    return HW_TRUE;
                }
                break;
            }
            case ConditionTypeDailyTimer: {
                if (!CalculateDailyTimerCondition(result, &conditionItem->data.dailyTimer, epochTime)) {
                    DEVICE_RULE_ERROR("CalculateDailyTimerCondition failed");
                    return HW_FALSE;
                }
                if (*result == HW_TRUE) {
                    return HW_TRUE;
                }
                break;
            }
            default:
                break;
        }
    }
    return HW_TRUE;
}

void CheckRuleInfoListAndExecute(RuleInfoList *list, const GetPropertyValueCallBack propertyCallback,
    const ExecuteCommandCallBack commandCallback, HW_BOOL isTimerRule, time_t epochTime)
{
    RuleInfo *ruleItem;
    DyListFor (ruleItem, list) {
        if (strcmp(ruleItem->status, "active") != 0) {
            // DEVICE_RULE_DEBUG("rule %s: \"%s\" is inactive, skip", ruleItem->ruleName, ruleItem->ruleId);
            continue;
        }
        if (RuleInfoHasTimer(ruleItem) && !isTimerRule) {
            // DEVICE_RULE_DEBUG("require no-timer rule but %s is not, skip", ruleItem->ruleId);
            continue;
        }

        if (!RuleInfoHasTimer(ruleItem) && isTimerRule) {
           //  DEVICE_RULE_DEBUG("require timer rule but %s is not, skip", ruleItem->ruleId);
            continue;
        }
        if (!TimeRangeIsNuLL(&ruleItem->timeRange)) {
            struct tm time;
            gmtime_r(&epochTime, &time);
            // meaning of daysOfWeek
            // bit 7    6    5    4    3   2   1    0
            // bit Sat  Fri  Thu  Wen Tu   Mon Sun  x
            // but tm_wday in struct rm starts from 0(Sun)->6(Sat), so add 1 here

            int sec = HHMMSStoSecond(time.tm_hour, time.tm_min, time.tm_sec);
            if (ruleItem->timeRange.startTime < ruleItem->timeRange.endTime) {
                if ((ruleItem->timeRange.daysOfWeek & (1 << (time.tm_wday + 1))) == 0) {
                    DEVICE_RULE_DEBUG("rule isn't in this day of week, skip");
                    continue;
                }
                if ((sec < ruleItem->timeRange.startTime) || (sec > ruleItem->timeRange.endTime)) {
                    DEVICE_RULE_DEBUG("rule isn't in time range, skip");
                    continue;
                }
            } else {
                int today = time.tm_wday;
                int yesterdayInWeek  = ruleItem->timeRange.daysOfWeek >> 1;
                if (yesterdayInWeek & 1) {
                    yesterdayInWeek |= DaysOfWeekSaturday;
                }

                HW_BOOL secInToday = ((ruleItem->timeRange.daysOfWeek & (1 << (today + 1))) != 0 &&
                    sec <= ruleItem->timeRange.endTime);
                HW_BOOL secInYesterday = ((yesterdayInWeek & (1 << (today + 1))) != 0 &&
                    sec >= ruleItem->timeRange.startTime);
                if (!(secInToday || secInYesterday)) {
                    DEVICE_RULE_DEBUG("rule isn't in this day of week, skip");
                    continue;
                }
            }
        }

        HW_BOOL result;
        if (!IsTriggeredByTimer(&result, ruleItem, epochTime)) {
            DEVICE_RULE_DEBUG("IsTriggeredByTimer failed");
            continue;
        }

        if (!CalculateConditions(&result, &ruleItem->conditions, ruleItem->logic, propertyCallback, epochTime)) {
            DEVICE_RULE_WARN("CalculateConditions falied! skip.");
            continue;
        }

        if (result) {
            DEVICE_RULE_INFO("Check condition true, to execute actions.");
            ExecuteActionList(&ruleItem->actions, commandCallback);
        } else {
            DEVICE_RULE_DEBUG("Check condition false, action won't be executed.");
        }
    }
}
