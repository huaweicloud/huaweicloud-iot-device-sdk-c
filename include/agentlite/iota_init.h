/*
 * Copyright (c) 2020-2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#ifndef IOTA_INIT_H
#define IOTA_INIT_H

#include <stdarg.h>
#include "hw_type.h"
#include "json_util.h"

typedef HW_VOID (*PFN_CALLBACK_HANDLER)(HW_VOID *context, HW_INT messageId, HW_INT code, HW_CHAR *message);
typedef HW_VOID (*PFN_LOG_CALLBACK_HANDLER)(int level, char *format, va_list args);

/**
 * @param context: the application-self defined parameter, current is NULL
 * @param messageId: the inner messageId(1-65535) of the uplink message in this MQTT SDK
 * param code: the reason code for the failure callback
 */
typedef struct {
    HW_VOID *context;
    HW_INT messageId;
    HW_INT code;
} EN_IOTA_MQTT_MSG_INFO;

typedef struct {
    HW_CHAR *bus_infos;
} EN_IOTA_SOFT_BUS_PARAS;

typedef struct {
    HW_CHAR *parent_device_id;
    HW_CHAR *node_id;
    HW_CHAR *device_id;
    HW_CHAR *name;
    HW_CHAR *description;
    HW_CHAR *manufacturer_id;
    HW_CHAR *model;
    HW_CHAR *product_id;
    HW_CHAR *fw_version;
    HW_CHAR *sw_version;
    HW_CHAR *status;
    HW_CHAR *extension_info;
} EN_IOTA_DEVICE_INFO;

typedef struct {
    HW_CHAR *version;
    HW_CHAR *url;
    HW_INT file_size;
    HW_CHAR *file_name;
    HW_CHAR *task_id; // 批量任务id
    HW_INT sub_device_count; // 子设备数量
    HW_CHAR *task_ext_info; // 创建批量任务时添加的扩展信息
    HW_CHAR *access_token;
    HW_INT expires;
    HW_CHAR *sign;
} EN_IOTA_OTA_PARAS;

typedef struct {
    HW_CHAR *payload;
} EN_IOTA_DEVICE_RULE_PARAS;

typedef struct {
    EN_IOTA_DEVICE_INFO *devices;
    HW_INT devices_count;
    HW_LLONG version;
} EN_IOTA_DEVICE_PARAS;

typedef struct {
    HW_LLONG device_real_time;
} EN_IOTA_NTP_PARAS;

typedef struct {
    HW_CHAR *node_id;
    HW_CHAR *product_id;
    HW_CHAR *error_code;
    HW_CHAR *error_msg;
} EN_IOTA_ADD_DEVICE_FAILED_REASON;

typedef struct {
    EN_IOTA_DEVICE_INFO *successful_devices;
    HW_INT successful_devices_count;
    EN_IOTA_ADD_DEVICE_FAILED_REASON *failed_devices;
    HW_INT failed_devices_count;
} EN_IOTA_GTW_ADD_DEVICE_PARAS;

typedef struct {
    HW_CHAR *device_id;
    HW_CHAR *error_code;
    HW_CHAR *error_msg;
} EN_IOTA_DEL_DEVICE_FAILED_REASON, EN_IOTA_SUB_UPDATE_REASON;

typedef struct {
    HW_CHAR **successful_devices;
    HW_INT successful_devices_count;
    EN_IOTA_DEL_DEVICE_FAILED_REASON *failed_devices;
    HW_INT failed_devices_count;
} EN_IOTA_GTW_DEL_DEVICE_PARAS;

typedef struct {
    HW_CHAR *device_id;
    HW_CHAR *status;
} EN_IOTA_UODATE_STATUS_INFO;

typedef struct {
    EN_IOTA_UODATE_STATUS_INFO *successful_devices;
    HW_INT successful_devices_count;
    EN_IOTA_SUB_UPDATE_REASON *failed_devices;
    HW_INT failed_devices_count;
} EN_IOTA_GTW_UODATE_STATUS_PARAS;

