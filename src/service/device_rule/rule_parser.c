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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "log_util.h"
#include "cJSON.h"
#include "rule_util.h"
#include "rule_info.h"
#include "rule_condition.h"
#include "rule_parser.h"

#define IS_ERRVAL(VAL) ((VAL) == ERRVAL)
#define IS_RULE_INFO_VALID(ptrRuleInfo) ((ptrRuleInfo)->ruleVersionInShadow == -1)


static int ParseCmdAction(Action *action, cJSON *actionCJSON);
static int ParsePropCondition(DeviceDataCondition *condition, cJSON *contentInJson);
static int ParseSimpleTimerCondition(SimpleTimerCondition *condition, cJSON *contentInJson);
static int ParseDailyTimerCondition(DailyTimerCondition *condition, cJSON *contentInJson);
static int ParseSingleCondition(Condition *condition, cJSON *contentInJson);
static int ParseSingleRule(RuleInfo *ruleinfo, cJSON *rule);

static RuleInfo *RuleListCheckExistence(RuleInfoList *list, const char *ruleId)
{
    RuleInfo *pOneInfo;
    DyListFor (pOneInfo, list) {
        if (strcmp(pOneInfo->ruleId, ruleId) == 0) {
            return pOneInfo;
        }
    }

    return NULL;
}

void ParseDeviceRule(RuleInfoList *ruleInfos, const cJSON *paras)
{
    ParseDeviceRuleWithHook(ruleInfos, paras, NULL, NULL);
}

void ParseDeviceRuleWithHook(RuleInfoList *ruleInfos, const cJSON *paras, void *hookTarget, ParseDeviceRuleHook hook)
{
    CHECK_NULL_RETURN_VOID(paras);

    // parse rules
    cJSON *rules = cJSON_GetObjectItem(paras, "rulesInfos");
    CHECK_NULL_RETURN_VOID(rules);

    cJSON *rule;
    cJSON_ArrayForEach(rule, rules) {
        const char *ruleId = cJSON_GetStringValue(cJSON_GetObjectItem(rule, "ruleId"));
        CHECK_NULL_RETURN_VOID(ruleId);

        cJSON *versionObj = cJSON_GetObjectItem(rule, "ruleVersionInShadow");
        int version;
        if (!GetIntValueFromJson(&version, versionObj)) {
            DEVICE_RULE_ERROR("invalid version in incoming json payload!\n");
            break;
        }

        RuleInfo *toBeOperatedInfo = RuleListCheckExistence(ruleInfos, ruleId);
        // if find any, then delete existing older rule
        if (toBeOperatedInfo != NULL) {
            if (version > toBeOperatedInfo->ruleVersionInShadow) {
                RuleInfoListRemoveItem(ruleInfos, toBeOperatedInfo);
                if ((hookTarget != NULL) && (hook != NULL)) {
                    hook(hookTarget, HW_TRUE, rule);
                }
            } else {
                DEVICE_RULE_WARN("the version %d isn\'t supposed to be smaller than existing rules %d", version,
                    toBeOperatedInfo->ruleVersionInShadow);
                continue;
            }
        }

        // insert new rule
        if ((hookTarget != NULL) && (hook != NULL)) {
            hook(hookTarget, HW_FALSE, rule);
        }

        toBeOperatedInfo = RuleInfoListPush(ruleInfos);
        if (toBeOperatedInfo == NULL) {
            DEVICE_RULE_ERROR("RuleInfoListPush failed");
            break;
        }
        if (IS_ERRVAL(ParseSingleRule(toBeOperatedInfo, rule))) {
            RuleInfoListPop(ruleInfos);
            DEVICE_RULE_ERROR("parse new rule failed");
            break;
        }
    }
}

static int ParseCmdAction(Action *actionObj, cJSON *actionCJSON)
{
    char *type = cJSON_GetStringValue(cJSON_GetObjectItem(actionCJSON, "type"));
    char *status = cJSON_GetStringValue(cJSON_GetObjectItem(actionCJSON, "status"));
    char *deviceId = cJSON_GetStringValue(cJSON_GetObjectItem(actionCJSON, "deviceId"));

    // save the action
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&actionObj->type, type));
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&actionObj->status, status));
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&actionObj->deviceId, deviceId));

    cJSON *command = cJSON_GetObjectItem(actionCJSON, "command");
    char *commandName = cJSON_GetStringValue(cJSON_GetObjectItem(command, "commandName"));
    char *serviceId = cJSON_GetStringValue(cJSON_GetObjectItem(command, "serviceId"));

    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&actionObj->command.commandName, commandName));
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&actionObj->command.serviceId, serviceId));

    DEVICE_RULE_DEBUG("in ParseCmdAction, type: %s, deviceId: %s commandName: %s serviceId: %s", type, deviceId,
        commandName, serviceId);
    cJSON *commandBody = cJSON_Duplicate(cJSON_GetObjectItem(command, "commandBody"), HW_TRUE);
    CHECK_NULL_RETURN_ERRVAL(commandBody);
    actionObj->command.commandBody = commandBody;

    return 0;
}

