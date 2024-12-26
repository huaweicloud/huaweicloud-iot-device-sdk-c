/*
 * Copyright (c) 2020-2022 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "securec.h"
#include "string_util.h"
#include "log_util.h"
#include "mqtt_base.h"
#include "base.h"
#include "subscribe.h"
#include "iota_error_type.h"
#include "json_util.h"
#include "cJSON.h"
#include "iota_datatrans.h"
#include "rule_trans.h"
#include "rule_manager.h"
#include "detect_anomaly.h"
#include "sys_hal.h"
#include "callback_func.h"

#if defined(SOFT_BUS_OPTION2)
#include "soft_bus_datatrans.h"
#include "soft_bus_init.h"
#include "data_trans.h"
#endif

EVENT_CALLBACK_HANDLER onEventDown;
CMD_CALLBACK_HANDLER onCmd;
CMD_CALLBACK_HANDLER_V3 onCmdV3;
PROTOCOL_CALLBACK_HANDLER onConnSuccess;
MESSAGE_CALLBACK_HANDLER onMessage;
RAW_MESSAGE_CALLBACK_HANDLER onRawMessage;
PROP_SET_CALLBACK_HANDLER onPropertiesSet;
PROP_GET_CALLBACK_HANDLER onPropertiesGet;
SHADOW_GET_CALLBACK_HANDLER onDeviceShadow;
USER_TOPIC_MSG_CALLBACK_HANDLER onUserTopicMessage;
USER_TOPIC_RAW_MSG_CALLBACK_HANDLER onUserTopicRawMessage;
BOOTSTRAP_CALLBACK_HANDLER onBootstrap;
M2M_CALLBACK_HANDLER onM2mMessage;
DEVICE_CONFIG_CALLBACK_HANDLER onDeviceConfig = NULL;
TagEventsOps tagEventsOps;
TagOtaOps tagOtaOps;
BRIDGES_DEVICE_LOGIN onBridgesDeviceLogin;
BRIDGES_DEVICE_LOGOUT onBridgesDeviceLogout;
BRIDGES_DEVICE_RESET_SECRET onBridgesDeviceResetSecret;
BRIDGES_DEVICE_PLATE_DISCONNECT onBridgesDeviceDisconnect;
UNDEFINED_MESSAGE_CALLBACK_HANDLER onOtherUndefined;

static void OnLoginSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    // The platform subscribes to system topic with QoS of 1 by default
    // SubscribeAll();
    // SubscribeM2m();
    SysHalInit(); 
    if (onConnSuccess) {
        (onConnSuccess)(rsp);
    }
    /*
        // report device log
        unsigned long long timestamp = getTime();
        char timeStampStr[TIMESTAMP_STR_LEN] = {0};
        (void)sprintf_s(timeStampStr, sizeof(timeStampStr), "%llu", timestamp);
        char *log = "login success";
        IOTA_ReportDeviceLog("DEVICE_STATUS", log, timeStampStr, NULL);

        // report sdk version
        ST_IOTA_DEVICE_INFO_REPORT deviceInfo;

        deviceInfo.device_sdk_version = SDK_VERSION;
        deviceInfo.sw_version = NULL;
        deviceInfo.fw_version = NULL;
        deviceInfo.event_time = NULL;
        deviceInfo.object_device_id = NULL;
        deviceInfo.device_ip = NULL;

        IOTA_ReportDeviceInfo(&deviceInfo, NULL);
        IOTA_GetDeviceShadow(DEVICE_RULE_REQUEST_ID, NULL, NULL, NULL);
    */
}

static void OnBootstrapDownArrived(void *context, int token, int code, const char *message)
{
    EN_IOTA_MQTT_PROTOCOL_RSP *bootstrap_msg = (EN_IOTA_MQTT_PROTOCOL_RSP *)malloc(sizeof(EN_IOTA_MQTT_PROTOCOL_RSP));
    if (bootstrap_msg == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnBootstrapDownArrived(): there is not enough memory here.\n");
        return;
    }
    bootstrap_msg->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (bootstrap_msg->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnBootstrapDownArrived(): there is not enough memory here.\n");
        MemFree(&bootstrap_msg);
        return;
    }
    bootstrap_msg->mqtt_msg_info->context = context;
    bootstrap_msg->mqtt_msg_info->messageId = token;
    bootstrap_msg->mqtt_msg_info->code = code;

    JSON *root = JSON_Parse(message);
    bootstrap_msg->message = JSON_GetStringFromObject(root, ADDRESS, "-1");
    bootstrap_msg->deviceSecret =  JSON_GetStringFromObject(root, DEVICE_SECRET, "-1");
    if (onBootstrap) {
        (onBootstrap)(bootstrap_msg);
    }

    JSON_Delete(root);
    MemFree(&bootstrap_msg->mqtt_msg_info);
    MemFree(&bootstrap_msg);
    return;
}

static EN_IOTA_MQTT_MSG_INFO *CreateMqttMsgInfo(void *context, int token, int code)
{
    EN_IOTA_MQTT_MSG_INFO *info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (info == NULL) {
        return NULL;
    }
    info->context = context;
    info->messageId = token;
    info->code = code;
    return info;
}

static bool GetMessageInSystemFormat(cJSON *root, char **objectDeviceId, char **name, char **id, char **content)
{
    if (!cJSON_IsObject(root)) {
        return false;
    }

    // check if any key is not belong to that in system format
    cJSON *kvInMessage = root->child;
    while (kvInMessage) {
        if ((strcmp(kvInMessage->string, CONTENT) != 0) && (strcmp(kvInMessage->string, OBJECT_DEVICE_ID) != 0) &&
            (strcmp(kvInMessage->string, NAME) != 0) && (strcmp(kvInMessage->string, ID) != 0)) {
            return false;
        }
        kvInMessage = kvInMessage->next;
    }

    // check if value is complying to system format
    cJSON *contentObject = cJSON_GetObjectItem(root, CONTENT);
    cJSON *objectDeviceIdObject = cJSON_GetObjectItem(root, OBJECT_DEVICE_ID);
    cJSON *nameObject = cJSON_GetObjectItem(root, NAME);
    cJSON *idObject = cJSON_GetObjectItem(root, ID);
    if ((objectDeviceIdObject && !cJSON_IsNull(objectDeviceIdObject) && !cJSON_IsString(objectDeviceIdObject)) ||
        (nameObject && !cJSON_IsNull(nameObject) && !cJSON_IsString(nameObject)) ||
        (idObject && !cJSON_IsNull(idObject) && !cJSON_IsString(idObject)) ||
        (contentObject && !cJSON_IsNull(contentObject) && (!cJSON_IsString(contentObject) && !cJSON_IsObject(contentObject)))) {
        return false; // is not system format
    }

    *objectDeviceId = cJSON_GetStringValue(objectDeviceIdObject);
    *name = cJSON_GetStringValue(nameObject);
    *id = cJSON_GetStringValue(idObject);
    if (cJSON_IsObject(contentObject)) {
        *content = JSON_Print(contentObject);
    } else {
        *content = cJSON_GetStringValue(contentObject);
    }
    return true;
}

static void OnRawMessageDownArrived(void *context, int token, int code, char *message, int messageLength, void *mqttv5)
{
    EN_IOTA_RAW_MESSAGE *rawMsg = (EN_IOTA_RAW_MESSAGE *)malloc(sizeof(EN_IOTA_RAW_MESSAGE));
    if (rawMsg == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnMessagesDownArrived(): there is not enough memory here.\n");
        return;
    }
    (void)memset_s(rawMsg, sizeof(EN_IOTA_RAW_MESSAGE), 0, sizeof(EN_IOTA_RAW_MESSAGE));

    rawMsg->mqttMsgInfo = CreateMqttMsgInfo(context, token, code);
    if (rawMsg->mqttMsgInfo == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnMessagesDownArrived(): there is not enough memory here.\n");
        goto EXIT;
    }

    rawMsg->payloadLength = messageLength;
    rawMsg->payload = message;
    if (onRawMessage) {
        (onRawMessage)(rawMsg, mqttv5);
    }
EXIT:
    MemFree(&rawMsg->mqttMsgInfo);
    MemFree(&rawMsg);
}


static void OnMessagesDownArrived(void *context, int token, int code, char *bridgeDeviceId, char *message, int messageLength, void *mqttv5)
{
    OnRawMessageDownArrived(context, token, code, message, messageLength, mqttv5);

    cJSON *root = cJSON_Parse(message);
    EN_IOTA_MESSAGE *msg = (EN_IOTA_MESSAGE *)malloc(sizeof(EN_IOTA_MESSAGE));
    if (msg == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnMessagesDownArrived(): there is not enough memory here.\n");
        goto EXIT;
    }
    msg->bridge_device_id = bridgeDeviceId;
    msg->mqtt_msg_info = CreateMqttMsgInfo(context, token, code);
    if (msg->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnMessagesDownArrived(): there is not enough memory here.\n");
        goto EXIT;
    }

    if (!GetMessageInSystemFormat(root, &msg->object_device_id, &msg->name, &msg->id, &msg->content)) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "OnMessagesDownArrived(): message is not system fomart.\n");
        goto EXIT;
    }
    if (onMessage) {
        (onMessage)(msg, mqttv5);
    }

EXIT:
    cJSON_Delete(root);
    if (msg) {
        MemFree(&msg->mqtt_msg_info);
    }
    MemFree(&msg);
}