typedef struct {
    HW_CHAR *log_switch;
    HW_CHAR *end_time;
} EN_IOTA_DEVICE_LOG_PARAS;

typedef struct {
    HW_CHAR *tunnel_url;
    HW_CHAR *tunnel_access_token;
} EN_IOTA_TUNNEL_MGR_PARAS;

typedef struct {
    HW_CHAR *url;
} EN_IOTA_FILE_MGR_PARAS;

typedef struct {
    HW_INT servie_id;  // see the enum EN_IOTA_EVENT_SERVICE_ID
    HW_INT event_type;  // see the enum EN_IOTA_EVENT_TYPE
    HW_CHAR *event_time;
    HW_CHAR *event_id;
    EN_IOTA_DEVICE_PARAS *paras;
    EN_IOTA_OTA_PARAS *ota_paras;
    EN_IOTA_NTP_PARAS *ntp_paras;
    EN_IOTA_GTW_ADD_DEVICE_PARAS *gtw_add_device_paras;
    EN_IOTA_GTW_DEL_DEVICE_PARAS *gtw_del_device_paras;
    EN_IOTA_GTW_UODATE_STATUS_PARAS *gtw_update_status_paras;
    EN_IOTA_DEVICE_LOG_PARAS *device_log_paras;
    EN_IOTA_DEVICE_RULE_PARAS *device_rule_paras;
    EN_IOTA_TUNNEL_MGR_PARAS *tunnel_mgr_paras;
    EN_IOTA_FILE_MGR_PARAS *file_mgr_paras;
    EN_IOTA_SOFT_BUS_PARAS *soft_bus_paras;
} EN_IOTA_SERVICE_EVENT;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *object_device_id;
    EN_IOTA_SERVICE_EVENT *services;
    HW_INT services_count;
    HW_CHAR *bridge_device_id;
} EN_IOTA_EVENT;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *object_device_id;
    HW_CHAR *service_id;
    HW_CHAR *command_name;
    HW_CHAR *paras;
    HW_CHAR *request_id;
    HW_CHAR *bridge_device_id;
} EN_IOTA_COMMAND;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *msgType;
    HW_CHAR *serviceId;
    HW_CHAR *cmd;
    HW_CHAR *paras;
    HW_INT mid;
} EN_IOTA_COMMAND_V3;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *message;
    HW_CHAR *deviceSecret;
} EN_IOTA_MQTT_PROTOCOL_RSP;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *object_device_id;
    HW_CHAR *name;
    HW_CHAR *id;
    HW_CHAR *content;
    HW_CHAR *bridge_device_id;
} EN_IOTA_MESSAGE;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *object_device_id;
    HW_CHAR *payload;
    HW_CHAR *bridge_device_id;
} EN_IOTA_UNDEFINED_MESSAGE;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqttMsgInfo;
    HW_CHAR *objectDeviceId;
    HW_INT payloadLength;
    HW_CHAR *payload;
} EN_IOTA_RAW_MESSAGE;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *request_id;
    HW_CHAR *to;
    HW_CHAR *from;
    HW_CHAR *content;
} EN_IOTA_M2M_MESSAGE;

typedef struct {
    HW_CHAR *service_id;
    HW_CHAR *properties;
} EN_IOTA_SERVICE_PROPERTY;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *object_device_id;
    HW_CHAR *request_id;
    EN_IOTA_SERVICE_PROPERTY *services;
    HW_INT services_count;
    HW_CHAR *bridge_device_id;
} EN_IOTA_PROPERTY_SET;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *object_device_id;
    HW_CHAR *request_id;
    HW_CHAR *service_id;
    HW_CHAR *bridge_device_id;
} EN_IOTA_PROPERTY_GET;

