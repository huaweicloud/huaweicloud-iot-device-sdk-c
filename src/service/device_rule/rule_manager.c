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

#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <linux/limits.h>

#include "securec.h"
#include "mqtt_base.h"
#include "string_util.h"
#include "rule_parser.h"
#include "string_util.h"
#include "mqtt_base.h"
#include "rule_util.h"
#include "iota_datatrans.h"
#include "rule_execution.h"
#include "rule_manager.h"
#include "stdlib.h"

#ifdef DEVICE_RULE_ENALBE

typedef struct {
    char *str;
    pthread_mutex_t m;
} AtomicString;

static unsigned long long GeySysTmeInMs(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((unsigned long long)tv.tv_sec) * 1000ull + (unsigned long long)tv.tv_usec / 1000ull;
}

static HW_BOOL AtomicStringInit(AtomicString *self)
{
    self->str = NULL;
    if (pthread_mutex_init(&self->m, NULL) != 0) {
        DEVICE_RULE_ERROR("initialize mutex failed!");
        return HW_FALSE;
    }
    return HW_TRUE;
}

static HW_BOOL AtomicStringUpdate(AtomicString *self, const char *str)
{
    HW_BOOL ret = HW_TRUE;
    if (pthread_mutex_lock(&self->m) != 0) {
        DEVICE_RULE_ERROR("lock mutex failed!");
        return HW_FALSE;
    }

    if (!CStrDuplicate(&self->str, str)) {
        DEVICE_RULE_WARN("can't copy string");
        ret = HW_FALSE;
    }

    if (pthread_mutex_unlock(&self->m) != 0) {
        DEVICE_RULE_ERROR("unlock mutex failed!");
        return HW_FALSE;
    }
    return ret;
}

static char *AtomicStringCopy(AtomicString *self)
{
    char *ret = NULL;
    if (pthread_mutex_lock(&self->m) != 0) {
        DEVICE_RULE_ERROR("lock mutex failed!");
        return HW_FALSE;
    }

    if (!CStrDuplicate(&ret, self->str)) {
        DEVICE_RULE_ERROR("can't copy atmoic string");
    }

    if (pthread_mutex_unlock(&self->m) != 0) {
        DEVICE_RULE_ERROR("unlock mutex failed!");
        return HW_FALSE;
    }
    return ret;
}

static void AtomicStringDestory(AtomicString *self)
{
    MemFree(&self->str);
    pthread_mutex_destroy(&self->m);
}

static RuleInfoList g_ruleInfosList;
static pthread_mutex_t g_ruleMutex;
static PFN_CMD_CALLBACK_HANDLER g_commandCallbackHandler = NULL;
static PFN_DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER g_deviceRuleSendMsgCallBack = NULL;
static pthread_t g_deviceRuleThreadId;

#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
static AtomicString g_ruleFilePath;
static cJSON *g_ruleInfosListJSON = NULL;
#endif
static AtomicString g_currentUserName;

static cJSON *g_cacheDeviceDataJSON = NULL;
typedef struct {
    char *deviceId;
    Command command;
} ExeucutionThreadArg;

static void ExeucutionThreadArgDestory(ExeucutionThreadArg *args)
{
    MemFree(&args->deviceId);
    MemFree(&args->command.serviceId);
    MemFree(&args->command.commandName);
    cJSON_Delete(args->command.commandBody);
    MemFree(&args);
}