static void OnOtherUndefinedTopicArrived(void *context, int token, int code, char *bridgeDeviceId, const char *message)
{
    EN_IOTA_UNDEFINED_MESSAGE *msg = (EN_IOTA_UNDEFINED_MESSAGE *)malloc(sizeof(EN_IOTA_UNDEFINED_MESSAGE));
    if (msg == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnOtherUndefinedTopicArrived(): there is not enough memory here.\n");
        return;
    }
    
    msg->bridge_device_id = bridgeDeviceId;
    msg->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (msg->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnOtherUndefinedTopicArrived(): there is not enough memory here.\n");
        free(msg);
        return;
    }
    msg->mqtt_msg_info->context = context;
    msg->mqtt_msg_info->messageId = token;
    msg->mqtt_msg_info->code = code;
    msg->payload = message;
    if (onOtherUndefined) {
        (onOtherUndefined)(msg);
    }

    MemFree(&msg->mqtt_msg_info);
    MemFree(&msg);
    return;
}

static void OnM2mMessagesDownArrived(void *context, int token, int code, char *bridgeDeviceId, const char *message)
{
    EN_IOTA_M2M_MESSAGE *m2mMsg = (EN_IOTA_M2M_MESSAGE *)malloc(sizeof(EN_IOTA_M2M_MESSAGE));
    if (m2mMsg == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnM2mMessagesDownArrived(): there is not enough memory here.\n");
        return;
    }
    m2mMsg->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (m2mMsg->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnM2mMessagesDownArrived(): there is not enough memory here.\n");
        free(m2mMsg);
        return;
    }
    m2mMsg->mqtt_msg_info->context = context;
    m2mMsg->mqtt_msg_info->messageId = token;
    m2mMsg->mqtt_msg_info->code = code;

    JSON *root = JSON_Parse(message);
    m2mMsg->request_id = JSON_GetStringFromObject(root, REQUEST_ID, NULL);
    m2mMsg->to = JSON_GetStringFromObject(root, TO, NULL);
    m2mMsg->from = JSON_GetStringFromObject(root, FROM, NULL);
    m2mMsg->content = JSON_GetStringFromObject(root, CONTENT, NULL);
    if (onM2mMessage) {
        (onM2mMessage)(m2mMsg);
    }

    JSON_Delete(root);
    MemFree(&m2mMsg->mqtt_msg_info);
    MemFree(&m2mMsg);
    return;
}

static void OnV1DevicesArrived(void *context, int token, int code, const char *message)
{
    EN_IOTA_COMMAND_V3 *command_v3 = (EN_IOTA_COMMAND_V3 *)malloc(sizeof(EN_IOTA_COMMAND));
    if (command_v3 == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnV1DevicesArrived(): there is not enough memory here.\n");
        return;
    }
    command_v3->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (command_v3->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnV1DevicesArrived(): there is not enough memory here.\n");
        MemFree(&command_v3);
        return;
    }
    command_v3->mqtt_msg_info->context = context;
    command_v3->mqtt_msg_info->messageId = token;
    command_v3->mqtt_msg_info->code = code;

    command_v3->msgType = CLOUD_REQ;

    JSON *root = JSON_Parse(message);

    char *serviceId = JSON_GetStringFromObject(root, SERVICE_ID_V3, "-1");
    command_v3->serviceId = serviceId;

    char *cmd = JSON_GetStringFromObject(root, CMD, "-1");
    command_v3->cmd = cmd;

    int mid = JSON_GetIntFromObject(root, MID, -1);
    command_v3->mid = mid;

    JSON *paras = JSON_GetObjectFromObject(root, PARAS);

    char *paras_obj = cJSON_Print(paras);
    command_v3->paras = paras_obj;

    if (onCmdV3) {
        (onCmdV3)(command_v3);
    }

    JSON_Delete(root);
    MemFree(&paras_obj);
    MemFree(&command_v3->mqtt_msg_info);
    MemFree(&command_v3);
    return;
}

static void OnCommandsArrived(void *context, int token, int code, char *bridgeDeviceId, const char *topic, const char *message)
{
    char *requestId_value = strstr(topic, "=");
    char *request_id = strstr(requestId_value + 1, "");

    EN_IOTA_COMMAND *command = (EN_IOTA_COMMAND *)malloc(sizeof(EN_IOTA_COMMAND));
    if (command == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        return;
    }

    command->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (command->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        MemFree(&command);
        return;
    }
    command->mqtt_msg_info->context = context;
    command->mqtt_msg_info->messageId = token;
    command->mqtt_msg_info->code = code;
    command->bridge_device_id = bridgeDeviceId;

    command->request_id = request_id;

    JSON *root = JSON_Parse(message);

    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");
    command->object_device_id = object_device_id;

    char *service_id = JSON_GetStringFromObject(root, SERVICE_ID, "-1");
    command->service_id = service_id;

    char *command_name = JSON_GetStringFromObject(root, COMMAND_NAME, "-1");
    command->command_name = command_name;

    JSON *paras = JSON_GetObjectFromObject(root, PARAS);

    char *paras_obj = cJSON_Print(paras);
    command->paras = paras_obj;

    if (onCmd) {
        (onCmd)(command);
    }

    JSON_Delete(root);
    MemFree(&paras_obj);
    MemFree(&command->mqtt_msg_info);
    MemFree(&command);
    return;
}

static void OnPropertiesSetArrived(void *context, int token, int code, char *bridgeDeviceId, const char *topic, const char *message)
{
    char *requestId_value = strstr(topic, "=");
    char *request_id = strstr(requestId_value + 1, "");

    EN_IOTA_PROPERTY_SET *prop_set = (EN_IOTA_PROPERTY_SET *)malloc(sizeof(EN_IOTA_PROPERTY_SET));
    if (prop_set == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        return;
    }

    prop_set->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (prop_set->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        MemFree(&prop_set);
        return;
    }
    prop_set->mqtt_msg_info->context = context;
    prop_set->mqtt_msg_info->messageId = token;
    prop_set->mqtt_msg_info->code = code;
    prop_set->bridge_device_id = bridgeDeviceId;

    prop_set->request_id = request_id;

    JSON *root = JSON_Parse(message);

    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");
    prop_set->object_device_id = object_device_id;

    JSON *services = JSON_GetObjectFromObject(root, SERVICES);

    int services_count = JSON_GetArraySize(services);
    prop_set->services_count = 0;

    if (services_count >= MAX_SERVICE_COUNT) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the number of service exceeds.\n");
        JSON_Delete(root);
        MemFree(&prop_set->mqtt_msg_info);
        MemFree(&prop_set);
        return;
    }

    prop_set->services = (EN_IOTA_SERVICE_PROPERTY *)malloc(sizeof(EN_IOTA_SERVICE_PROPERTY) * services_count);
    if (prop_set->services == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        JSON_Delete(root);
        MemFree(&prop_set->mqtt_msg_info);
        MemFree(&prop_set);
        return;
    }

    int i = 0;
    char *prop[MAX_SERVICE_COUNT] = {0};
    JSON *service = NULL;
    cJSON_ArrayForEach(service, services) {
        if (service) {
            char *service_id = JSON_GetStringFromObject(service, SERVICE_ID, NULL);
            prop_set->services[i].service_id = service_id;

            JSON *properties = JSON_GetObjectFromObject(service, PROPERTIES);
            prop[i] = cJSON_Print(properties);
            prop_set->services[i].properties = prop[i];
            if (strcmp(service_id, DEVICE_RULE) == 0) {
                RuleTrans_DeviceRuleUpdate(prop[i]);
            } else if (strcmp(service_id, SECURITY_DETECTION_CONFIG) == 0) {
                Detect_ParseShadowGetOrPropertiesSet(prop[i]);
            }
        }
        i++;
        services_count--;
    }
    prop_set->services_count = i;

    if (onPropertiesSet) {
        (onPropertiesSet)(prop_set);
    }
#if defined(SOFT_BUS_OPTION2)
    if (strcmp(prop_set->services[0].service_id, SOFT_BUS_SERVICEID) == 0) {
        usleep(5 * 1000);
        ST_IOTA_SERVICE_DATA_INFO service_data[1];
        service_data[0].properties = prop[0];
        service_data[0].service_id = SOFT_BUS_SERVICEID;
        service_data[0].event_time = NULL;
        int messageId = IOTA_PropertiesReport(service_data, 1, 0, NULL);
        if (messageId != 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "callback_func: report shadow value failed, the result is %d\n", messageId);
        }
        usleep(5 * 1000);
    }
#endif
    JSON_Delete(root);
    int j;
    for (j = 0; j < i; j++) {
        MemFree(&prop[j]);
    }

    MemFree(&prop_set->services);
    MemFree(&prop_set->mqtt_msg_info);
    MemFree(&prop_set);
    return;
}

static void OnPropertiesGetArrived(void *context, int token, int code, char *bridgeDeviceId, const char *topic, const char *message)
{
    char *requestId_value = strstr(topic, "=");
    char *request_id = strstr(requestId_value + 1, "");

    EN_IOTA_PROPERTY_GET *prop_get = (EN_IOTA_PROPERTY_GET *)malloc(sizeof(EN_IOTA_PROPERTY_GET));
    if (prop_get == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        return;
    }

    prop_get->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (prop_get->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        MemFree(&prop_get);
        return;
    }
    prop_get->mqtt_msg_info->context = context;
    prop_get->mqtt_msg_info->messageId = token;
    prop_get->mqtt_msg_info->code = code;
    prop_get->bridge_device_id = bridgeDeviceId;
    prop_get->request_id = request_id;

    JSON *root = JSON_Parse(message);

    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");
    prop_get->object_device_id = object_device_id;

    char *service_id = JSON_GetStringFromObject(root, SERVICE_ID, "-1");
    prop_get->service_id = service_id;

    if (onPropertiesGet) {
        (onPropertiesGet)(prop_get);
    }

    JSON_Delete(root);
    MemFree(&prop_get->mqtt_msg_info);
    MemFree(&prop_get);
    return;
}

