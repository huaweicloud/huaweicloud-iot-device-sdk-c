/* Copyright (c) <2020>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * &Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  */

#include "string_util.h"
#include "log_util.h"
#include "mqtt_base.h"
#include "base.h"
#include "callback_func.h"
#include "subscribe.h"
#include "string.h"
#include "iota_error_type.h"
#include "json_util.h"
#include "cJSON.h"
#include "iota_datatrans.h"
#include "rule_trans.h"

EVENT_CALLBACK_HANDLER onEventDown;
CMD_CALLBACK_HANDLER onCmd;
CMD_CALLBACK_HANDLER_V3 onCmdV3;
PROTOCOL_CALLBACK_HANDLER onConnSuccess;
MESSAGE_CALLBACK_HANDLER onMessage;
PROP_SET_CALLBACK_HANDLER onPropertiesSet;
PROP_GET_CALLBACK_HANDLER onPropertiesGet;
SHADOW_GET_CALLBACK_HANDLER onDeviceShadow;
USER_TOPIC_MSG_CALLBACK_HANDLER onUserTopicMessage;
BOOTSTRAP_CALLBACK_HANDLER onBootstrap;

void OnLoginSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    // The platform subscribes to system topic with QoS of 1 by default
    // SubscribeAll();
    if (onConnSuccess) {
        (onConnSuccess)(rsp);
    }

    // report device log
    unsigned long long timestamp = getTime();
    char timeStampStr[14];
    sprintf_s(timeStampStr, sizeof(timeStampStr), "%llu", timestamp);
    char *log = "login success";
    IOTA_ReportDeviceLog("DEVICE_STATUS", log, timeStampStr, NULL);

    // report sdk version
    ST_IOTA_DEVICE_INFO_REPORT deviceInfo;

    deviceInfo.device_sdk_version = SDK_VERSION;
    deviceInfo.sw_version = NULL;
    deviceInfo.fw_version = NULL;
    deviceInfo.event_time = NULL;
    deviceInfo.object_device_id = NULL;

    IOTA_ReportDeviceInfo(&deviceInfo, NULL);
    IOTA_GetDeviceShadow(DEVICE_RULE_REQUEST_ID, NULL, NULL, NULL);

}
void OnBootstrapDownArrived(void *context, int token, int code, char *message)
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
    if (onBootstrap) {
        (onBootstrap)(bootstrap_msg);
    }

    JSON_Delete(root);
    MemFree(&bootstrap_msg->mqtt_msg_info);
    MemFree(&bootstrap_msg);
    return;
}

void OnMessagesDownArrived(void *context, int token, int code, char *message, void *mqttv5)
{
    EN_IOTA_MESSAGE *msg = (EN_IOTA_MESSAGE *)malloc(sizeof(EN_IOTA_MESSAGE));
    if (msg == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnMessagesDownArrived(): there is not enough memory here.\n");
        return;
    }
    msg->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (msg->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnMessagesDownArrived(): there is not enough memory here.\n");
        MemFree(&msg);
        return;
    }
    msg->mqtt_msg_info->context = context;
    msg->mqtt_msg_info->messageId = token;
    msg->mqtt_msg_info->code = code;

    JSON *root = JSON_Parse(message);

    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");
    msg->object_device_id = object_device_id;

    char *name = JSON_GetStringFromObject(root, NAME, "-1");
    msg->name = name;

    char *id = JSON_GetStringFromObject(root, ID, "-1");
    msg->id = id;

    char *content = JSON_GetStringFromObject(root, CONTENT, "-1");
    msg->content = content;

    if (onMessage) {
        (onMessage)(msg, mqttv5);
    }

    JSON_Delete(root);
    MemFree(&msg->mqtt_msg_info);
    MemFree(&msg);
    return;
}