static int ParseDeviceInfo(DeviceInfo *deviceInfo, cJSON *jsonObj)
{
    const char *deviceId = cJSON_GetStringValue(cJSON_GetObjectItem(jsonObj, "deviceId"));
    const char *path = cJSON_GetStringValue(cJSON_GetObjectItem(jsonObj, "path"));
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&deviceInfo->deviceId, deviceId));
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&deviceInfo->path, path));

    DEVICE_RULE_DEBUG("deviceId = \"%s\", path = \"%s\"", deviceInfo->deviceId, deviceInfo->path);

    return 0;
}

static int ParsePropCondition(DeviceDataCondition *condition, cJSON *contentInJson)
{
    char *operator = cJSON_GetStringValue(cJSON_GetObjectItem(contentInJson, "operator"));
    char *value = cJSON_GetStringValue(cJSON_GetObjectItem(contentInJson, "value"));

    // save the condition
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&condition->operator, operator));
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&condition->value, value));

    cJSON *deviceInfo = cJSON_GetObjectItem(contentInJson, "deviceInfo");
    CHECK_NULL_RETURN_ERRVAL(deviceInfo);
    CHECK_ERRVAL_RETURN_ERRVAL(ParseDeviceInfo(&condition->deviceInfo, deviceInfo));

    DEVICE_RULE_DEBUG("in ParsePropCondition, operator is %s, value is %s", operator, value);

    return 0;
}

static int ParseSimpleTimerCondition(SimpleTimerCondition *condition, cJSON *contentInJson)
{
    char *startTime = cJSON_GetStringValue(cJSON_GetObjectItem(contentInJson, "startTime"));
    cJSON *repeatInterval = cJSON_GetObjectItem(contentInJson, "repeatInterval");
    cJSON *repeatCount = cJSON_GetObjectItem(contentInJson, "repeatCount");

    // save the condition
    CHECK_FALSE_RETURN_ERRVAL(GMTTimestampFromStr(&condition->startTime, startTime));
    CHECK_FALSE_RETURN_ERRVAL(GetIntValueFromJson(&condition->repeatInterval, repeatInterval));
    CHECK_FALSE_RETURN_ERRVAL(GetIntValueFromJson(&condition->repeatCount, repeatCount));

    return 0;
}

static int ParseDailyTimerCondition(DailyTimerCondition *condition, cJSON *contentInJson)
{
    char *time = cJSON_GetStringValue(cJSON_GetObjectItem(contentInJson, "time"));
    char *daysOfWeek = cJSON_GetStringValue(cJSON_GetObjectItem(contentInJson, "daysOfWeek"));

    // save the condition
    CHECK_FALSE_RETURN_ERRVAL(ConvertTimeStrToSeconds(&condition->time, time));
    CHECK_FALSE_RETURN_ERRVAL(SaveDaysOfWeek(&condition->daysOfWeek, daysOfWeek));
    return 0;
}

static int ParseSingleCondition(Condition *condition, cJSON *contentInJson)
{
    if (condition == NULL) {
        return -1;
    }

    char *type = cJSON_GetStringValue(cJSON_GetObjectItem(contentInJson, "type"));
    CHECK_NULL_RETURN_ERRVAL(type);
    ConditionType conditionDataType = ConditionTypeFromStr(type);
    switch (conditionDataType) {
        case ConditionTypeDeviceData:
            if (DeviceDataConditionCtor(&condition->data.deviceData)) {
                condition->type = conditionDataType;
                return ParsePropCondition(&condition->data.deviceData, contentInJson);
            }
            return ERRVAL;

        case ConditionTypeSimpleTimer:
            if (SimpleTimerConditionCtor(&condition->data.simpleTimer)) {
                condition->type = conditionDataType;
                return ParseSimpleTimerCondition(&condition->data.simpleTimer, contentInJson);
            }
            return ERRVAL;

        case ConditionTypeDailyTimer:
            if (DailyTimerConditionCtor(&condition->data.dailyTimer)) {
                condition->type = conditionDataType;
                return ParseDailyTimerCondition(&condition->data.dailyTimer, contentInJson);
            }
            return ERRVAL;

        case ConditionTypeEmpty:
        default:
            DEVICE_RULE_ERROR("invalid condition type: %s", type);
            return ERRVAL;
    }
    return 0;
}