static void OnShadowGetResponseArrived(void *context, int token, int code, char *bridgeDeviceId, const char *topic, char *message)
{
    char *requestId_value = strstr(topic, "=");
    char *request_id = strstr(requestId_value + 1, "");

    EN_IOTA_DEVICE_SHADOW *device_shadow = (EN_IOTA_DEVICE_SHADOW *)malloc(sizeof(EN_IOTA_DEVICE_SHADOW));
    if (device_shadow == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        return;
    }

    device_shadow->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (device_shadow->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        MemFree(&device_shadow);
        return;
    }
    device_shadow->mqtt_msg_info->context = context;
    device_shadow->mqtt_msg_info->messageId = token;
    device_shadow->mqtt_msg_info->code = code;
    device_shadow->bridge_device_id = bridgeDeviceId;
    device_shadow->request_id = request_id;

    JSON *root = JSON_Parse(message);

    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");
    device_shadow->object_device_id = object_device_id;

    JSON *shadow = JSON_GetObjectFromObject(root, SHADOW);
    int shadow_data_count = 0;
    device_shadow->shadow_data_count = 0;
    if (shadow != NULL) {
        shadow_data_count = JSON_GetArraySize(shadow);
    }

    if (shadow_data_count >= MAX_SERVICE_COUNT) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the number of shadow service exceeds.\n");
        JSON_Delete(root);
        MemFree(&device_shadow->mqtt_msg_info);
        MemFree(&device_shadow);
        return;
    }

    device_shadow->shadow = (EN_IOTA_SHADOW_DATA *)malloc(sizeof(EN_IOTA_SHADOW_DATA) * shadow_data_count);
    if (device_shadow->shadow == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        JSON_Delete(root);
        MemFree(&device_shadow->mqtt_msg_info);
        MemFree(&device_shadow);
        return;
    }
    (void)memset_s(device_shadow->shadow, sizeof(EN_IOTA_SHADOW_DATA) * shadow_data_count, 0,
        sizeof(EN_IOTA_SHADOW_DATA) * shadow_data_count);
    int i = 0;
    char *desired_str[MAX_SERVICE_COUNT] = {0};
    char *reported_str[MAX_SERVICE_COUNT] = {0};
    JSON *shadow_data = NULL;
    cJSON_ArrayForEach(shadow_data, shadow) {
        if (shadow_data) {
            char *service_id = JSON_GetStringFromObject(shadow_data, SERVICE_ID, "-1");
            device_shadow->shadow[i].service_id = service_id;

            JSON *desired = JSON_GetObjectFromObject(shadow_data, DESIRED);
            if (desired) {
                char *desired_event_time = JSON_GetStringFromObject(desired, EVENT_TIME, "-1");
                device_shadow->shadow[i].desired_event_time = desired_event_time;
                JSON *desired_properties = JSON_GetObjectFromObject(desired, PROPERTIES);
                desired_str[i] = cJSON_Print(desired_properties);
                device_shadow->shadow[i].desired_properties = desired_str[i];
            }

            JSON *reported = JSON_GetObjectFromObject(shadow_data, REPORTED);
            if (reported) {
                char *reported_event_time = JSON_GetStringFromObject(reported, EVENT_TIME, "-1");
                device_shadow->shadow[i].desired_event_time = reported_event_time;
                JSON *reported_properties = JSON_GetObjectFromObject(reported, PROPERTIES);
                reported_str[i] = cJSON_Print(reported_properties);
                device_shadow->shadow[i].reported_properties = reported_str[i];
            }

            int version = JSON_GetIntFromObject(shadow_data, VERSION, -1);
            device_shadow->shadow[i].version = version;
        }
        if (strcmp(device_shadow->shadow[i].service_id, DEVICE_RULE) == 0) {
            RuleTrans_DeviceRuleUpdate(desired_str[i]);
            MemFree(&desired_str[i]);
            MemFree(&reported_str[i]);
        } else if (strcmp(device_shadow->shadow[i].service_id, SECURITY_DETECTION_CONFIG) == 0) {
            Detect_ParseShadowGetOrPropertiesSet(desired_str[i]);
            MemFree(&desired_str[i]);
            MemFree(&reported_str[i]);
        } else {
            i++;
        }
        shadow_data_count--;
    }
    device_shadow->shadow_data_count = i;

    if (onDeviceShadow) {
        (onDeviceShadow)(device_shadow);
    }
    JSON_Delete(root);
    int j;
    for (j = 0; j < i; j++) {
        MemFree(&desired_str[j]);
        MemFree(&reported_str[j]);
    }

    MemFree(&device_shadow->shadow);
    MemFree(&device_shadow->mqtt_msg_info);
    MemFree(&device_shadow);
    return;
}

static void OnUserTpoicRawMessage(void *context, int token, int code, char *topicParas, char *message,
    int messageLength)
{
    EN_IOTA_USER_TOPIC_RAW_MESSAGE *rawMsg =
        (EN_IOTA_USER_TOPIC_RAW_MESSAGE *)malloc(sizeof(EN_IOTA_USER_TOPIC_RAW_MESSAGE));
    if (rawMsg == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnUserTopicArrived(): there is not enough memory here.\n");
        return;
    }
    (void)memset_s(rawMsg, sizeof(EN_IOTA_USER_TOPIC_RAW_MESSAGE), 0, sizeof(EN_IOTA_USER_TOPIC_RAW_MESSAGE));

    rawMsg->mqttMsgInfo = CreateMqttMsgInfo(context, token, code);
    if (rawMsg->mqttMsgInfo == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnUserTopicArrived(): there is not enough memory here.\n");
        goto EXIT;
    }

    rawMsg->payloadLength = messageLength;
    rawMsg->payload = message;
    rawMsg->topicPara = topicParas;
    if (onUserTopicRawMessage) {
        (onUserTopicRawMessage)(rawMsg);
    }
EXIT:
    MemFree(&rawMsg->mqttMsgInfo);
    MemFree(&rawMsg);
}

static void OnBridgesLoginOrLogoutArrived(void *context, int token, int code, char *bridgeDeviceId, char *topic, char *message, int isLogin)
{
    char *requestId_value = strstr(topic, "=");
    char *request_id = strstr(requestId_value + 1, "");

    EN_IOTA_BRIDGES_LOGIN *log = (EN_IOTA_BRIDGES_LOGIN *)malloc(sizeof(EN_IOTA_BRIDGES_LOGIN));
    if (log == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnBridgesLoginOrLogoutArrived(): there is not enough memory here.\n");
        return;
    }
    
    log->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (log->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        MemFree(&log);
        return;
    }
    log->mqtt_msg_info->context = context;
    log->mqtt_msg_info->messageId = token;
    log->mqtt_msg_info->code = code;
    log->bridge_device_id = bridgeDeviceId;

    log->request_id = request_id;

    JSON *root = JSON_Parse(message);
    log->result_code = JSON_GetIntFromObject(root, RESULT_CODE, -1);

    if (isLogin && onBridgesDeviceLogin) {
        (onBridgesDeviceLogin)(log);
    }
    if (!isLogin && onBridgesDeviceLogout) {
        (onBridgesDeviceLogout)(log);
    }
    cJSON_Delete(root);
    MemFree(&log->mqtt_msg_info);
    MemFree(&log);
}

static void OnBridgesResetSecretArrived(void *context, int token, int code, char *bridgeDeviceId, char *topic, char *message)
{
    char *requestId_value = strstr(topic, "=");
    char *request_id = strstr(requestId_value + 1, "");

    EN_IOTA_BRIDGES_RESET_SECRET *resetSecret = (EN_IOTA_BRIDGES_RESET_SECRET *)malloc(sizeof(EN_IOTA_BRIDGES_RESET_SECRET));
    if (resetSecret == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnBridgesResetSecretArrived(): there is not enough memory here.\n");
        return;
    }
    
    resetSecret->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (resetSecret->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        MemFree(&resetSecret);
        return;
    }
    resetSecret->mqtt_msg_info->context = context;
    resetSecret->mqtt_msg_info->messageId = token;
    resetSecret->mqtt_msg_info->code = code;
    resetSecret->bridge_device_id = bridgeDeviceId;
    resetSecret->request_id = request_id;

    JSON *root = JSON_Parse(message);
    resetSecret->result_code = JSON_GetIntFromObject(root, RESULT_CODE, -1);

    JSON *paras = JSON_GetObjectFromObject(root, PARAS);
    resetSecret->paras = cJSON_Print(paras);
    resetSecret->new_secret = JSON_GetStringFromObject(root, NEW_SECRET, "-1");
    
    if (onBridgesDeviceResetSecret) {
        (onBridgesDeviceResetSecret)(resetSecret);
    }

    cJSON_Delete(root);
    MemFree(&resetSecret->mqtt_msg_info);
    MemFree(&resetSecret);
}