void OnV1DevicesArrived(void *context, int token, int code, char *message)
{
    EN_IOTA_COMMAND_V3 *command_v3 = (EN_IOTA_COMMAND *)malloc(sizeof(EN_IOTA_COMMAND));
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

    char *serviceId = JSON_GetStringFromObject(root, SERVICE_ID_V3, "-1"); // get value of serviceId
    command_v3->serviceId = serviceId;

    char *cmd = JSON_GetStringFromObject(root, CMD, "-1"); // get value of cmd
    command_v3->cmd = cmd;

    int mid = JSON_GetIntFromObject(root, MID, -1); // get value of mid
    command_v3->mid = mid;

    JSON *paras = JSON_GetObjectFromObject(root, PARAS); // get value of paras

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

void OnCommandsArrived(void *context, int token, int code, const char *topic, char *message)
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

    command->request_id = request_id;

    JSON *root = JSON_Parse(message);

    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1"); // get value of object_device_id
    command->object_device_id = object_device_id;

    char *service_id = JSON_GetStringFromObject(root, SERVICE_ID, "-1"); // get value of service_id
    command->service_id = service_id;

    char *command_name = JSON_GetStringFromObject(root, COMMAND_NAME, "-1"); // get value of command_name
    command->command_name = command_name;

    JSON *paras = JSON_GetObjectFromObject(root, PARAS); // get value of data

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

void OnPropertiesSetArrived(void *context, int token, int code, const char *topic, char *message)
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

    prop_set->request_id = request_id;

    JSON *root = JSON_Parse(message);

    // get value of object_device_id
    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1"); 
    prop_set->object_device_id = object_device_id;

    JSON *services = JSON_GetObjectFromObject(root, SERVICES); // get  services array

    int services_count = JSON_GetArraySize(services); // get length of services array
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
    while (services_count > 0) {
        JSON *service = JSON_GetObjectFromArray(services, i);
        if (service) {
            char *service_id = JSON_GetStringFromObject(service, SERVICE_ID, NULL);
            prop_set->services[i].service_id = service_id;

            JSON *properties = JSON_GetObjectFromObject(service, PROPERTIES);
            prop[i] = cJSON_Print(properties);
            prop_set->services[i].properties = prop[i];
            if(strcmp(service_id, DEVICE_RULE) == 0) {
                RuleTrans_DeviceRuleUpdate(prop[i]);
                MemFree(&prop[i]);
                i--;
            } 
        }
        i++;
        services_count--;
    }
    prop_set->services_count = i;

    if (onPropertiesSet) {
        (onPropertiesSet)(prop_set);
    }

    JSON_Delete(root);
    int j;
    for (j = 0; j < i; j++) {
        MemFree(&prop[j]);
    }
    MemFree(&prop);
    MemFree(&prop_set->services);
    MemFree(&prop_set->mqtt_msg_info);
    MemFree(&prop_set);
    return;
}

void OnPropertiesGetArrived(void *context, int token, int code, const char *topic, char *message)
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

    prop_get->request_id = request_id;

    JSON *root = JSON_Parse(message);

    // get value of object_device_id
    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1"); 
    prop_get->object_device_id = object_device_id;

    char *service_id = JSON_GetStringFromObject(root, SERVICE_ID, "-1"); // get value of object_device_id
    prop_get->service_id = service_id;

    if (onPropertiesGet) {
        (onPropertiesGet)(prop_get);
    }

    JSON_Delete(root);
    MemFree(&prop_get->mqtt_msg_info);
    MemFree(&prop_get);
    return;
}