typedef struct {
    HW_CHAR *service_id;
    HW_CHAR *desired_event_time;
    HW_CHAR *desired_properties;
    HW_CHAR *reported_event_time;
    HW_CHAR *reported_properties;
    HW_INT version;
} EN_IOTA_SHADOW_DATA;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *object_device_id;
    HW_CHAR *request_id;
    EN_IOTA_SHADOW_DATA *shadow;
    HW_INT shadow_data_count;
    HW_CHAR *bridge_device_id;
} EN_IOTA_DEVICE_SHADOW;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *object_device_id;
    HW_CHAR *name;
    HW_CHAR *id;
    HW_CHAR *content;
    HW_CHAR *topic_para;
} EN_IOTA_USER_TOPIC_MESSAGE;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqttMsgInfo;
    HW_CHAR *objectDeviceId;
    HW_INT payloadLength;
    HW_CHAR *payload;
    HW_CHAR *topicPara;
} EN_IOTA_USER_TOPIC_RAW_MESSAGE;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_INT result_code;
    HW_CHAR *bridge_device_id;
    HW_CHAR *request_id;
} EN_IOTA_BRIDGES_LOGIN, EN_IOTA_BRIDGES_LOGOUT;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_INT result_code;
    HW_CHAR *bridge_device_id;
    HW_CHAR *paras;
    HW_CHAR *request_id;
    HW_CHAR *new_secret;
} EN_IOTA_BRIDGES_RESET_SECRET;

typedef struct {
    EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info;
    HW_CHAR *bridge_device_id;
} EN_IOTA_BRIDGES_PALLET_DISCONNECT;
typedef enum {
    EN_IOTA_CALLBACK_CONNECT_SUCCESS = 0,
    EN_IOTA_CALLBACK_CONNECT_FAILURE = 1,
    EN_IOTA_CALLBACK_DISCONNECT_SUCCESS = 2,
    EN_IOTA_CALLBACK_DISCONNECT_FAILURE = 3,
    EN_IOTA_CALLBACK_CONNECTION_LOST = 4,
    EN_IOTA_CALLBACK_PUBLISH_SUCCESS = 5,
    EN_IOTA_CALLBACK_PUBLISH_FAILURE = 6,
    EN_IOTA_CALLBACK_SUBSCRIBE_SUCCESS = 7,
    EN_IOTA_CALLBACK_SUBSCRIBE_FAILURE = 8
} EN_IOTA_CALLBACK_SETTING;

HW_API_FUNC HW_INT IOTA_Init(HW_CHAR *workPath);
HW_API_FUNC HW_INT IOTA_Destroy(void);
HW_API_FUNC HW_VOID IOTA_SetPrintLogCallback(PFN_LOG_CALLBACK_HANDLER pfnLogCallbackHandler);
HW_API_FUNC HW_INT IOTA_ConnectConfigSet(HW_CHAR *ip, HW_CHAR *port, HW_CHAR *deviceId, HW_CHAR *password);

typedef HW_VOID (*PFN_PROTOCOL_CALLBACK_HANDLER)(EN_IOTA_MQTT_PROTOCOL_RSP *message);
HW_API_FUNC HW_VOID IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_SETTING item,
    PFN_PROTOCOL_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_EVENT_CALLBACK_HANDLER)(EN_IOTA_EVENT *message);
HW_API_FUNC HW_VOID IOTA_SetEventCallback(PFN_EVENT_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_CMD_CALLBACK_HANDLER)(EN_IOTA_COMMAND *message);
HW_API_FUNC HW_VOID IOTA_SetCmdCallback(PFN_CMD_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_CMD_CALLBACK_HANDLER_V3)(EN_IOTA_COMMAND_V3 *message);
HW_API_FUNC HW_VOID IOTA_SetCmdCallbackV3(PFN_CMD_CALLBACK_HANDLER_V3 callbackHandler);

