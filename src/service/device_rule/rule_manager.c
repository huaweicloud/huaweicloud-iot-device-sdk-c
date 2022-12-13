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

#include <pthread.h>
#include <time.h>
#include "securec.h"
#include "rule_util.h"
#include "rule_manager.h"
#include "rule_execute.h"
#include "rule_parse.h"

#ifdef DEVIC_ERULE_ENALBE

static RuleInfoList g_ruleInfosList;
static pthread_mutex_t g_ruleMutex;
PFN_CMD_CALLBACK_HANDLER g_commandCallbackHandler;
static pthread_t g_deviceRuleThreadId;
static DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER g_deviceRuleSendMsgCallBack;

void RuleMgr_SetSendMsgCallback(DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER pfnCallbackHandler)
{
    g_deviceRuleSendMsgCallBack = pfnCallbackHandler;
}

typedef struct {
    char *serviceId;
    cJSON *properties;
} CacheDeviceData;

HW_BOOL CacheDeviceDataCtor(CacheDeviceData *self)
{
    return (memset_s(self, sizeof(*self), 0, sizeof(*self)) == EOK);
}

void CacheDeviceDataDtor(CacheDeviceData *self)
{
    MemFree(&self->serviceId);
    DeviceRuleJSONDtor(&self->properties);
}

DECL_DYARRY_FUNC_UTIL(CacheDeviceDataList, CacheDeviceData, elements);
DECL_DYARRY_FUNC_UTIL_IMPL(CacheDeviceDataList, CacheDeviceData, elements, CacheDeviceDataCtor, CacheDeviceDataDtor);

static CacheDeviceDataList g_cacheDeviceDataList;

static void executeCommandCallback(const char *deviceId, const Command * commandIn)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    const char *serviceId =    commandIn ->serviceId;
    const char *commandName =  commandIn ->commandName;
    const cJSON *commandBody = commandIn ->commandBody;
    EN_IOTA_COMMAND command;
    EN_IOTA_MQTT_MSG_INFO dummyMsg;
    (void)memset_s(&command, sizeof(command), 0, sizeof(command));
    (void)memset_s(&dummyMsg, sizeof(dummyMsg), 0, sizeof(dummyMsg));
    command.mqtt_msg_info = &dummyMsg;

    command.object_device_id = DEVICE_RULE;
    
    if (!CStrDuplicate(&command.service_id, serviceId)) {
        DEVICE_RULE_ERROR("can't copy serviceId name");
        return;
    }
    if (!CStrDuplicate(&command.command_name, commandName)) {
        DEVICE_RULE_ERROR("can't copy command name");
        MemFree(&command.service_id);
        return;
    }
    command.request_id = DEVICE_RULE;
    char *paras = cJSON_Print(commandBody);
    if (paras == NULL) {
        DEVICE_RULE_ERROR("can't create paras for executing command");
        return;
    }
    command.paras = paras;
    if (g_commandCallbackHandler == NULL) {
        DEVICE_RULE_ERROR("command callback can't be NULL, need to set it");
        return;
    }
    if (strcmp(deviceId, username) != 0) {
        DEVICE_RULE_DEBUG("send message from  this device : %s to device : %s", username, deviceId);
        cJSON *cmdRoot;
        cmdRoot = cJSON_CreateObject();
        cJSON_AddStringToObject(cmdRoot, SERVICE_ID_V3, serviceId);
        cJSON_AddStringToObject(cmdRoot, COMMAND_NAME, commandName);
        cJSON_AddItemReferenceToObject(cmdRoot, PARAS, commandBody);

        char *cmd = cJSON_Print(cmdRoot);
        g_deviceRuleSendMsgCallBack(deviceId, cmd);
        cJSON_Delete(cmdRoot);
        MemFree(&cmd);
    } else {
        g_commandCallbackHandler(&command);
    }
    MemFree(&command.service_id);
    MemFree(&command.command_name);
    MemFree(&paras);
}

static HW_BOOL getPropertyCallback(const char *serviceId, const char *propertyName, PropertyValue *value)
{
    CacheDeviceData *dataItem;
    DyListFor (dataItem, &g_cacheDeviceDataList) {
        if (strcmp(dataItem->serviceId, serviceId) != 0) {
            continue;
        }
        cJSON *propertyItem;
        cJSON_ArrayForEach(propertyItem, dataItem->properties) {
            if (strcmp(propertyItem->string, propertyName) != 0) {
                continue;
            }

            if (cJSON_IsString(propertyItem)) {
                PropertyValueSetCStr(value, cJSON_GetStringValue(propertyItem));
                return HW_TRUE;
            }

            if (cJSON_IsNumber(propertyItem)) {
                PropertyValueSetDouble(value, propertyItem->valuedouble);
                return HW_TRUE;
            }
            return HW_TRUE;
        }
    }

    return HW_FALSE;
}
#endif