void OnShadowGetResponseArrived(void *context, int token, int code, const char *topic, char *message)
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
    int i = 0;
    char *desired_str[MAX_SERVICE_COUNT] = {0};
    char *reported_str[MAX_SERVICE_COUNT] = {0};
    while (shadow_data_count > 0) {
        JSON *shadow_data = JSON_GetObjectFromArray(shadow, i);

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

void OnUserTopicArrived(void *context, int token, int code, const char *topic, char *message)
{
    char *topic_paras_value = strstr(topic, "user/");
    char *topic_paras = strstr(topic_paras_value + 5, "");


    EN_IOTA_USER_TOPIC_MESSAGE *user_topic_msg =
        (EN_IOTA_USER_TOPIC_MESSAGE *)malloc(sizeof(EN_IOTA_USER_TOPIC_MESSAGE));
    if (user_topic_msg == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnUserTopicArrived(): there is not enough memory here.\n");
        return;
    }
    user_topic_msg->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (user_topic_msg->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnUserTopicArrived(): there is not enough memory here.\n");
        MemFree(&user_topic_msg);
        return;
    }
    user_topic_msg->mqtt_msg_info->context = context;
    user_topic_msg->mqtt_msg_info->messageId = token;
    user_topic_msg->mqtt_msg_info->code = code;

    user_topic_msg->topic_para = topic_paras;

    JSON *root = JSON_Parse(message);

    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");
    user_topic_msg->object_device_id = object_device_id;

    char *name = JSON_GetStringFromObject(root, NAME, "-1");
    user_topic_msg->name = name;

    char *id = JSON_GetStringFromObject(root, ID, "-1");
    user_topic_msg->id = id;

    char *content = JSON_GetStringFromObject(root, CONTENT, "-1");
    user_topic_msg->content = content;

    if (onUserTopicMessage) {
        (onUserTopicMessage)(user_topic_msg);
    }

    JSON_Delete(root);
    MemFree(&user_topic_msg->mqtt_msg_info);
    MemFree(&user_topic_msg);
    return;
}

// if it is the platform inform the gateway to add or delete the sub device
int OnEventsGatewayAddOrDelete(int i, char *event_type, EN_IOTA_EVENT *event, JSON *paras, char *message)
{
    event->services[i].paras = (EN_IOTA_DEVICE_PARAS *)malloc(sizeof(EN_IOTA_DEVICE_PARAS));
    if (event->services[i].paras == NULL) {
        return -1;
    }

    JSON *devices = JSON_GetObjectFromObject(paras, DEVICES); // get object of devices

    int devices_count = JSON_GetArraySize(devices); // get size of devicesSize

    if (devices_count > MAX_EVENT_DEVICE_COUNT) {
        // you can increase the  MAX_EVENT_DEVICE_COUNT in iota_init.h
        PrintfLog(EN_LOG_LEVEL_ERROR, "messageArrivaled: OnEventsGatewayAddOrDelete(), devices_count is too large.\n"); 
        // JSON_Delete(root);
        return 0;
    }

    event->services[i].paras->devices = (EN_IOTA_DEVICE_INFO *)malloc(sizeof(EN_IOTA_DEVICE_INFO) * devices_count);
    if (event->services[i].paras->devices == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsGatewayAddOrDelete(): there is not enough memory here.\n");
        while (i >= 0) {
            MemFree(&event->services[i].paras);
            i--;
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
        while (devices_count > 0) {
            JSON *deviceInfo = JSON_GetObjectFromArray(devices, j); // get object of deviceInfo

            char *parent_device_id = JSON_GetStringFromObject(deviceInfo, PARENT_DEVICE_ID, NULL); // get value of parent_device_id
            event->services[i].paras->devices[j].parent_device_id = parent_device_id;

            char *node_id = JSON_GetStringFromObject(deviceInfo, NODE_ID, NULL); // get value of node_id
            event->services[i].paras->devices[j].node_id = node_id;

            char *device_id = JSON_GetStringFromObject(deviceInfo, DEVICE_ID, NULL); // get value of device_id
            event->services[i].paras->devices[j].device_id = device_id;

            char *name = JSON_GetStringFromObject(deviceInfo, NAME, NULL); // get value of name
            event->services[i].paras->devices[j].name = name;

            char *description = JSON_GetStringFromObject(deviceInfo, DESCRIPTION, NULL); // get value of description
            event->services[i].paras->devices[j].description = description;

            char *manufacturer_id = JSON_GetStringFromObject(deviceInfo, MANUFACTURER_ID,
                NULL); // get value of manufacturer_id
            event->services[i].paras->devices[j].manufacturer_id = manufacturer_id;

            char *model = JSON_GetStringFromObject(deviceInfo, MODEL, NULL); // get value of model
            event->services[i].paras->devices[j].model = model;

            char *product_id = JSON_GetStringFromObject(deviceInfo, PRODUCT_ID, NULL); // get value of product_id
            event->services[i].paras->devices[j].product_id = product_id;

            char *fw_version = JSON_GetStringFromObject(deviceInfo, FW_VERSION, NULL); // get value of fw_version
            event->services[i].paras->devices[j].fw_version = fw_version;

            char *sw_version = JSON_GetStringFromObject(deviceInfo, SW_VERSION, NULL); // get value of sw_version
            event->services[i].paras->devices[j].sw_version = sw_version;

            char *status = JSON_GetStringFromObject(deviceInfo, STATUS, NULL); // get value of status
            event->services[i].paras->devices[j].status = status;

            char *extension_info = JSON_GetStringFromObject(deviceInfo, EXTENSION_INFO, NULL); // get value of status
            event->services[i].paras->devices[j].extension_info = extension_info;

            j++;
            devices_count--;
        }
    }

    // deleting a sub device notify
    if (!strcmp(event_type, DELETE_SUB_DEVICE_NOTIFY)) {
        event->services[i].event_type = EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY;
        while (devices_count > 0) {
            JSON *deviceInfo = JSON_GetObjectFromArray(devices, j); // get object of deviceInfo

            // get value of parent_device_id
            char *parent_device_id = JSON_GetStringFromObject(deviceInfo, PARENT_DEVICE_ID, NULL); 
            event->services[i].paras->devices[j].parent_device_id = parent_device_id;

            char *node_id = JSON_GetStringFromObject(deviceInfo, NODE_ID, NULL); // get value of node_id
            event->services[i].paras->devices[j].node_id = node_id;

            char *device_id = JSON_GetStringFromObject(deviceInfo, DEVICE_ID, NULL); // get value of device_id
            event->services[i].paras->devices[j].device_id = device_id;

            j++;
            devices_count--;
        }
    }
    return 1;
}

int OnEventsAddSubDeviceResponse(int i, char *event_type, EN_IOTA_EVENT *event, JSON *paras, JSON *serviceEvent)
{
    // get value of event_id
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
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsAddSubDeviceResponse(): successful_devices is not enough memory here.\n");
        while (i >= 0) {
            MemFree(&event->services[i].gtw_add_device_paras);
            i--;
        }
        return -1;
    }

    int j = 0;
    while (successful_devices_count > 0) {
        JSON *deviceInfo = JSON_GetObjectFromArray(successful_devices, j);

        char *parent_device_id = JSON_GetStringFromObject(deviceInfo, PARENT_DEVICE_ID, NULL); // get value of parent_device_id
        event->services[i].gtw_add_device_paras->successful_devices[j].parent_device_id = parent_device_id;

        char *node_id = JSON_GetStringFromObject(deviceInfo, NODE_ID, NULL); // get value of node_id
        event->services[i].gtw_add_device_paras->successful_devices[j].node_id = node_id;

        char *device_id = JSON_GetStringFromObject(deviceInfo, DEVICE_ID, NULL); // get value of device_id
        event->services[i].gtw_add_device_paras->successful_devices[j].device_id = device_id;

        char *name = JSON_GetStringFromObject(deviceInfo, NAME, NULL); // get value of name
        event->services[i].gtw_add_device_paras->successful_devices[j].name = name;

        char *description = JSON_GetStringFromObject(deviceInfo, DESCRIPTION, NULL); // get value of description
        event->services[i].gtw_add_device_paras->successful_devices[j].description = description;

        char *manufacturer_id = JSON_GetStringFromObject(deviceInfo, MANUFACTURER_ID, NULL); // get value of manufacturer_id
        event->services[i].gtw_add_device_paras->successful_devices[j].manufacturer_id = manufacturer_id;

        char *model = JSON_GetStringFromObject(deviceInfo, MODEL, NULL); // get value of model
        event->services[i].gtw_add_device_paras->successful_devices[j].model = model;

        char *product_id = JSON_GetStringFromObject(deviceInfo, PRODUCT_ID, NULL); // get value of product_id
        event->services[i].gtw_add_device_paras->successful_devices[j].product_id = product_id;

        char *fw_version = JSON_GetStringFromObject(deviceInfo, FW_VERSION, NULL); // get value of fw_version
        event->services[i].gtw_add_device_paras->successful_devices[j].fw_version = fw_version;

        char *sw_version = JSON_GetStringFromObject(deviceInfo, SW_VERSION, NULL); // get value of sw_version
        event->services[i].gtw_add_device_paras->successful_devices[j].sw_version = sw_version;

        char *status = JSON_GetStringFromObject(deviceInfo, STATUS, NULL); // get value of status
        event->services[i].gtw_add_device_paras->successful_devices[j].status = status;

        char *extension_info = JSON_GetStringFromObject(deviceInfo, EXTENSION_INFO, NULL); // get value of status
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
    while (failed_devices_count > 0) {
        JSON *reason = JSON_GetObjectFromArray(failed_devices, k);

        char *node_id = JSON_GetStringFromObject(reason, NODE_ID, NULL); // get value of parent_device_id
        event->services[i].gtw_add_device_paras->failed_devices[k].node_id = node_id;

        char *product_id = JSON_GetStringFromObject(reason, PRODUCT_ID, NULL); // get value of parent_device_id
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

int OnEventsDeleteSubDeviceResponse(int i, char *event_type, EN_IOTA_EVENT *event, JSON *paras, JSON *serviceEvent)
{
    char *event_id = JSON_GetStringFromObject(serviceEvent, EVENT_ID, NULL); // get value of
                                                                             // event_id
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
        (HW_CHAR *)malloc(sizeof(HW_CHAR) * successful_devices_count);
    if (event->services[i].gtw_del_device_paras->successful_devices == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDeleteSubDeviceResponse(): successful_devices is not enough memory here.\n");
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
    while (failed_devices_count > 0) {
        char *reason = JSON_GetObjectFromArray(failed_devices, k);

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

int OnEventsDownManagerArrived(int i, char *event_type, EN_IOTA_EVENT *event, JSON *paras, char *message, JSON *serviceEvent)
{
    // if it is the platform inform the gateway to add or delete the sub device
    if (!strcmp(event_type, DELETE_SUB_DEVICE_NOTIFY) || !strcmp(event_type, ADD_SUB_DEVICE_NOTIFY)) {
        int ret = OnEventsGatewayAddOrDelete(i, event_type, event, paras, message);
        return ret;
    }

    // the response of gateway adding a sub device
    if (!strcmp(event_type, ADD_SUB_DEVICE_RESPONSE)) {
        int ret = OnEventsAddSubDeviceResponse(i, event_type, event, paras, serviceEvent);
        return ret;
    }

    // the response of gateway deleting a sub device
    if (!strcmp(event_type, DEL_SUB_DEVICE_RESPONSE)) {
        int ret = OnEventsDeleteSubDeviceResponse(i, event_type, event, paras, serviceEvent);
        return ret;
    }
    return 1;
}

int OnEventsDownOtaArrived(EN_IOTA_SERVICE_EVENT *services, char *event_type, JSON *paras)
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
    }

    // firmware_upgrade or software_upgrade
    if (services->event_type == EN_IOTA_EVENT_FIRMWARE_UPGRADE ||
        services->event_type == EN_IOTA_EVENT_SOFTWARE_UPGRADE) {
        char *version = JSON_GetStringFromObject(paras, VERSION, NULL); // get value of version
        services->ota_paras->version = version;

        char *url = JSON_GetStringFromObject(paras, URL, NULL); // get value of url
        services->ota_paras->url = url;

        int file_size = JSON_GetIntFromObject(paras, FILE_SIZE, -1); // get value of file_size
        services->ota_paras->file_size = file_size;

        char *access_token = JSON_GetStringFromObject(paras, ACCESS_TOKEN, NULL); // get value of access_token
        services->ota_paras->access_token = access_token;

        int expires = JSON_GetIntFromObject(paras, EXPIRES, -1); // get value of expires
        services->ota_paras->expires = expires;

        char *sign = JSON_GetStringFromObject(paras, SIGN, NULL); // get value of sign
        services->ota_paras->sign = sign;
    }
    return 1;
}

int OnEventsDownNtpArrived(EN_IOTA_SERVICE_EVENT *services, char *event_type, char *message)
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
        if (device_send_time < 0 || server_recv_time < 0 || server_send_time < 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR,
                "getLLongValueFromStr(), Length out of bounds. Modifiable value LONG_LONG_MAX_LENGTH\n");
            return -1;
        }
        services->ntp_paras->device_real_time =
            (server_recv_time + server_send_time + (HW_LLONG)getTime() - device_send_time) / 2;
    }
    return 1;
}

int OnEventsDownLogArrived(EN_IOTA_SERVICE_EVENT *services, char *event_type, JSON *paras)
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

void OnEventsDownMemFree(EN_IOTA_EVENT *event, int services_count)
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
        }
    }

    MemFree(&event->services);
    MemFree(&event->mqtt_msg_info);
    MemFree(&event);
    return;
}


