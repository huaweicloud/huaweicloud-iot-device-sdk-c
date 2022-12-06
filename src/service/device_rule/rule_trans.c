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

#include "rule_trans.h"
#include "rule_info.h"
#include "iota_datatrans.h"
#include "log_util.h"
#include "data_trans.h"
#include "string_util.h"

static void DeviceRulePropertiesReply(RuleInfo *reportList, int reportCount)
{
    cJSON *root, *serviceDatas, *properties;
    root = cJSON_CreateObject();
    serviceDatas = cJSON_CreateArray();
    int i;
    for (i = 0; i < reportCount; i++) {
        cJSON *property = cJSON_CreateObject();
        cJSON *ruleIdObj = cJSON_CreateObject();
        cJSON_AddItemToObject(property, reportList[i].ruleId, ruleIdObj);
        cJSON_AddNumberToObject(ruleIdObj, VERSION, reportList[i].ruleVersionInShadow);

        cJSON *tmp = cJSON_CreateObject();
        cJSON_AddStringToObject(tmp, SERVICE_ID, DEVICE_RULE);
        char *timestamp = GetEventTimesStamp();
        if (timestamp == NULL) {
            cJSON_Delete(root);
            return;
        }

        cJSON_AddStringToObject(tmp, EVENT_TIME, timestamp);
        MemFree(&timestamp);
        cJSON_AddItemToObject(tmp, PROPERTIES, property);
        cJSON_AddItemToArray(serviceDatas, tmp);
    }

    cJSON_AddItemToObject(root, SERVICES, serviceDatas);
    char *payload = cJSON_Print(root);
    cJSON_Delete(root);

    if (payload == NULL) {
        return;
    } else {
        (void)ReportDeviceProperties(payload, 0, NULL, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "DeviceRulePropertiesReply() with payload %s ==>\n", payload);
        MemFree(&payload);
    }
}

static void DeviceRuleConfigReply(RuleInfo *reportList, int reportCount)
{
    cJSON *root, *services, *serviceEvent;
    int i;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    serviceEvent = cJSON_CreateObject();

    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, DEVICE_RULE);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, CONFIG_REQUEST);
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, NULL);
    cJSON_AddStringToObject(serviceEvent, EVENT_ID, NULL);

    cJSON *paras = cJSON_CreateObject();

    cJSON *ruleIds = cJSON_CreateArray();
    for (i = 0; i < reportCount; i++) {
        cJSON *ruleId = cJSON_CreateString(reportList[i].ruleId);
        cJSON_AddItemToArray(ruleIds, ruleId);
    }

    cJSON_AddItemToObject(paras, RULE_ID, ruleIds);
    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);

    if (payload == NULL) {
        return;
    } else {
        (void)EventUp(payload, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "DeviceRuleConfigReply() with payload %s ==>\n", payload);
        MemFree(&(payload));
    }
}

void RuleTrans_DeviceRuleUpdate(char *ruleIds)
{
    cJSON *properties = cJSON_Parse(ruleIds);

    RuleInfoList delList;
    RuleInfoList addList;

    if (!RuleInfoListCtor(&delList)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "initialize delList failed!\n");
        return;
    }

    if (!RuleInfoListCtor(&addList)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "initialize addList failed!\n");
        return;
    }
    if (!RuleMgr_GetList(properties, &delList, &addList)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "can't get add/deletion list\n");
        return;
    }

    // report list to cloud platform
    PrintfLog(EN_LOG_LEVEL_DEBUG, "delsize is %d addsize is %d\n", delList.size, addList.size);

    if (delList.size > 0) {
        RuleMgr_DelRule(&delList);
        DeviceRulePropertiesReply(delList.elements, delList.size);
    }

    if (addList.size > 0) {
        DeviceRulePropertiesReply(addList.elements, addList.size);
        DeviceRuleConfigReply(addList.elements, addList.size);
    }

    // del resources
    RuleInfoListDtor(&delList);
    RuleInfoListDtor(&addList);
}