static int ParseSingleRule(RuleInfo *ruleInfo, cJSON *rule)
{
    const char *ruleId = cJSON_GetStringValue(cJSON_GetObjectItem(rule, "ruleId"));
    const char *ruleName = cJSON_GetStringValue(cJSON_GetObjectItem(rule, "ruleName"));
    const char *status = cJSON_GetStringValue(cJSON_GetObjectItem(rule, "status"));
    const char *logic = cJSON_GetStringValue(cJSON_GetObjectItem(rule, "logic"));

    cJSON *ruleVersionInShadow = cJSON_GetObjectItem(rule, "ruleVersionInShadow");

    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&ruleInfo->ruleId, ruleId));
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&ruleInfo->ruleName, ruleName));
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&ruleInfo->status, status));
    CHECK_FALSE_RETURN_ERRVAL(CStrDuplicateNotNULL(&ruleInfo->logic, logic));
    CHECK_FALSE_RETURN_ERRVAL(GetIntValueFromJson(&ruleInfo->ruleVersionInShadow, ruleVersionInShadow));

    const cJSON *timeRange = cJSON_GetObjectItem(rule, "timeRange");
    if ((timeRange != NULL) && (!cJSON_IsNull(timeRange))) {
        const char *startTime = cJSON_GetStringValue(cJSON_GetObjectItem(timeRange, "startTime"));
        const char *endTime = cJSON_GetStringValue(cJSON_GetObjectItem(timeRange, "endTime"));
        const char *daysOfWeek = cJSON_GetStringValue(cJSON_GetObjectItem(timeRange, "daysOfWeek"));
        CHECK_FALSE_RETURN_ERRVAL(ConvertTimeStrToSeconds(&ruleInfo->timeRange.startTime, startTime));
        CHECK_FALSE_RETURN_ERRVAL(ConvertTimeStrToSeconds(&ruleInfo->timeRange.endTime, endTime));
        ruleInfo->timeRange.endTime += 59;
        CHECK_FALSE_RETURN_ERRVAL(SaveDaysOfWeek(&ruleInfo->timeRange.daysOfWeek, daysOfWeek));
    }

    // parse conditions
    cJSON *conditions = cJSON_GetObjectItem(rule, "conditions");
    CHECK_NULL_RETURN_ERRVAL(conditions);

    cJSON *oneCondition;
    cJSON_ArrayForEach(oneCondition, conditions) {
        Condition *toBeOperatedCondition = ConditionListPush(&ruleInfo->conditions);
        CHECK_NULL_RETURN_ERRVAL(toBeOperatedCondition);
        if (ParseSingleCondition(toBeOperatedCondition, oneCondition) != 0) {
            ConditionListPop(&ruleInfo->conditions);
            return -1;
        }
    }

    // parse actions, device rule only support cmd action right now
    cJSON *actions = cJSON_GetObjectItem(rule, "actions");
    cJSON *oneAction;
    cJSON_ArrayForEach(oneAction, actions) {
        Action *toBeOperatedAction = ActionListPush(&ruleInfo->actions);
        CHECK_NULL_RETURN_ERRVAL(toBeOperatedAction);
        if (ParseCmdAction(toBeOperatedAction, oneAction) != 0) {
            ActionListPop(&ruleInfo->actions);
            return -1;
        }
    }

    return 0;
}

HW_BOOL GetRuleInfoList(RuleInfoList *list, const cJSON * const propertiesCJSON, RuleInfoList *delList,
    RuleInfoList *addList)
{
    cJSON *oneProperty;
    cJSON_ArrayForEach(oneProperty, propertiesCJSON) {
        // get rule id string
        const char *ruleId = oneProperty->string;
        cJSON *versionObj = cJSON_GetObjectItem(oneProperty, "version");
        DEVICE_RULE_DEBUG("get rule %s ---> ", ruleId);

        // get rule version
        int version;
        if (!GetIntValueFromJson(&version, versionObj)) {
            DEVICE_RULE_ERROR("invalid version in incoming json payload!\n");
            return HW_FALSE;
        }

        // check if the rule id saved already?
        const RuleInfo *oneRule = RuleListCheckExistence(list, ruleId);

        if (version == -1) {
            RuleInfo *p = RuleInfoListPush(delList);
            if (!CStrDuplicate(&p->ruleId, ruleId)) {
                DEVICE_RULE_ERROR("[error]: can't allocate memory for target ruleId");
                RuleInfoListPop(delList);
                return HW_FALSE;
            }
            p->ruleVersionInShadow = version;
            DEVICE_RULE_DEBUG("delete this rule");
        } else if ((oneRule == NULL) || (version > oneRule->ruleVersionInShadow)) {
            RuleInfo *p = RuleInfoListPush(addList);
            if (!CStrDuplicate(&p->ruleId, ruleId)) {
                DEVICE_RULE_ERROR("[error]: can't allocate memory for target ruleId");
                RuleInfoListPop(addList);
                return HW_FALSE;
            }
            p->ruleVersionInShadow = version;
            DEVICE_RULE_DEBUG("add this rule\n");
        } else {
            DEVICE_RULE_WARN("vesrion smaller than/equal to existing rule!");
        }
        putchar('\n');
    }

    return HW_TRUE;
}

void DeletRulesFromList(RuleInfoList *list, const RuleInfoList *delList)
{
    RuleInfo *listItem;
    RuleInfo *delItem;

    ConstDyListFor (delItem, delList) {
        HW_BOOL found = HW_FALSE;
        DyListFor (listItem, list) {
            // find the rule and delete it
            if (strcmp(listItem->ruleId, delItem->ruleId) == 0) {
                RuleInfoListRemoveItem(list, listItem);
                found = HW_TRUE;
                break;
            }
        }
        if (!found) {
            DEVICE_RULE_DEBUG("rule: %s doesn't exist", delItem->ruleId);
        } else {
            DEVICE_RULE_DEBUG("rule: %s del ok", delItem->ruleId);
        }
    }
}