void OnEventsDownArrived(void *context, int token, int code, char *message)
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


    JSON *root = JSON_Parse(message);
    if (root == NULL) {
        MemFree(&event->mqtt_msg_info);
        return;
    }
    // get value of object_device_id
    char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1"); 
    event->object_device_id = object_device_id;

    JSON *services = JSON_GetObjectFromObject(root, SERVICES); // get object of services

    int services_count = 0;
    services_count = JSON_GetArraySize(services); // get size of services

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

    int services_count_copy = services_count; // for releasing the services

    int i = 0;
    while (services_count > 0) {
        JSON *serviceEvent = JSON_GetObjectFromArray(services, i); // get object of ServiceEvent
        if (serviceEvent) {
            char *service_id = JSON_GetStringFromObject(serviceEvent, SERVICE_ID, NULL); // get value of service_id
            event->services[i].servie_id = EN_IOTA_EVENT_ERROR;                          // init service id

            char *event_type = NULL; // To determine whether to add or delete a sub device
            event_type = JSON_GetStringFromObject(serviceEvent, EVENT_TYPE, NULL); // get value of event_type
            event->services[i].event_type = EN_IOTA_EVENT_TYPE_ERROR;              // init event type

            char *event_time = JSON_GetStringFromObject(serviceEvent, EVENT_TIME, NULL); // get value of event_time
            event->services[i].event_time = event_time;

            JSON *paras = JSON_GetObjectFromObject(serviceEvent, PARAS); // get object of paras

            // sub device manager
            if (!strcmp(service_id, SUB_DEVICE_MANAGER)) {
                event->services[i].servie_id = EN_IOTA_EVENT_SUB_DEVICE_MANAGER;
                int ret = OnEventsDownManagerArrived(i, event_type, event, paras, message, serviceEvent);
                if (ret == 0) {
                    break;
                } else if (ret < 0) {
                    PrintfLog(EN_LOG_LEVEL_ERROR, "OnEventsDownArrived(): there is not enough memory here.\n");
                    MemFree(&event->services);
                    MemFree(&event->mqtt_msg_info);
                    MemFree(&event);
                    return;
                }
            }

            // OTA
            if (!strcmp(service_id, OTA)) {
                event->services[i].servie_id = EN_IOTA_EVENT_OTA;
                int ret = OnEventsDownOtaArrived(&event->services[i], event_type, paras);
                if (ret < 0) {
                    MemFree(&event->services);
                    MemFree(&event->mqtt_msg_info);
                    MemFree(&event);
                    return;
                }
            }

            // NTP
            if (!strcmp(service_id, SDK_TIME)) {
                event->services[i].servie_id = EN_IOTA_EVENT_TIME_SYNC;
                int ret = OnEventsDownNtpArrived(&event->services[i], event_type, message);
                if (ret < 0) {
                    MemFree(&event->services);
                    MemFree(&event->mqtt_msg_info);
                    MemFree(&event);
                    return;
                }
            }

            // device log
            if (!strcmp(service_id, LOG)) {
                event->services[i].servie_id = EN_IOTA_EVENT_DEVICE_LOG;
                int ret = OnEventsDownLogArrived(&event->services[i], event_type, paras);
                if (ret < 0) {
                    MemFree(&event->services);
                    MemFree(&event->mqtt_msg_info);
                    MemFree(&event);
                    return;
                }
            }

            // device rule
            if (!strcmp(service_id, DEVICE_RULE)) {
                char *payload = cJSON_Print(paras);
                if(payload == NULL) {
                    return;
                }
                RuleMgr_Parse(payload);
                MemFree(&payload);
                i--;
            }
        }

        i++;
        services_count--;
    }
    event->services_count = i;

    if (onEventDown) {
        (onEventDown)(event);
    }

    JSON_Delete(root);
    OnEventsDownMemFree(event, i);
    return;
}