static void OnBridgesDeviceDisconnectArrived(void *context, int token, int code, char *bridgeDeviceId, char *message)
{

    EN_IOTA_BRIDGES_PALLET_DISCONNECT *disConnect = (EN_IOTA_BRIDGES_PALLET_DISCONNECT *)malloc(sizeof(EN_IOTA_BRIDGES_PALLET_DISCONNECT));
    if (disConnect == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnBridgesDeviceDisconnectArrived(): there is not enough memory here.\n");
        return;
    }
    
    disConnect->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (disConnect->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        MemFree(&disConnect);
        return;
    }
    disConnect->mqtt_msg_info->context = context;
    disConnect->mqtt_msg_info->messageId = token;
    disConnect->mqtt_msg_info->code = code;
    disConnect->bridge_device_id = bridgeDeviceId;

    if (onBridgesDeviceDisconnect) {
        (onBridgesDeviceDisconnect)(disConnect);
    }

    MemFree(&disConnect->mqtt_msg_info);
    MemFree(&disConnect);
}

static void OnUserTopicArrived(void *context, int token, int code, char *topic, char *message, int messageLength)
{
    char *topicParasValue = strstr(topic, "user/");
    char *topicParas = strstr(topicParasValue + 5, "");
    OnUserTpoicRawMessage(context, token, code, topicParas, message, messageLength);

    cJSON *root = cJSON_Parse(message);
    EN_IOTA_USER_TOPIC_MESSAGE *userTopicMsg = (EN_IOTA_USER_TOPIC_MESSAGE *)malloc(sizeof(EN_IOTA_USER_TOPIC_MESSAGE));
    if (userTopicMsg == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnUserTopicArrived(): there is not enough memory here.\n");
        goto EXIT;
    }
    userTopicMsg->mqtt_msg_info = CreateMqttMsgInfo(context, token, code);
    if (userTopicMsg->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnUserTopicArrived(): there is not enough memory here.\n");
        goto EXIT;
    }

    if (!GetMessageInSystemFormat(root, &userTopicMsg->object_device_id, &userTopicMsg->name, &userTopicMsg->id,
        &userTopicMsg->content)) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "OnUserTopicArrived(): message is not system fomart.\n");
        goto EXIT;
    }

    userTopicMsg->topic_para = topicParas;

    if (onUserTopicMessage) {
        (onUserTopicMessage)(userTopicMsg);
    }
EXIT:
    cJSON_Delete(root);
    if (userTopicMsg) {
        MemFree(&userTopicMsg->mqtt_msg_info);
    }
    MemFree(&userTopicMsg);
    return;
}

// if it is the platform inform the gateway to add or delete the sub device
static int OnEventsGatewayAddOrDelete(int i, char *event_type, EN_IOTA_EVENT *event, JSON *paras, const char *message)
{
    event->services[i].paras = (EN_IOTA_DEVICE_PARAS *)malloc(sizeof(EN_IOTA_DEVICE_PARAS));
    if (event->services[i].paras == NULL) {
        return -1;
    }

    JSON *devices = JSON_GetObjectFromObject(paras, DEVICES);

    int devices_count = JSON_GetArraySize(devices);
    if (devices_count > MAX_EVENT_DEVICE_COUNT) {
        // you can increase the  MAX_EVENT_DEVICE_COUNT in iota_init.h
        PrintfLog(EN_LOG_LEVEL_ERROR, "messageArrivaled: OnEventsGatewayAddOrDelete(), devices_count is too large.\n");
        return 0;
    }

    event->services[i].paras->devices = (EN_IOTA_DEVICE_INFO *)malloc(sizeof(EN_IOTA_DEVICE_INFO) * devices_count);
    if (event->services[i].paras->devices == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsGatewayAddOrDelete(): there is not enough memory here.\n");
        int k = i;
        while (k >= 0) {
            MemFree(&event->services[k].paras);
            k--;
        }
        return -1;
    }

    event->services[i].paras->devices_count = devices_count;

    long long version = getLLongValueFromStr(message, VERSION_JSON);
    if (version < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "getLLongValueFromStr(), Length out of bounds. Modifiable value LONG_LONG_MAX_LENGTH\n");
        return -1;
    }

    event->services[i].paras->version = version;
    int j = 0;

    // adding a sub device notify
    if (!strcmp(event_type, ADD_SUB_DEVICE_NOTIFY)) {
        event->services[i].event_type = EN_IOTA_EVENT_ADD_SUB_DEVICE_NOTIFY;
        JSON *deviceInfo = NULL;
        cJSON_ArrayForEach(deviceInfo, devices) {
            char *parent_device_id = JSON_GetStringFromObject(deviceInfo, PARENT_DEVICE_ID, NULL);
            event->services[i].paras->devices[j].parent_device_id = parent_device_id;

            char *node_id = JSON_GetStringFromObject(deviceInfo, NODE_ID, NULL);
            event->services[i].paras->devices[j].node_id = node_id;

            char *device_id = JSON_GetStringFromObject(deviceInfo, DEVICE_ID, NULL);
            event->services[i].paras->devices[j].device_id = device_id;

            char *name = JSON_GetStringFromObject(deviceInfo, NAME, NULL);
            event->services[i].paras->devices[j].name = name;

            char *description = JSON_GetStringFromObject(deviceInfo, DESCRIPTION, NULL);
            event->services[i].paras->devices[j].description = description;

            char *manufacturer_id = JSON_GetStringFromObject(deviceInfo, MANUFACTURER_ID, NULL);
            event->services[i].paras->devices[j].manufacturer_id = manufacturer_id;

            char *model = JSON_GetStringFromObject(deviceInfo, MODEL, NULL);
            event->services[i].paras->devices[j].model = model;

            char *product_id = JSON_GetStringFromObject(deviceInfo, PRODUCT_ID, NULL);
            event->services[i].paras->devices[j].product_id = product_id;

            char *fw_version = JSON_GetStringFromObject(deviceInfo, FW_VERSION, NULL);
            event->services[i].paras->devices[j].fw_version = fw_version;

            char *sw_version = JSON_GetStringFromObject(deviceInfo, SW_VERSION, NULL);
            event->services[i].paras->devices[j].sw_version = sw_version;

            char *status = JSON_GetStringFromObject(deviceInfo, STATUS, NULL);
            event->services[i].paras->devices[j].status = status;

            char *extension_info = JSON_GetStringFromObject(deviceInfo, EXTENSION_INFO, NULL);
            event->services[i].paras->devices[j].extension_info = extension_info;

            j++;
            devices_count--;
        }
    }

    // deleting a sub device notify
    if (!strcmp(event_type, DELETE_SUB_DEVICE_NOTIFY)) {
        event->services[i].event_type = EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY;
        JSON *deviceInfo = NULL;
        cJSON_ArrayForEach(deviceInfo, devices) {
            char *parent_device_id = JSON_GetStringFromObject(deviceInfo, PARENT_DEVICE_ID, NULL);
            event->services[i].paras->devices[j].parent_device_id = parent_device_id;

            char *node_id = JSON_GetStringFromObject(deviceInfo, NODE_ID, NULL);
            event->services[i].paras->devices[j].node_id = node_id;

            char *device_id = JSON_GetStringFromObject(deviceInfo, DEVICE_ID, NULL);
            event->services[i].paras->devices[j].device_id = device_id;

            j++;
            devices_count--;
        }
    }
    return 1;
}