static void *executeCommandCallbackThread(void *argsIn)
{
    char *username = AtomicStringCopy(&g_currentUserName);
    if (username == NULL) {
        goto RELEASE;
    }
    ExeucutionThreadArg *args = (ExeucutionThreadArg *)argsIn;

    char *deviceId = args->deviceId;
    char *serviceId = args->command.serviceId;
    char *commandName = args->command.commandName;
    cJSON *commandBody = args->command.commandBody;

    EN_IOTA_COMMAND command;
    EN_IOTA_MQTT_MSG_INFO dummyMsg;
    (void)memset_s(&command, sizeof(command), 0, sizeof(command));
    (void)memset_s(&dummyMsg, sizeof(dummyMsg), 0, sizeof(dummyMsg));

    command.mqtt_msg_info = &dummyMsg;
    command.service_id = serviceId;
    command.command_name = commandName;
    command.object_device_id = deviceId;
    command.request_id = DEVICE_RULE;

    char *paras = cJSON_Print(commandBody);
    if (paras == NULL) {
        DEVICE_RULE_ERROR("can't create paras for executing command");
        goto RELEASE;
    }
    command.paras = paras;

    if (g_commandCallbackHandler == NULL) {
        DEVICE_RULE_ERROR("command callback can't be NULL, need to set it");
        goto RELEASE;
    }
    if (strcmp(deviceId, username) != 0) {
        DEVICE_RULE_DEBUG("send message from  this device : %s to device : %s", username, deviceId);
        cJSON *cmdRoot;
        cmdRoot = cJSON_CreateObject();
        cJSON_AddStringToObject(cmdRoot, SERVICE_ID, serviceId);
        cJSON_AddStringToObject(cmdRoot, COMMAND_NAME, commandName);
        cJSON_AddItemReferenceToObject(cmdRoot, PARAS, commandBody);

        char *cmd = cJSON_Print(cmdRoot);
        g_deviceRuleSendMsgCallBack(deviceId, cmd);
        cJSON_Delete(cmdRoot);
        MemFree(&cmd);
    } else {
        g_commandCallbackHandler(&command);
    }
RELEASE:
    MemFree(&username);
    MemFree(&paras);
    ExeucutionThreadArgDestory(args);
    return NULL;
}


static void executeCommandCallback(const char *deviceId, const Command *commandIn)
{
    ExeucutionThreadArg *args = malloc(sizeof(ExeucutionThreadArg));
    if (args == NULL) {
        DEVICE_RULE_ERROR("ExeucutionThreadArg allocation error");
        return;
    }
    (void)memset_s(args, sizeof(ExeucutionThreadArg), 0, sizeof(ExeucutionThreadArg));

    if (!CStrDuplicate(&args->deviceId, deviceId)) {
        DEVICE_RULE_ERROR("can't copy serviceId name");
        goto ERROR_RELEASE;
    }
    if (!CStrDuplicate(&args->command.serviceId, commandIn->serviceId)) {
        DEVICE_RULE_ERROR("can't copy serviceId name");
        goto ERROR_RELEASE;
    }
    if (!CStrDuplicate(&args->command.commandName, commandIn->commandName)) {
        DEVICE_RULE_ERROR("can't copy command name");
        goto ERROR_RELEASE;
    }
    args->command.commandBody = cJSON_Duplicate(commandIn->commandBody, cJSON_True);
    if (args->command.commandBody == NULL) {
        DEVICE_RULE_ERROR("can't copy command body");
        goto ERROR_RELEASE;
    }

    // execute at another thread to prevent dead lock
    pthread_t commandExecuteThread;
    if (pthread_create(&commandExecuteThread, NULL, executeCommandCallbackThread, args) != 0) {
        DEVICE_RULE_ERROR("create command execution thread error");
        goto ERROR_RELEASE;
    }
    (void)pthread_detach(commandExecuteThread);
    return;

ERROR_RELEASE:
    ExeucutionThreadArgDestory(args);
}

void RuleMgr_SetSendMsgCallback(PFN_DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER cb)
{
    g_deviceRuleSendMsgCallBack = cb;
}

void RuleMgr_SetCommandCallbackHandler(PFN_CMD_CALLBACK_HANDLER callbackHandler)
{
    g_commandCallbackHandler = callbackHandler;
}

HW_BOOL RuleMgr_SetCurrentUserName(const char *username)
{
    return AtomicStringUpdate(&g_currentUserName, username);
}

// should be moved to other places
static cJSON *IOTA_JsonFindServiceInListById(cJSON *services, const char *serviceId)
{
    cJSON *serviceItem;
    cJSON_ArrayForEach(serviceItem, services) {
        const char *oldServiceId = cJSON_GetStringValue(cJSON_GetObjectItem(serviceItem, SERVICE_ID));
        if (oldServiceId == NULL) {
            continue;
        }
        if (strcmp(oldServiceId, serviceId) == 0) {
            return serviceItem;
        }
    }

    return NULL;
}