typedef HW_VOID (*PFN_MESSAGE_CALLBACK_HANDLER)(EN_IOTA_MESSAGE *message, void *mqttv5);
HW_API_FUNC HW_VOID IOTA_SetMessageCallback(PFN_MESSAGE_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_RAW_MESSAGE_CALLBACK_HANDLER)(EN_IOTA_RAW_MESSAGE *message, void *mqttv5);
HW_API_FUNC HW_VOID IOTA_SetRawMessageCallback(PFN_RAW_MESSAGE_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_PROP_SET_CALLBACK_HANDLER)(EN_IOTA_PROPERTY_SET *message);
HW_API_FUNC HW_VOID IOTA_SetPropSetCallback(PFN_PROP_SET_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_PROP_GET_CALLBACK_HANDLER)(EN_IOTA_PROPERTY_GET *message);
HW_API_FUNC HW_VOID IOTA_SetPropGetCallback(PFN_PROP_GET_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_SHADOW_GET_CALLBACK_HANDLER)(EN_IOTA_DEVICE_SHADOW *message);
HW_API_FUNC HW_VOID IOTA_SetShadowGetCallback(PFN_SHADOW_GET_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_USER_TOPIC_MSG_CALLBACK_HANDLER)(EN_IOTA_USER_TOPIC_MESSAGE *message);
HW_API_FUNC HW_VOID IOTA_SetUserTopicMsgCallback(PFN_USER_TOPIC_MSG_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_USER_TOPIC_RAW_MSG_CALLBACK_HANDLER)(EN_IOTA_USER_TOPIC_RAW_MESSAGE *message);
HW_API_FUNC HW_VOID IOTA_SetUserTopicRawMsgCallback(PFN_USER_TOPIC_RAW_MSG_CALLBACK_HANDLER pfnCallbackHandler);

typedef HW_VOID (*PFN_BOOTSTRAP_CALLBACK_HANDLER)(EN_IOTA_MQTT_PROTOCOL_RSP *message);
HW_API_FUNC HW_VOID IOTA_SetBootstrapCallback(PFN_BOOTSTRAP_CALLBACK_HANDLER callbackHandler);

typedef HW_INT (*PFN_DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER)(char *deviceId, char *message);
HW_API_FUNC HW_VOID IOTA_SetDeviceRuleSendMsgCallback(PFN_DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_M2M_CALLBACK_HANDLER)(EN_IOTA_M2M_MESSAGE *message);
HW_API_FUNC HW_VOID IOTA_SetM2mCallback(PFN_M2M_CALLBACK_HANDLER callbackHandler);

typedef HW_VOID (*PFN_BRIDGES_DEVICE_LOGIN)(EN_IOTA_BRIDGES_LOGIN *message);
HW_API_FUNC HW_VOID IOTA_SetBridgesDeviceLoginCallback(PFN_BRIDGES_DEVICE_LOGIN callbackHandler);

typedef HW_VOID (*PFN_BRIDGES_DEVICE_LOGOUT)(EN_IOTA_BRIDGES_LOGOUT *message);
HW_API_FUNC HW_VOID IOTA_SetBridgesDeviceLogoutCallback(PFN_BRIDGES_DEVICE_LOGOUT callbackHandler);

typedef HW_VOID (*PFN_BRIDGES_DEVICE_RESET_SECRET)(EN_IOTA_BRIDGES_RESET_SECRET *message);
HW_API_FUNC HW_VOID IOTA_SetBridgesDeviceResetSecretCallback(PFN_BRIDGES_DEVICE_RESET_SECRET callbackHandler);

typedef HW_VOID (*PFN_BRIDGES_DEVICE_PLATE_DISCONNECT)(EN_IOTA_BRIDGES_PALLET_DISCONNECT *message);
HW_API_FUNC HW_VOID IOTA_SetBridgesDeviceDisConnCallback(PFN_BRIDGES_DEVICE_PLATE_DISCONNECT callbackHandler);

typedef HW_VOID (*PFN_UNDEFINED_MSG_CALLBACK_HANDLER)(EN_IOTA_UNDEFINED_MESSAGE *message);
HW_API_FUNC HW_VOID IOTA_SetUndefinedMessageCallback(PFN_UNDEFINED_MSG_CALLBACK_HANDLER callbackHandler);

typedef void (*PFN_EVENT_CALLBACK_HANDLER_SPECIFY)(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message);
HW_API_FUNC HW_VOID IOTA_SetEvenSubDeviceCallback(PFN_EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);
HW_API_FUNC HW_VOID IOTA_SetNtpCallback(PFN_EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);
HW_API_FUNC HW_VOID IOTA_SetEvenDeviceLogCallback(PFN_EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);
HW_API_FUNC HW_VOID IOTA_SetEvenSoftBusCallback(PFN_EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);
HW_API_FUNC HW_VOID IOTA_SetEvenTunnelManagerCallback(PFN_EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);
HW_API_FUNC HW_VOID IOTA_SetEvenFileManagerCallback(PFN_EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);