static int OnEventsAddSubDeviceResponse(int i, EN_IOTA_EVENT *event, JSON *paras, JSON *serviceEvent)
{
    char *event_id = JSON_GetStringFromObject(serviceEvent, EVENT_ID, NULL);
    event->services[i].event_id = event_id;
    event->services[i].event_type = EN_IOTA_EVENT_ADD_SUB_DEVICE_RESPONSE;

    event->services[i].gtw_add_device_paras =
        (EN_IOTA_GTW_ADD_DEVICE_PARAS *)malloc(sizeof(EN_IOTA_GTW_ADD_DEVICE_PARAS));
    if (event->services[i].gtw_add_device_paras == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsAddSubDeviceResponse(): there is not enough memory here.\n");
        return -1;
    }

    JSON *successful_devices = JSON_GetObjectFromObject(paras, SUCCESSFUL_DEVICES);
    int successful_devices_count = JSON_GetArraySize(successful_devices);
    event->services[i].gtw_add_device_paras->successful_devices_count = successful_devices_count;

    event->services[i].gtw_add_device_paras->successful_devices =
        (EN_IOTA_DEVICE_INFO *)malloc(sizeof(EN_IOTA_DEVICE_INFO) * successful_devices_count);
    if (event->services[i].gtw_add_device_paras->successful_devices == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsAddSubDeviceResponse(): "
            "successful_devices is not enough memory here.\n");
        int k = i; 
        while (k >= 0) {
            MemFree(&event->services[k].gtw_add_device_paras);
            k--;
        }
        return -1;
    }

    int j = 0;
    JSON *deviceInfo = NULL;
    cJSON_ArrayForEach(deviceInfo, successful_devices) {
        char *parent_device_id = JSON_GetStringFromObject(deviceInfo, PARENT_DEVICE_ID, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].parent_device_id = parent_device_id;

        char *node_id = JSON_GetStringFromObject(deviceInfo, NODE_ID, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].node_id = node_id;

        char *device_id = JSON_GetStringFromObject(deviceInfo, DEVICE_ID, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].device_id = device_id;

        char *name = JSON_GetStringFromObject(deviceInfo, NAME, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].name = name;

        char *description = JSON_GetStringFromObject(deviceInfo, DESCRIPTION, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].description = description;

        char *manufacturer_id = JSON_GetStringFromObject(deviceInfo, MANUFACTURER_ID, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].manufacturer_id = manufacturer_id;

        char *model = JSON_GetStringFromObject(deviceInfo, MODEL, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].model = model;

        char *product_id = JSON_GetStringFromObject(deviceInfo, PRODUCT_ID, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].product_id = product_id;

        char *fw_version = JSON_GetStringFromObject(deviceInfo, FW_VERSION, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].fw_version = fw_version;

        char *sw_version = JSON_GetStringFromObject(deviceInfo, SW_VERSION, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].sw_version = sw_version;

        char *status = JSON_GetStringFromObject(deviceInfo, STATUS, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].status = status;

        char *extension_info = JSON_GetStringFromObject(deviceInfo, EXTENSION_INFO, NULL);
        event->services[i].gtw_add_device_paras->successful_devices[j].extension_info = extension_info;

        j++;
        successful_devices_count--;
    }

    JSON *failed_devices = JSON_GetObjectFromObject(paras, FAILED_DEVICES);
    int failed_devices_count = JSON_GetArraySize(failed_devices);

    event->services[i].gtw_add_device_paras->failed_devices_count = failed_devices_count;

    event->services[i].gtw_add_device_paras->failed_devices =
        (EN_IOTA_ADD_DEVICE_FAILED_REASON *)malloc(sizeof(EN_IOTA_ADD_DEVICE_FAILED_REASON) * failed_devices_count);
    if (event->services[i].gtw_add_device_paras->failed_devices == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsAddSubDeviceResponse(): failed_devices is not enough memory here.\n");
        return -1;
    }

    int k = 0;
    JSON *reason = NULL;
    cJSON_ArrayForEach(reason, failed_devices) {
        char *node_id = JSON_GetStringFromObject(reason, NODE_ID, NULL);
        event->services[i].gtw_add_device_paras->failed_devices[k].node_id = node_id;

        char *product_id = JSON_GetStringFromObject(reason, PRODUCT_ID, NULL);
        event->services[i].gtw_add_device_paras->failed_devices[k].product_id = product_id;

        char *error_code = JSON_GetStringFromObject(reason, ERROR_CODE, NULL);
        event->services[i].gtw_add_device_paras->failed_devices[k].error_code = error_code;

        char *error_msg = JSON_GetStringFromObject(reason, ERROR_MSG, NULL);
        event->services[i].gtw_add_device_paras->failed_devices[k].error_msg = error_msg;

        k++;
        failed_devices_count--;
    }
    return 1;
}

static int OnEventsDeleteSubDeviceResponse(int i, EN_IOTA_EVENT *event, JSON *paras, JSON *serviceEvent)
{
    char *event_id = JSON_GetStringFromObject(serviceEvent, EVENT_ID, NULL);
    event->services[i].event_id = event_id;
    event->services[i].event_type = EN_IOTA_EVENT_DEL_SUB_DEVICE_RESPONSE;

    event->services[i].gtw_del_device_paras =
        (EN_IOTA_GTW_DEL_DEVICE_PARAS *)malloc(sizeof(EN_IOTA_GTW_DEL_DEVICE_PARAS));
    if (event->services[i].gtw_del_device_paras == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDeleteSubDeviceResponse(): there is not enough memory here.\n");
        return -1;
    }

    JSON *successful_devices = JSON_GetObjectFromObject(paras, SUCCESSFUL_DEVICES);
    int successful_devices_count = JSON_GetArraySize(successful_devices);
    event->services[i].gtw_del_device_paras->successful_devices_count = successful_devices_count;

    event->services[i].gtw_del_device_paras->successful_devices =
        (HW_CHAR **)malloc(sizeof(HW_CHAR) * successful_devices_count);
    if (event->services[i].gtw_del_device_paras->successful_devices == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDeleteSubDeviceResponse(): "
            "successful_devices is not enough memory here.\n");
        return -1;
    }

    int j = 0;
    while (successful_devices_count > 0) {
        char *device_id = JSON_GetStringFromArray(successful_devices, j, NULL);
        event->services[i].gtw_del_device_paras->successful_devices[j] = device_id;
        j++;
        successful_devices_count--;
    }

    JSON *failed_devices = JSON_GetObjectFromObject(paras, FAILED_DEVICES);
    int failed_devices_count = JSON_GetArraySize(failed_devices);
    event->services[i].gtw_del_device_paras->failed_devices_count = failed_devices_count;

    event->services[i].gtw_del_device_paras->failed_devices =
        (EN_IOTA_DEL_DEVICE_FAILED_REASON *)malloc(sizeof(EN_IOTA_DEL_DEVICE_FAILED_REASON) * failed_devices_count);
    if (event->services[i].gtw_del_device_paras->failed_devices == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDeleteSubDeviceResponse(): this is not enough memory here.\n");
        return -1;
    }
    int k = 0;
    JSON *reason = NULL;
    cJSON_ArrayForEach(reason, failed_devices) {
        char *device_id = JSON_GetStringFromObject(reason, DEVICE_ID, NULL);
        event->services[i].gtw_del_device_paras->failed_devices[k].device_id = device_id;

        char *error_code = JSON_GetStringFromObject(reason, ERROR_CODE, NULL);
        event->services[i].gtw_del_device_paras->failed_devices[k].error_code = error_code;

        char *error_msg = JSON_GetStringFromObject(reason, ERROR_MSG, NULL);
        event->services[i].gtw_del_device_paras->failed_devices[k].error_msg = error_msg;
        k++;
        failed_devices_count--;
    }
    return 1;
}

static int OnEventsDownManagerArrived(int i, char *event_type, EN_IOTA_EVENT *event,
    JSON *paras, const char *message, JSON *serviceEvent)
{
    // if it is the platform inform the gateway to add or delete the sub device
    if ((!strcmp(event_type, DELETE_SUB_DEVICE_NOTIFY)) || (!strcmp(event_type, ADD_SUB_DEVICE_NOTIFY))) {
        int ret = OnEventsGatewayAddOrDelete(i, event_type, event, paras, message);
        return ret;
    }

    // the response of gateway adding a sub device
    if (!strcmp(event_type, ADD_SUB_DEVICE_RESPONSE)) {
        int ret = OnEventsAddSubDeviceResponse(i, event, paras, serviceEvent);
        return ret;
    }

    // the response of gateway deleting a sub device
    if (!strcmp(event_type, DEL_SUB_DEVICE_RESPONSE)) {
        int ret = OnEventsDeleteSubDeviceResponse(i, event, paras, serviceEvent);
        return ret;
    }

    //  the update status of gateway deleting a sub device
    if (!strcmp(event_type, UPDATE_SUB_DEVICE_RESPONSE)) {
        int ret = OnEventsDeleteSubDeviceResponse(i, event, paras, serviceEvent);
        return ret;
    }
    return 1;
}

static int OnEventsDownFileManager(EN_IOTA_SERVICE_EVENT *services, char *event_type, JSON *paras)
{
    if (!strcmp(event_type, GET_UPLOAD_URL_RESPONSE)) {
        services->event_type = EN_IOTA_EVENT_GET_UPLOAD_URL_RESPONSE;
    } else if (!strcmp(event_type, GET_DOWNLOAD_URL_RESPONSE)) {
        services->event_type = EN_IOTA_EVENT_GET_DOWNLOAD_URL_RESPONSE;
    }
    services->file_mgr_paras = (EN_IOTA_OTA_PARAS *)malloc(sizeof(EN_IOTA_FILE_MGR_PARAS));
    if (services->file_mgr_paras == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDownFileManager(): there is not enough memory here.\n");
        return -1;
    }
    services->file_mgr_paras->url = JSON_GetStringFromObject(paras, URL, NULL);
    return 1;
}