static HW_BOOL getPropertyCallback(const char *serviceId, const char *propertyName, PropertyValue *value)
{
    cJSON *serviceJSON = IOTA_JsonFindServiceInListById(g_cacheDeviceDataJSON, serviceId);
    cJSON *propertiesJSON = cJSON_GetObjectItem(serviceJSON, PROPERTIES);

    cJSON *propertyItem;
    cJSON_ArrayForEach(propertyItem, propertiesJSON) {
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

    return HW_FALSE;
}

void RuleMgr_CachePropertiesValue(const cJSON *serviceData)
{
    const cJSON *serviceJSON;
    cJSON_ArrayForEach(serviceJSON, serviceData) {
        const char *serviceId = cJSON_GetStringValue(cJSON_GetObjectItem(serviceJSON, SERVICE_ID));
        if (serviceId == NULL) {
            continue;
        }
        cJSON *oldServiceJSON = IOTA_JsonFindServiceInListById(g_cacheDeviceDataJSON, serviceId);
        if (oldServiceJSON != NULL) {
            cJSON_DetachItemViaPointer(g_cacheDeviceDataJSON, oldServiceJSON);
            cJSON_Delete(oldServiceJSON);
        }
        cJSON_AddItemToArray(g_cacheDeviceDataJSON, cJSON_Duplicate(serviceJSON, HW_TRUE));
    }
}

static void *RuleMgr_CheckAndExecute(void *args);

HW_BOOL RuleMgr_Init()
{
#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
    if ((g_ruleInfosListJSON = cJSON_CreateArray()) == NULL) {
        DEVICE_RULE_ERROR("create g_ruleInfosListJSON failed");
        goto RELEASE_CJSON;
    }
#endif
    if ((g_cacheDeviceDataJSON = cJSON_CreateArray()) == NULL) {
        DEVICE_RULE_ERROR("create g_cacheDeviceDataJSON failed");
        goto RELEASE_CJSON;
    }
#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
    if (!AtomicStringInit(&g_ruleFilePath)) {
        DEVICE_RULE_ERROR("initialize g_ruleFilePath failed!");
        goto RELEASE_CJSON;
    }
#endif
    if (!AtomicStringInit(&g_currentUserName)) {
        DEVICE_RULE_ERROR("initialize g_currentUserName failed!");
        goto RELEASE_RULE_FILE_PATH;
    }
    g_commandCallbackHandler = NULL;
    g_deviceRuleSendMsgCallBack = NULL;
    if (pthread_mutex_init(&g_ruleMutex, NULL) != 0) {
        DEVICE_RULE_ERROR("initialize mutex failed!");
        goto RELEASE_CURRENT_USER_NAME;
    }
    if (!RuleInfoListCtor(&g_ruleInfosList)) {
        DEVICE_RULE_ERROR("initialize g_ruleInfosList failed!");
        goto RELEASE_RULE_MUTEX;
    }

    if (pthread_create(&g_deviceRuleThreadId, NULL, RuleMgr_CheckAndExecute, NULL) != 0) {
        DEVICE_RULE_ERROR("initialize g_deviceRuleThreadId failed!");
        goto RELEASE_RULE_INFO_LIST;
    }
    return HW_TRUE;

RELEASE_RULE_INFO_LIST:
    RuleInfoListDtor(&g_ruleInfosList);
RELEASE_RULE_MUTEX:
    pthread_mutex_destroy(&g_ruleMutex);
RELEASE_CURRENT_USER_NAME:
    AtomicStringDestory(&g_currentUserName);
RELEASE_RULE_FILE_PATH:
#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
    AtomicStringDestory(&g_ruleFilePath);
#endif
RELEASE_CJSON:
#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
    cJSON_Delete(g_ruleInfosListJSON);
#endif
    cJSON_Delete(g_cacheDeviceDataJSON);
    return HW_FALSE;
}

void RuleMgr_Destroy()
{
#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
    cJSON_Delete(g_ruleInfosListJSON);
#endif
    cJSON_Delete(g_cacheDeviceDataJSON);
#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
    AtomicStringDestory(&g_ruleFilePath);
#endif
    AtomicStringDestory(&g_currentUserName);
    pthread_mutex_destroy(&g_ruleMutex);
    RuleInfoListDtor(&g_ruleInfosList);
}

#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
static void RuleJSONObjSaveToFile(const char *filePath)
{
    if (filePath == NULL) {
        DEVICE_RULE_WARN("file path is none");
        return;
    }
    DEVICE_RULE_INFO("save rule to file %s", filePath);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "rulesInfos", g_ruleInfosListJSON);
    char *content = cJSON_Print(root);
    cJSON_DetachItemViaPointer(root, g_ruleInfosListJSON);
    cJSON_Delete(root);

    if (content == NULL) {
        DEVICE_RULE_WARN("can't convert rule to data to save");
        return;
    }
    DEVICE_RULE_DEBUG("%s", content);

    char realFilePath[PATH_MAX] = {0};
    if (realpath(filePath, realFilePath) == NULL) {
        DEVICE_RULE_ERROR("realpath can't reslove path");
        free(content);
        return;
    }

    FILE *fp = fopen(realFilePath, "wb");
    if (fp == NULL) {
        DEVICE_RULE_ERROR("can't open file to save rule");
        free(content);
        return;
    }

    (void)fputs(content, fp);
    (void)fclose(fp);
    free(content);
}