typedef void (*PFN_OTAVERSION_CALLBACK_HANDLER_SPECIFY)(char *objectDeviceId);
HW_API_FUNC HW_VOID IOTA_SetEvenOtaVersionUpCallback(PFN_OTAVERSION_CALLBACK_HANDLER_SPECIFY callbackHandler);

typedef void (*PFN_OTAURL_CALLBACK_HANDLER_SPECIFY)(char *objectDeviceId, int event_type, EN_IOTA_OTA_PARAS *ota_paras);
HW_API_FUNC HW_VOID IOTA_SetEvenOtaUrlResponseCallback(PFN_OTAURL_CALLBACK_HANDLER_SPECIFY callbackHandler);

/**
 * @Description: device configure callback function
 * @param cfg: Configuration data in JSON format
 * @param description: Output parameter. Configuration data processing result description,
 *                     which is reported through event/up
 */
typedef int (*PFN_DEVICE_CONFIG_CALLBACK_HANDLER)(JSON *cfg, char *description);
HW_API_FUNC HW_VOID IOTA_SetDeviceConfigCallback(PFN_DEVICE_CONFIG_CALLBACK_HANDLER callbackHandler);
/**
 * @Description: load rule from filepath
 * @param filepath: the path of file to read the rule, also be saved for reading rule later
 */
HW_API_FUNC HW_VOID IOTA_EnableDeviceRuleStorage(const char *filepath);


typedef enum {
    EN_IOTA_EVENT_SUB_DEVICE_MANAGER = 0,
    EN_IOTA_EVENT_OTA = 1,
    EN_IOTA_EVENT_TIME_SYNC = 2,
    EN_IOTA_EVENT_DEVICE_LOG = 3,
    EN_IOTA_EVENT_FILE_MANAGER = 4,
    // EN_IOTA_EVENT_SDK_INFO = 5,
    EN_IOTA_EVENT_DEVICE_RULE = 6,
    EN_IOTA_EVENT_TUNNEL_MANAGER = 7,
    EN_IOTA_EVENT_DEVICE_CONFIG = 8,
    EN_IOTA_EVENT_SOFT_BUS = 9,
    EN_IOTA_EVENT_ERROR = -1
} EN_IOTA_EVENT_SERVICE_ID;

typedef enum {
    EN_IOTA_EVENT_ADD_SUB_DEVICE_NOTIFY = 0,
    EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY = 1,
    EN_IOTA_EVENT_VERSION_QUERY = 2,
    EN_IOTA_EVENT_FIRMWARE_UPGRADE = 3,
    EN_IOTA_EVENT_SOFTWARE_UPGRADE = 4,
    EN_IOTA_EVENT_GET_TIME_SYNC_RESPONSE = 5,
    EN_IOTA_EVENT_ADD_SUB_DEVICE_RESPONSE = 6,
    EN_IOTA_EVENT_DEL_SUB_DEVICE_RESPONSE = 7,
    EN_IOTA_EVENT_LOG_CONFIG = 8,
    EN_IOTA_EVENT_GET_UPLOAD_URL_RESPONSE = 9,
    EN_IOTA_EVENT_GET_DOWNLOAD_URL_RESPONSE = 10,
    EN_IOTA_EVENT_TUNNEL_NOTIFY = 11,
    EN_IOTA_EVENT_DEVICE_CONFIG_UPDATE = 12,
    EN_IOTA_EVENT_FIRMWARE_UPGRADE_V2 = 13,
    EN_IOTA_EVENT_SOFTWARE_UPGRADE_V2 = 14,
    EN_IOTA_EVENT_UPDATE_SUB_DEVICE_RESPONSE = 15,
    EN_IOTA_EVENT_TYPE_ERROR = -1
} EN_IOTA_EVENT_TYPE;

#endif