static int OnEventsDownOtaArrived(EN_IOTA_SERVICE_EVENT *services, char *event_type, JSON *paras)
{
    services->ota_paras = (EN_IOTA_OTA_PARAS *)malloc(sizeof(EN_IOTA_OTA_PARAS));
    if (services->ota_paras == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDownOtaArrived(): there is not enough memory here.\n");
        return -1;
    }

    if (!strcmp(event_type, VERSION_QUERY)) {
        services->event_type = EN_IOTA_EVENT_VERSION_QUERY;
        
    } else if (!strcmp(event_type, FIRMWARE_UPGRADE)) {
        services->event_type = EN_IOTA_EVENT_FIRMWARE_UPGRADE;
    } else if (!strcmp(event_type, SOFTWARE_UPGRADE)) {
        services->event_type = EN_IOTA_EVENT_SOFTWARE_UPGRADE;
    } else if (!strcmp(event_type, FIRMWARE_UPGRADE_V2)) {
        services->event_type = EN_IOTA_EVENT_FIRMWARE_UPGRADE_V2;
    } else if (!strcmp(event_type, SOFTWARE_UPGRADE_V2)) {
        services->event_type = EN_IOTA_EVENT_SOFTWARE_UPGRADE_V2;
    }

    // firmware_upgrade or software_upgrade
    if ((services->event_type == EN_IOTA_EVENT_FIRMWARE_UPGRADE) ||
        (services->event_type == EN_IOTA_EVENT_SOFTWARE_UPGRADE) ||
        (services->event_type == EN_IOTA_EVENT_FIRMWARE_UPGRADE_V2) ||
        (services->event_type == EN_IOTA_EVENT_SOFTWARE_UPGRADE_V2)) {
        char *version = JSON_GetStringFromObject(paras, VERSION, NULL);
        services->ota_paras->version = version;

        char *url = JSON_GetStringFromObject(paras, URL, NULL);
        services->ota_paras->url = url;

        int file_size = JSON_GetIntFromObject(paras, FILE_SIZE, -1);
        services->ota_paras->file_size = file_size;

        char *file_name = JSON_GetStringFromObject(paras, FILE_NAME, NULL);
        services->ota_paras->file_name = file_name;

        char *task_id = JSON_GetStringFromObject(paras, TASK_ID, NULL);
        services->ota_paras->task_id = task_id;
        
        int sub_device_count = JSON_GetIntFromObject(paras, SUBDEVICE_COUNT, -1);
        services->ota_paras->sub_device_count = sub_device_count;

        char *task_ext_info = JSON_GetStringFromObject(paras, TASKEXT_INFO, NULL);
        services->ota_paras->task_ext_info = task_ext_info;

        char *access_token = JSON_GetStringFromObject(paras, ACCESS_TOKEN, NULL);
        services->ota_paras->access_token = access_token;

        int expires = JSON_GetIntFromObject(paras, EXPIRES, -1);
        services->ota_paras->expires = expires;

        char *sign = JSON_GetStringFromObject(paras, SIGN, NULL);
        services->ota_paras->sign = sign;
    }
    return 1;
}

static int OnEventsDownNtpArrived(EN_IOTA_SERVICE_EVENT *services, char *event_type, const char *message)
{
    services->ntp_paras = (EN_IOTA_NTP_PARAS *)malloc(sizeof(EN_IOTA_NTP_PARAS));
    if (services->ntp_paras == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDownNtpArrived(): ntp_paras is not enough memory here.\n");
        return -1;
    }

    if (!strcmp(event_type, TIME_SYNC_RSP)) {
        services->event_type = EN_IOTA_EVENT_GET_TIME_SYNC_RESPONSE;

        long long device_send_time = getLLongValueFromStr(message, DEVICE_SEND_TIME_JSON);
        long long server_recv_time = getLLongValueFromStr(message, SERVER_RECV_TIME_JSON);
        long long server_send_time = getLLongValueFromStr(message, SERVER_SEND_TIME_JSON);
        if ((device_send_time < 0) || (server_recv_time < 0) || (server_send_time < 0)) {
            PrintfLog(EN_LOG_LEVEL_ERROR,
                "getLLongValueFromStr(), Length out of bounds. Modifiable value LONG_LONG_MAX_LENGTH\n");
            return -1;
        }
        services->ntp_paras->device_real_time =
            (server_recv_time + server_send_time + (HW_LLONG)getTime() - device_send_time) / 2;
    }
    return 1;
}

static int OnEventsDownLogArrived(EN_IOTA_SERVICE_EVENT *services, char *event_type, JSON *paras)
{
    services->device_log_paras = (EN_IOTA_DEVICE_LOG_PARAS *)malloc(sizeof(EN_IOTA_DEVICE_LOG_PARAS));
    if (services->device_log_paras == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDownLogArrived(): is not enough memory here.\n");
        return -1;
    }

    if (!strcmp(event_type, LOG_CONFIG)) {
        services->event_type = EN_IOTA_EVENT_LOG_CONFIG;

        char *log_switch = JSON_GetStringFromObject(paras, SWITCH, NULL);
        services->device_log_paras->log_switch = log_switch;

        char *end_time = JSON_GetStringFromObject(paras, END_TIME, NULL);
        services->device_log_paras->end_time = end_time;
    }
    return 1;
}

static int OnEventsDownSoftBus(EN_IOTA_SERVICE_EVENT *services, char *event_type, JSON *paras)
{
    services->soft_bus_paras = (EN_IOTA_SOFT_BUS_PARAS *)malloc(sizeof(EN_IOTA_SOFT_BUS_PARAS));
    if (services->soft_bus_paras == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDownSoftBus(): is not enough memory here.\n");
        return -1;
    }
    services->soft_bus_paras->bus_infos = cJSON_Print(paras);

#if defined(SOFT_BUS_OPTION2)
    // 获取g_soft_bus_total地址
    soft_bus_total *softBusTotal = getSoftBusTotal();
    cJSON *busInfos = cJSON_GetObjectItem(paras, BUS_INFOS);
    int  i = 0, j = 0;
    JSON *busInfosEvent = NULL;
    JSON *deviceInfoEvent = NULL;
    cJSON_ArrayForEach(busInfosEvent, busInfos) {
        cJSON *deviceInfo = cJSON_GetObjectItem(busInfosEvent, DEVICES_INFO);
        char *busKey = JSON_GetStringFromObject(busInfosEvent, BUS_KEY, "-1");
        soft_bus_infos *softBusInfos = &softBusTotal->g_soft_bus_info[i];
        cJSON_ArrayForEach(deviceInfoEvent, deviceInfo) {
            char *deviceId = JSON_GetStringFromObject(deviceInfoEvent, DEVICE_ID, "-1");
            char *deviceIp = JSON_GetStringFromObject(deviceInfoEvent, DEVICE_IP, "-1");
            if (strcmp(deviceId, "-1") == 0 && strcmp(deviceIp, "-1") == 0) {
                continue;
            }
            soft_bus_info *softBusInfo = &softBusInfos->g_device_soft_bus_info[j];
            MemFree(&softBusInfo->device_id);
            MemFree(&softBusInfo->device_ip);
            softBusInfo->device_id = CombineStrings(1, deviceId);
            softBusInfo->device_ip = CombineStrings(1, deviceIp);
            j++;
            if (j >= SOFTBUS_INFO_LEN) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "the soft_bus_info Exceeding the limit value");
                break;
            }
        }
        softBusInfos->count = j;
        i++;
        if (i >= SOFTBUS_TOTAL_LEN) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "the soft_bus_infos Exceeding the limit value");
            break;
        }
        MemFree(&softBusInfos->bus_key);
        softBusInfos->bus_key = CombineStrings(1, busKey);
    }
    softBusTotal->count = i;
#endif
    return 1;
}

static int OnEventsDownTunnelArrived(EN_IOTA_SERVICE_EVENT *services, const char *event_type, JSON *paras)
{
    services->tunnel_mgr_paras = (EN_IOTA_TUNNEL_MGR_PARAS *)malloc(sizeof(EN_IOTA_TUNNEL_MGR_PARAS));
    if (services->tunnel_mgr_paras == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDownTunnelArrived(): is not enough memory here.\n");
        return -1;
    }

    if (!strcmp(event_type, TUNNEL_NTF)) {
        services->event_type = EN_IOTA_EVENT_TUNNEL_NOTIFY;

        char *tunnel_url = JSON_GetStringFromObject(paras, TUNNEL_URL, NULL);
        services->tunnel_mgr_paras->tunnel_url = tunnel_url;

        char *tunnel_access_token = JSON_GetStringFromObject(paras, TUNNEL_ACCESS_TOKEN, NULL);
        services->tunnel_mgr_paras->tunnel_access_token = tunnel_access_token;
    }
    return 1;
}

static int OnEventsDownRemoteCfgArrived(EN_IOTA_SERVICE_EVENT *services, const char *event_type,
    JSON *paras, char *object_device_id)
{
    ST_IOTA_DEVICE_CONFIG_RESULT deviceCfgRpt = {0};
    JSON *cfg = NULL;
    if (!strcmp(event_type, DEVICE_CONFIG_UPDATE)) {
        services->event_type = EN_IOTA_EVENT_DEVICE_CONFIG_UPDATE;
        cfg = JSON_GetObjectFromObject(paras, DEVICE_CONFIG_CONTENT);
        if (cfg == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDownRemoteCfgArrived(): paras parse failed.\n");
            return -1;
        }
        deviceCfgRpt.object_device_id = object_device_id;
        if (onDeviceConfig) {
            deviceCfgRpt.result_code = onDeviceConfig(cfg, deviceCfgRpt.description);
        }
        IOTA_RptDeviceConfigRst(&deviceCfgRpt, NULL);
    }
    return 1;
}