static void RuleJSONObjDelById(const char *ruleId)
{
    if (ruleId == NULL) {
        return;
    }
    cJSON *searchRule;
    cJSON_ArrayForEach(searchRule, g_ruleInfosListJSON) {
        const char *searchRuleId = cJSON_GetStringValue(cJSON_GetObjectItem(searchRule, "ruleId"));
        if (strcmp(searchRuleId, ruleId) == 0) {
            cJSON_DetachItemViaPointer(g_ruleInfosListJSON, searchRule);
            cJSON_Delete(searchRule);
            break;
        }
    }
}

static void UpdateRuleJSONObj(void *target, HW_BOOL removeOrAdd, const cJSON *rule)
{
    if (removeOrAdd) {
        const char *ruleId = cJSON_GetStringValue(cJSON_GetObjectItem(rule, "ruleId"));
        RuleJSONObjDelById(ruleId);
    } else {
        cJSON_AddItemToArray(g_ruleInfosListJSON, cJSON_Duplicate(rule, cJSON_True));
    }
}
#endif

void RuleMgr_ParseJSON(const cJSON *paras)
{
#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
    char *filePath = AtomicStringCopy(&g_ruleFilePath);

    pthread_mutex_lock(&g_ruleMutex);
    ParseDeviceRuleWithHook(&g_ruleInfosList, paras, g_ruleInfosListJSON, UpdateRuleJSONObj);
    RuleJSONObjSaveToFile(filePath);
    pthread_mutex_unlock(&g_ruleMutex);

    MemFree(&filePath);
#else
    pthread_mutex_lock(&g_ruleMutex);
    ParseDeviceRule(&g_ruleInfosList, paras);
    pthread_mutex_unlock(&g_ruleMutex);
#endif
}

void RuleMgr_Parse(const char *payload)
{
    cJSON *paras = cJSON_Parse(payload);
#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
    char *filePath = AtomicStringCopy(&g_ruleFilePath);

    pthread_mutex_lock(&g_ruleMutex);
    ParseDeviceRuleWithHook(&g_ruleInfosList, paras, g_ruleInfosListJSON, UpdateRuleJSONObj);
    RuleJSONObjSaveToFile(filePath);
    pthread_mutex_unlock(&g_ruleMutex);

    MemFree(&filePath);
#else
    pthread_mutex_lock(&g_ruleMutex);
    ParseDeviceRule(&g_ruleInfosList, paras);
    pthread_mutex_unlock(&g_ruleMutex);
#endif
    cJSON_Delete(paras);
}