void RuleMgr_CachePropertiesValue(const ST_IOTA_SERVICE_DATA_INFO *pServiceData, const int serviceNum)
{
#ifdef DEVIC_ERULE_ENALBE
    int i;
    CacheDeviceDataListDtor(&g_cacheDeviceDataList);
    CacheDeviceDataListCtor(&g_cacheDeviceDataList);
    for (i = 0; i < serviceNum; i++) {
        CacheDeviceData *data = CacheDeviceDataListPush(&g_cacheDeviceDataList);
        if (data == NULL) {
            DEVICE_RULE_ERROR("can't allocate memory for cache device data");
            return; // don't worry, it'll be relase at next time when calling this function
        }

        CStrDuplicateNotNULL(&data->serviceId, pServiceData[i].service_id);
        cJSON *properties = cJSON_Parse(pServiceData[i].properties);
        if (properties == NULL) {
            DEVICE_RULE_ERROR("can't parse properties");
            return;
        }

        data->properties = properties;
    }
#endif
}

static void *RuleMgr_CheckAndExecute(void *args);

HW_BOOL RuleMgr_Init(void)
{
#ifdef DEVIC_ERULE_ENALBE
    g_commandCallbackHandler = NULL;
    if (pthread_mutex_init(&g_ruleMutex, NULL) != 0) {
        DEVICE_RULE_ERROR("initialize mutext failed!");
        return HW_FALSE;
    }
    if (!RuleInfoListCtor(&g_ruleInfosList)) {
        return HW_FALSE;
    }

    if (!CacheDeviceDataListCtor(&g_cacheDeviceDataList)) {
        return HW_FALSE;
    }

    return (pthread_create(&g_deviceRuleThreadId, NULL, RuleMgr_CheckAndExecute, NULL) == 0);
#else
    return HW_TRUE;
#endif
}

void RuleMgr_SetCommandCallbackHandler(PFN_CMD_CALLBACK_HANDLER pfnCallbackHandler)
{
#ifdef DEVIC_ERULE_ENALBE
    g_commandCallbackHandler = pfnCallbackHandler;
#endif
}

void RuleMgr_Destroy()
{
#ifdef DEVIC_ERULE_ENALBE
    pthread_mutex_destroy(&g_ruleMutex);
    RuleInfoListDtor(&g_ruleInfosList);
#endif
}

void RuleMgr_Parse(const char *payload)
{
#ifdef DEVIC_ERULE_ENALBE
    pthread_mutex_lock(&g_ruleMutex);
    ParseDeviceRule(&g_ruleInfosList, payload);
    pthread_mutex_unlock(&g_ruleMutex);
#endif
}

static void *RuleMgr_CheckAndExecute(void *args)
{
    (void)args;
#ifdef DEVIC_ERULE_ENALBE
    while (1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        time_t epochTime = tv.tv_sec;

        if (!RuleInfoListEmpty(&g_ruleInfosList)) {
            DEVICE_RULE_INFO(">>>>>>>>>>>>>>>>>start to check rules: %lld>>>>>>>>>>>>>>>>\n", (long long)epochTime);
            pthread_mutex_lock(&g_ruleMutex);
            CheckRuleInfoListAndExecute(&g_ruleInfosList, getPropertyCallback, executeCommandCallback,
                HW_TRUE, epochTime);
            pthread_mutex_unlock(&g_ruleMutex);
        }

        gettimeofday(&tv, NULL);
        int sleepMs = 1000 - tv.tv_usec / 1000;

        TimeSleep(sleepMs);
    }
#endif
    return NULL;
}

void RuleMgr_CheckAndExecuteNoTimers(void)
{
#ifdef DEVIC_ERULE_ENALBE
        struct timeval tv;
        gettimeofday(&tv, NULL);

        time_t epochTime = tv.tv_sec;
        pthread_mutex_lock(&g_ruleMutex);
        CheckRuleInfoListAndExecute(&g_ruleInfosList, getPropertyCallback, executeCommandCallback,
            HW_FALSE, epochTime);
        pthread_mutex_unlock(&g_ruleMutex);
#endif
}

HW_BOOL RuleMgr_GetList(const cJSON *properties, RuleInfoList *delList, RuleInfoList *addList)
{
#ifdef DEVIC_ERULE_ENALBE
    // get addList and delList
    if (!GetRuleInfoList(&g_ruleInfosList, properties, delList, addList)) {
        DEVICE_RULE_ERROR("can't get add/deletion information form properties\n");
        return HW_FALSE;
    }
#endif
    return HW_TRUE;
}

void RuleMgr_DelRule(RuleInfoList *delList)
{
#ifdef DEVIC_ERULE_ENALBE
    // del rules from list
    pthread_mutex_lock(&g_ruleMutex);
    DeletRulesFromList(&g_ruleInfosList, delList);
    pthread_mutex_unlock(&g_ruleMutex);
#endif
}
