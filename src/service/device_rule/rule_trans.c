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

#include "rule_manager.h"
#include "securec.h"
#include "iota_datatrans.h"
#include "log_util.h"
#include "data_trans.h"
#include "string_util.h"
#include "rule_trans.h"

static void DeviceRulePropertiesReply(cJSON *properties)
{
    cJSON *services = cJSON_CreateArray();
    cJSON *service = cJSON_CreateObject();
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddItemReferenceToObject(service, PROPERTIES, properties);
    cJSON_AddStringToObject(service, SERVICE_ID, DEVICE_RULE);
    cJSON_AddNullToObject(service, EVENT_TIME);
    cJSON_AddItemToArray(services, service);
    cJSON_AddItemToObject(payload, SERVICES, services);

    char *msg = cJSON_Print(payload);
    cJSON_Delete(payload);

    if (msg == NULL) {
        return;
    }
    if (ReportDeviceProperties(msg, 0, NULL, NULL) < 0) {
        DEVICE_RULE_ERROR("DeviceRulePropertiesReply() failed!\n");
    }
    MemFree(&msg);
}

static void DeviceRuleConfigReply(RuleInfo *reportList, int reportCount)
{
    int i;
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();
    cJSON *serviceEvent = cJSON_CreateObject();

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
    }

    DEVICE_RULE_DEBUG("DeviceRuleConfigReply() with payload %s \n", payload);
    if (EventUp(payload, NULL) < 0) {
        DEVICE_RULE_ERROR("DeviceRulePropertiesReply() failed!\n");
    } else {
        DEVICE_RULE_INFO("DeviceRulePropertiesReply() success!\n");
    }
    MemFree(&payload);
}

void RuleTrans_DeviceRuleUpdateByJSON(cJSON *properties)
{
    RuleInfoList delList;
    RuleInfoList addList;

    if (!RuleInfoListCtor(&delList)) {
        DEVICE_RULE_ERROR("initialize delList failed!\n");
        goto RELEASE_RET;
    }

    if (!RuleInfoListCtor(&addList)) {
        DEVICE_RULE_ERROR("initialize addList failed!\n");
        goto RELEASE_DEL_LIST;
    }

    if (!RuleMgr_GetList(properties, &delList, &addList)) {
        DEVICE_RULE_ERROR("can't get add/deletion list\n");
        goto RELEASE_ALL;
    }

    // report list to cloud platform
    DEVICE_RULE_INFO("delsize is %lu addsize is %lu\n", delList.size, addList.size);
    DeviceRulePropertiesReply(properties);
    if (delList.size > 0) {
        RuleMgr_DelRule(&delList);
    }

    if (addList.size > 0) {
        DeviceRuleConfigReply(addList.elements, addList.size);
    }

    // del resources
RELEASE_ALL:
    RuleInfoListDtor(&addList);
RELEASE_DEL_LIST:
    RuleInfoListDtor(&delList);
RELEASE_RET:
    return;
}

void RuleTrans_DeviceRuleUpdate(char *ruleIds)
{
    cJSON *properties = cJSON_Parse(ruleIds);
    RuleTrans_DeviceRuleUpdateByJSON(properties);
    cJSON_Delete(properties);
}