void OnMessageArrived(void *context, int token, int code, const char *topic, char *message, void *mqttv5)
{
    if (StringLength(StrInStr(topic, BOOTSTRAP_DOWN)) > 0) {
        OnBootstrapDownArrived(context, token, code, message);
        return;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_MESSAGEDOWN)) > 0) {
        OnMessagesDownArrived(context, token, code, message, mqttv5);
        return;
    }

    if (StringLength(StrInStr(topic, TOPIC_PREFIX_V3)) > 0) {
        OnV1DevicesArrived(context, token, code, message);
        return;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_COMMAND)) > 0) {
        OnCommandsArrived(context, token, code, topic, message);
        return;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_SET)) > 0) {
        OnPropertiesSetArrived(context, token, code, topic, message);
        return;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_GET)) > 0) {
        OnPropertiesGetArrived(context, token, code, topic, message);
        return;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_RSP)) > 0) {
        OnShadowGetResponseArrived(context, token, code, topic, message);
        return;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_USER)) > 0) {
        OnUserTopicArrived(context, token, code, topic, message);
        return;
    }

    if (StringLength(StrInStr(topic, TOPIC_SUFFIX_EVENT_DOWN)) > 0) {
        OnEventsDownArrived(context, token, code, message);
        return;
    }
}

void SetLogCallback(LOG_CALLBACK_HANDLER logCallbackHandler)
{
    SetPrintfLogCallback(logCallbackHandler);
}