static void OnEventsDownMemFree(EN_IOTA_EVENT *event, int services_count)
{
    int m;
    for (m = 0; m < services_count; m++) {
        if (event->services[m].servie_id == EN_IOTA_EVENT_SUB_DEVICE_MANAGER) {
            if ((event->services[m].event_type == EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY) ||
                (event->services[m].event_type == EN_IOTA_EVENT_ADD_SUB_DEVICE_NOTIFY)) {
                MemFree(&event->services[m].paras->devices);
                MemFree(&event->services[m].paras);
            } else if (event->services[m].event_type == EN_IOTA_EVENT_ADD_SUB_DEVICE_RESPONSE) {
                MemFree(&event->services[m].gtw_add_device_paras->successful_devices);
                MemFree(&event->services[m].gtw_add_device_paras->failed_devices);
                MemFree(&event->services[m].gtw_add_device_paras);
            } else if (event->services[m].event_type == EN_IOTA_EVENT_DEL_SUB_DEVICE_RESPONSE) {
                MemFree(&event->services[m].gtw_del_device_paras->successful_devices);
                MemFree(&event->services[m].gtw_del_device_paras->failed_devices);
                MemFree(&event->services[m].gtw_del_device_paras);
            }
        } else if (event->services[m].servie_id == EN_IOTA_EVENT_OTA) {
            MemFree(&event->services[m].ota_paras);
        } else if (event->services[m].servie_id == EN_IOTA_EVENT_TIME_SYNC) {
            MemFree(&event->services[m].ntp_paras);
        } else if (event->services[m].servie_id == EN_IOTA_EVENT_DEVICE_LOG) {
            MemFree(&event->services[m].device_log_paras);
        } else if (event->services[m].servie_id == EN_IOTA_EVENT_TUNNEL_MANAGER) {
            MemFree(&event->services[m].tunnel_mgr_paras);
        } else if (event->services[m].servie_id == EN_IOTA_EVENT_SOFT_BUS) {
            MemFree(&event->services[m].soft_bus_paras);
        } else if (event->services[m].servie_id == EN_IOTA_EVENT_FILE_MANAGER) {
            MemFree(&event->services[m].file_mgr_paras);
        }
    }

    MemFree(&event->services);
    MemFree(&event->mqtt_msg_info);
    MemFree(&event);
    return;
}

static void setOtaCallback(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message)
{
    if (message->event_type == EN_IOTA_EVENT_VERSION_QUERY) {
        if (tagOtaOps.onDeviceVersionUp) {
            tagOtaOps.onDeviceVersionUp(objectDeviceId);
        }
    } 
    if ((message->event_type == EN_IOTA_EVENT_FIRMWARE_UPGRADE) ||
        (message->event_type == EN_IOTA_EVENT_SOFTWARE_UPGRADE) ||
        (message->event_type == EN_IOTA_EVENT_FIRMWARE_UPGRADE_V2) ||
        (message->event_type == EN_IOTA_EVENT_SOFTWARE_UPGRADE_V2)) {
        
        if (tagOtaOps.onUrlResponse) {
            tagOtaOps.onUrlResponse(objectDeviceId, message->event_type, message->ota_paras);
        }

    }
}

static int OnEventsDownArrivedJosnSplicing(EN_IOTA_EVENT *event, JSON *services, const char *message, int services_count)
{
    int i = 0;
    JSON *serviceEvent = NULL;
    cJSON_ArrayForEach(serviceEvent, services) {
        if (serviceEvent) {
            char *service_id = JSON_GetStringFromObject(serviceEvent, SERVICE_ID, NULL); 
            event->services[i].servie_id = EN_IOTA_EVENT_ERROR;

            char *event_type = NULL; // To determine whether to add or delete a sub device
            event_type = JSON_GetStringFromObject(serviceEvent, EVENT_TYPE, NULL);
            event->services[i].event_type = EN_IOTA_EVENT_TYPE_ERROR;

            char *event_time = JSON_GetStringFromObject(serviceEvent, EVENT_TIME, NULL);
            event->services[i].event_time = event_time;

            JSON *paras = JSON_GetObjectFromObject(serviceEvent, PARAS);

            // sub device manager
            if (!strcmp(service_id, SUB_DEVICE_MANAGER)) {
                event->services[i].servie_id = EN_IOTA_EVENT_SUB_DEVICE_MANAGER;
                int ret = OnEventsDownManagerArrived(i, event_type, event, paras, message, serviceEvent);
                if (ret < 0) {
                    PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDownArrived(): there is not enough memory here.\n");
                    return -1;
                }
                if (tagEventsOps.onSubDevice) {
                    tagEventsOps.onSubDevice(event->object_device_id, event->mqtt_msg_info, &event->services[i]);
                }
            }

            // OTA
            if (!strcmp(service_id, OTA)) {
                event->services[i].servie_id = EN_IOTA_EVENT_OTA;
                int ret = OnEventsDownOtaArrived(&event->services[i], event_type, paras);
                if (ret < 0) {
                    return -1;
                }
                setOtaCallback(event->object_device_id, event->mqtt_msg_info, &event->services[i]);
            }

            // NTP
            if (!strcmp(service_id, SDK_TIME)) {
                event->services[i].servie_id = EN_IOTA_EVENT_TIME_SYNC;
                int ret = OnEventsDownNtpArrived(&event->services[i], event_type, message);
                if (ret < 0) {
                    return -1;
                }
                if (tagEventsOps.onNTP) {
                    tagEventsOps.onNTP(event->object_device_id, event->mqtt_msg_info, &event->services[i]);
                }
            }

            // device log
            if (!strcmp(service_id, LOG)) {
                event->services[i].servie_id = EN_IOTA_EVENT_DEVICE_LOG;
                int ret = OnEventsDownLogArrived(&event->services[i], event_type, paras);
                if (ret < 0) {
                    return -1;
                }
                if (tagEventsOps.onDeviceLog) {
                    tagEventsOps.onDeviceLog(event->object_device_id, event->mqtt_msg_info, &event->services[i]);
                }
            }

            // soft bus
            if (!strcmp(service_id, SOFT_BUS_SERVICEID)) {
                event->services[i].servie_id = EN_IOTA_EVENT_SOFT_BUS;
                char *event_id = JSON_GetStringFromObject(serviceEvent, EVENT_ID, NULL);
                event->services[i].event_id = event_id;
                int ret = OnEventsDownSoftBus(&event->services[i], event_type, paras);
                if (ret < 0) {
                    return -1;
                }
                if (tagEventsOps.onSoftBus){
                    tagEventsOps.onSoftBus(event->object_device_id, event->mqtt_msg_info, &event->services[i]);
                }
            }

            // tunnel manager
            if (!strcmp(service_id, TUNNEL_MGR)) {
                event->services[i].servie_id = EN_IOTA_EVENT_TUNNEL_MANAGER;
                int ret = OnEventsDownTunnelArrived(&event->services[i], event_type, paras);
                if (ret < 0) {
                    return -1;
                }
                if (tagEventsOps.onTunnelManager) {
                    tagEventsOps.onTunnelManager(event->object_device_id, event->mqtt_msg_info, &event->services[i]);
                }
            }
            // file manager
            if (!strcmp(service_id, FILE_MANAGER)) {
                event->services[i].servie_id = EN_IOTA_EVENT_FILE_MANAGER;
                int ret = OnEventsDownFileManager(&event->services[i], event_type, paras);
                if (ret < 0) {
                    return -1;
                }
                if (tagEventsOps.onFileManager) {
                    tagEventsOps.onFileManager(event->object_device_id, event->mqtt_msg_info, &event->services[i]);
                }
            }
            // device rule
            if (!strcmp(service_id, DEVICE_RULE)) {
                char *payload = cJSON_Print(paras);
                if (payload == NULL) {
                    return -2;
                }
                RuleMgr_Parse(payload);
                MemFree(&payload);
                i--;
            }
            // device remote config
            if (!strcmp(service_id, DEVICE_CONFIG)) {
                event->services[i].servie_id = EN_IOTA_EVENT_DEVICE_CONFIG;
                int ret = OnEventsDownRemoteCfgArrived(&event->services[i], event_type, paras, event->object_device_id);
                if (ret < 0) {
                    return -1;
                }
            }
        }
        i++;
        services_count--;
    }
    event->services_count = i;
    return 0;
}

static void OnEventsDownArrived(void *context, int token, int code, char *bridgeDeviceId, const char *message)
{
    EN_IOTA_EVENT *event = (EN_IOTA_EVENT *)malloc(sizeof(EN_IOTA_EVENT));
    if (event == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        return;
    }

    event->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (event->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        MemFree(&event);
        return;
    }

    event->mqtt_msg_info->context = context;
    event->mqtt_msg_info->messageId = token;
    event->mqtt_msg_info->code = code;
    event->bridge_device_id = bridgeDeviceId;
    JSON *root = JSON_Parse(message);
    if (root == NULL) {
        MemFree(&event->mqtt_msg_info);
        return;
    }

    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");
    event->object_device_id = object_device_id;

    JSON *services = JSON_GetObjectFromObject(root, SERVICES);

    int services_count = 0;
    services_count = JSON_GetArraySize(services);
    if (services_count > MAX_EVENT_COUNT) {
        // you can increase the MAX_EVENT_COUNT in iota_init.h
        PrintfLog(EN_LOG_LEVEL_ERROR, "messageArrivaled: services_count is too large.\n");
        JSON_Delete(root);
        MemFree(&event);
        return;
    }

    event->services_count = 0;
    event->services = (EN_IOTA_SERVICE_EVENT *)malloc(sizeof(EN_IOTA_SERVICE_EVENT) * services_count);
    if (event->services == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
        MemFree(&event->mqtt_msg_info);
        MemFree(&event);
        return;
    }
    int ret = OnEventsDownArrivedJosnSplicing(event, services, message, services_count);
    if (ret < 0) { 
        if (ret == -1) {
            MemFree(&event->services);
            MemFree(&event->mqtt_msg_info);
            MemFree(&event);
        }
        return;
    }

    if (onEventDown) {
        (onEventDown)(event);
    }
    JSON_Delete(root);
    OnEventsDownMemFree(event, event->services_count);
    return;
}