static void *RuleMgr_CheckAndExecute(void *args)
{
    (void)args;
    while (1) {
        time_t epochTime = (time_t)(GeySysTmeInMs() / 1000);

        pthread_mutex_lock(&g_ruleMutex);
        HW_BOOL isEmptyRule = RuleInfoListEmpty(&g_ruleInfosList);
        pthread_mutex_unlock(&g_ruleMutex);

        if (!isEmptyRule) {
            pthread_mutex_lock(&g_ruleMutex);
            CheckRuleInfoListAndExecute(&g_ruleInfosList, getPropertyCallback, executeCommandCallback,
                HW_TRUE, epochTime);
            pthread_mutex_unlock(&g_ruleMutex);
        }

        int sleepMs = 1000 - (int)(GeySysTmeInMs() % 1000);
        usleep(sleepMs * 1000);
    }

    return NULL;
}

void RuleMgr_CheckAndExecuteNoTimers(void)
{
    time_t epochTime = (time_t)GeySysTmeInMs() / 1000;
    pthread_mutex_lock(&g_ruleMutex);
    CheckRuleInfoListAndExecute(&g_ruleInfosList, getPropertyCallback, executeCommandCallback, HW_FALSE,
        epochTime);
    pthread_mutex_unlock(&g_ruleMutex);
}

HW_BOOL RuleMgr_GetList(const cJSON *properties, RuleInfoList *delList, RuleInfoList *addList)
{
    // get addList and delList
    pthread_mutex_lock(&g_ruleMutex);
    HW_BOOL ret = GetRuleInfoList(&g_ruleInfosList, properties, delList, addList);
    pthread_mutex_unlock(&g_ruleMutex);

    if (!ret) {
        DEVICE_RULE_ERROR("can't get add/deletion information form properties\n");
        return HW_FALSE;
    }
    return HW_TRUE;
}

void RuleMgr_DelRule(RuleInfoList *delList)
{
    // del rules from list
#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
    char *filePath = AtomicStringCopy(&g_ruleFilePath);

    pthread_mutex_lock(&g_ruleMutex);
    RuleInfo *listItem;
    DyListFor (listItem, delList) {
        // find the rule and delete it
        RuleJSONObjDelById(listItem->ruleId);
    }
    DeletRulesFromList(&g_ruleInfosList, delList);
    RuleJSONObjSaveToFile(filePath);
    pthread_mutex_unlock(&g_ruleMutex);

    MemFree(&filePath);
#else
    pthread_mutex_lock(&g_ruleMutex);
    DeletRulesFromList(&g_ruleInfosList, delList);
    pthread_mutex_unlock(&g_ruleMutex);
#endif
}

#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
void RuleMgr_EnableDeviceRuleStorage(const char *filePath)
{
    if (!AtomicStringUpdate(&g_ruleFilePath, filePath)) {
        return;
    }
    cJSON *paras = NULL;
    char *buffer = NULL;
    FILE *fp = NULL;
    char realFilePath[PATH_MAX] = {0};

    if (realpath(filePath, realFilePath) == NULL) {
        DEVICE_RULE_WARN("realpath can't resolve path");
        goto FAIL;
    }
    fp = fopen(realFilePath, "rb");
    if (fp == NULL) {
        DEVICE_RULE_WARN("can't load rule from file, proceed with none of rules");
        goto FAIL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        DEVICE_RULE_ERROR("error when operating file");
        goto FAIL;
    }

    long fileLength = ftell(fp);
    if (fileLength == -1L) {
        DEVICE_RULE_ERROR("error when get size of file");
        goto FAIL;
    }

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        DEVICE_RULE_ERROR("error when operating file");
        goto FAIL;
    }

    buffer = malloc(fileLength + 1);
    if (buffer == NULL) {
        DEVICE_RULE_ERROR("can't allocate memory for loading rule from file");
        goto FAIL;
    }

    (void)fread(buffer, fileLength, 1, fp);

    buffer[fileLength] = '\0';

    paras = cJSON_Parse(buffer);
    pthread_mutex_lock(&g_ruleMutex);
    ParseDeviceRuleWithHook(&g_ruleInfosList, paras, g_ruleInfosListJSON, UpdateRuleJSONObj);
    pthread_mutex_unlock(&g_ruleMutex);

    DEVICE_RULE_INFO("load rule successfully");

FAIL:
    if (fp != NULL) {
        fclose(fp);
    }
    MemFree(&buffer);
    cJSON_Delete(paras);
}
#endif
#endif