int SetEventCallback(EVENT_CALLBACK_HANDLER pfnCallbackHandler)
{
    onEventDown = pfnCallbackHandler;
    return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetCmdCallback(CMD_CALLBACK_HANDLER pfnCallbackHandler)
{
    onCmd = pfnCallbackHandler;
    return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetCmdCallbackV3(CMD_CALLBACK_HANDLER_V3 pfnCallbackHandler)
{
    onCmdV3 = pfnCallbackHandler;
    return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetProtocolCallback(int item, PROTOCOL_CALLBACK_HANDLER pfnCallbackHandler)
{
    switch (item) {
        case EN_CALLBACK_CONNECT_SUCCESS:
            onConnSuccess = pfnCallbackHandler;
            return MqttBase_SetCallback(item, OnLoginSuccess);
        default:
            return MqttBase_SetCallback(item, pfnCallbackHandler);
    }
}

int SetMessageCallback(MESSAGE_CALLBACK_HANDLER pfnCallbackHandler)
{
    onMessage = pfnCallbackHandler;
    return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetPropSetCallback(PROP_SET_CALLBACK_HANDLER pfnCallbackHandler)
{
    onPropertiesSet = pfnCallbackHandler;
    return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetPropGetCallback(PROP_GET_CALLBACK_HANDLER pfnCallbackHandler)
{
    onPropertiesGet = pfnCallbackHandler;
    return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetShadowGetCallback(SHADOW_GET_CALLBACK_HANDLER pfnCallbackHandler)
{
    onDeviceShadow = pfnCallbackHandler;
    return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetUserTopicMsgCallback(USER_TOPIC_MSG_CALLBACK_HANDLER pfnCallbackHandler)
{
    onUserTopicMessage = pfnCallbackHandler;
    return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetBootstrapCallback(BOOTSTRAP_CALLBACK_HANDLER pfnCallbackHandler)
{
    onBootstrap = pfnCallbackHandler;
    return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}