char *getBridgeDeviceId(char *topic)
{
    char *start = StrInStr(topic, TOPIC_SUFFIX_DEVICES);
    if  (start != NULL) {
       start += sizeof(char) * strlen(TOPIC_SUFFIX_DEVICES);
    }
    char *finish = StrInStr(topic, TOPIC_SUFFIX_SYS);
    if (start >= finish) {
        return NULL;
    }
    unsigned int len = sizeof(char) * (finish - start);
    char *bridgeDeviceId = (char *)malloc(sizeof(char) * (len + 1));
    if (bridgeDeviceId == NULL) {
        return NULL;
    }
    if (strncpy_s(bridgeDeviceId, len + 1, start, len) != 0) {
        MemFree(&bridgeDeviceId);
        return NULL;
    }
    return bridgeDeviceId;
}

static int OnBridgeArrived(void *context, int token, int code, char *topic, char *bridgeDeviceId, char *message)
{
    
    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_LOGIN)) > 0) {
        OnBridgesLoginOrLogoutArrived(context, token, code, bridgeDeviceId, topic, message, 1);
        return 0;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_LOGOUT)) > 0) {
        OnBridgesLoginOrLogoutArrived(context, token, code, bridgeDeviceId, topic, message, 0);
        return 0;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_RESET_SECRET)) > 0) {
        OnBridgesResetSecretArrived(context, token, code, bridgeDeviceId, topic, message);
        return 0;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_DISCONNECT)) > 0) {
        OnBridgesDeviceDisconnectArrived(context, token, code, bridgeDeviceId, message);
        return 0;
    }
    return -1;
}
// todo
static void OnMessageArrived(void *context, int token, int code, char *topic, char *message, int messageLength,
    void *mqttv5)
{
    char *bridgeDeviceId = NULL;
    if (StrInStr(topic, TOPIC_PREFIX_BRIDGE) == topic) {
        bridgeDeviceId = getBridgeDeviceId(topic);
    }
    if (StringLength(StrInStr(topic, BOOTSTRAP_DOWN)) > 0) {
        OnBootstrapDownArrived(context, token, code, message);
        goto EXIT;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_MESSAGEDOWN)) > 0) {
        OnMessagesDownArrived(context, token, code, bridgeDeviceId, message, messageLength, mqttv5);
        goto EXIT;
    }

    if (StringLength(StrInStr(topic, TOPIC_PREFIX_V3)) > 0) {
        OnV1DevicesArrived(context, token, code, message);
 
        goto EXIT;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_COMMAND)) > 0) {
        OnCommandsArrived(context, token, code, bridgeDeviceId, topic, message);
        goto EXIT;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_SET)) > 0) {
        OnPropertiesSetArrived(context, token, code, bridgeDeviceId, topic, message);
        goto EXIT;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_GET)) > 0) {
        OnPropertiesGetArrived(context, token, code, bridgeDeviceId, topic, message);
        goto EXIT;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_RSP)) > 0) {
        OnShadowGetResponseArrived(context, token, code, bridgeDeviceId, topic, message);
        goto EXIT;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_USER)) > 0) {
        OnUserTopicArrived(context, token, code, topic, message, messageLength);
        goto EXIT;
    }
    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_EVENT_DOWN)) > 0) {
        OnEventsDownArrived(context, token, code, bridgeDeviceId, message);
        goto EXIT;
    }

    if (StringLength(StrInStr(topic, TOPIC_PREFIX_M2M)) > 0) {
        OnM2mMessagesDownArrived(context, token, code, bridgeDeviceId, message);
        goto EXIT;
    }
    if (OnBridgeArrived(context, token, code, topic, bridgeDeviceId, message) < 0) {
        goto EXIT;
    }
    OnOtherUndefinedTopicArrived(context, token, code, bridgeDeviceId, message);  

EXIT:
    if (bridgeDeviceId) {
            MemFree(&bridgeDeviceId);
    }
}

void SetLogCallback(LOG_CALLBACK_HANDLER logCallbackHandler)
{
    SetPrintfLogCallback(logCallbackHandler);
}

void SetEventCallback(EVENT_CALLBACK_HANDLER callbackHandler)
{
    onEventDown = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetCmdCallback(CMD_CALLBACK_HANDLER callbackHandler)
{
    onCmd = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetCmdCallbackV3(CMD_CALLBACK_HANDLER_V3 callbackHandler)
{
    onCmdV3 = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetProtocolCallback(EN_CALLBACK_SETTING item, PROTOCOL_CALLBACK_HANDLER callbackHandler)
{
    switch (item) {
        case EN_CALLBACK_CONNECT_SUCCESS:
            onConnSuccess = callbackHandler;
            MqttBase_SetCallback(EN_MQTT_BASE_CALLBACK_CONNECT_SUCCESS, OnLoginSuccess);
            break;
        case EN_CALLBACK_CONNECT_FAILURE:
            MqttBase_SetCallback(EN_MQTT_BASE_CALLBACK_CONNECT_FAILURE, callbackHandler);
            break;
        case EN_CALLBACK_DISCONNECT_SUCCESS:
            MqttBase_SetCallback(EN_MQTT_BASE_CALLBACK_DISCONNECT_SUCCESS, callbackHandler);
            break;
        case EN_CALLBACK_DISCONNECT_FAILURE:
            MqttBase_SetCallback(EN_MQTT_BASE_CALLBACK_DISCONNECT_FAILURE, callbackHandler);
            break;
        case EN_CALLBACK_CONNECTION_LOST:
            MqttBase_SetCallback(EN_MQTT_BASE_CALLBACK_CONNECTION_LOST, callbackHandler);
            break;
        case EN_CALLBACK_PUBLISH_SUCCESS:
            MqttBase_SetCallback(EN_MQTT_BASE_CALLBACK_PUBLISH_SUCCESS, callbackHandler);
            break;
        case EN_CALLBACK_PUBLISH_FAILURE:
            MqttBase_SetCallback(EN_MQTT_BASE_CALLBACK_PUBLISH_FAILURE, callbackHandler);
            break;
        case EN_CALLBACK_SUBSCRIBE_SUCCESS:
            MqttBase_SetCallback(EN_MQTT_BASE_CALLBACK_SUBSCRIBE_SUCCESS, callbackHandler);
            break;
        case EN_CALLBACK_SUBSCRIBE_FAILURE:
            MqttBase_SetCallback(EN_MQTT_BASE_CALLBACK_SUBSCRIBE_FAILURE, callbackHandler);
            break;
        default:
            PrintfLog(EN_LOG_LEVEL_WARNING,
                "callback_func: SetProtocolCallback() warning, the item (%d) to be set is not available\n", item);
    }
}

void SetMessageCallback(MESSAGE_CALLBACK_HANDLER callbackHandler)
{
    onMessage = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetRawMessageCallback(RAW_MESSAGE_CALLBACK_HANDLER callbackHandler)
{
    onRawMessage = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetPropSetCallback(PROP_SET_CALLBACK_HANDLER callbackHandler)
{
    onPropertiesSet = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetPropGetCallback(PROP_GET_CALLBACK_HANDLER callbackHandler)
{
    onPropertiesGet = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetShadowGetCallback(SHADOW_GET_CALLBACK_HANDLER callbackHandler)
{
    onDeviceShadow = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetUserTopicMsgCallback(USER_TOPIC_MSG_CALLBACK_HANDLER callbackHandler)
{
    onUserTopicMessage = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetUserTopicRawMsgCallback(USER_TOPIC_RAW_MSG_CALLBACK_HANDLER callbackHandler)
{
    onUserTopicRawMessage = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetBridgesDeviceLoginCallback(BRIDGES_DEVICE_LOGIN callbackHandler)
{
    onBridgesDeviceLogin = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetBridgesDeviceLogoutCallback(BRIDGES_DEVICE_LOGOUT callbackHandler)
{
    onBridgesDeviceLogout = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetBridgesDeviceResetSecretCallback(BRIDGES_DEVICE_RESET_SECRET callbackHandler) 
{
    onBridgesDeviceResetSecret = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetBridgesDevicePalletDisConnCallback(BRIDGES_DEVICE_PLATE_DISCONNECT callbackHandler)
{
    onBridgesDeviceDisconnect = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetUndefinedMessageCallback(UNDEFINED_MESSAGE_CALLBACK_HANDLER callbackHandler)
{
    onOtherUndefined = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetBootstrapCallback(BOOTSTRAP_CALLBACK_HANDLER callbackHandler)
{
    onBootstrap = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetDeviceRuleSendMsgCallback(DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER callbackHandler)
{
    RuleMgr_SetSendMsgCallback(callbackHandler);
}

void SetM2mCallback(M2M_CALLBACK_HANDLER callbackHandler)
{
    onM2mMessage = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetDeviceConfigCallback(DEVICE_CONFIG_CALLBACK_HANDLER callbackHandler)
{
    onDeviceConfig = callbackHandler;
}

void SetTagEventsOps(TagEventsOps ops) 
{
    tagEventsOps = ops;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetEvenSubDeviceCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    tagEventsOps.onSubDevice = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetEvenOtaVersionUpCallback(OTAVERSION_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    tagOtaOps.onDeviceVersionUp = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetEvenOtaUrlResponseCallback(OTAURL_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    tagOtaOps.onUrlResponse = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetNtpCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    tagEventsOps.onNTP = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetEvenDeviceLogCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    tagEventsOps.onDeviceLog = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetEvenSoftBusCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    tagEventsOps.onSoftBus = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetEvenTunnelManagerCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    tagEventsOps.onTunnelManager = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}

void SetEvenFileManagerCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    tagEventsOps.onFileManager = callbackHandler;
    MqttBase_SetMessageArrivedCallback(OnMessageArrived);
}
