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

#ifndef CALLBACK_FUNC_H
#define CALLBACK_FUNC_H

#include <stdarg.h>
#include "iota_init.h"
#include "json_util.h"
#include "base.h"

typedef void (*LOG_CALLBACK_HANDLER)(int level, char *format, va_list args);

#define MAX_EVENT_COUNT 3
#define MAX_EVENT_DEVICE_COUNT 10
#define MAX_SERVICE_COUNT 50


void SetLogCallback(LOG_CALLBACK_HANDLER logCallbackHandler);

typedef void (*EVENT_CALLBACK_HANDLER)(EN_IOTA_EVENT *message);
void SetEventCallback(EVENT_CALLBACK_HANDLER callbackHandler);

typedef void (*CMD_CALLBACK_HANDLER)(EN_IOTA_COMMAND *message);
void SetCmdCallback(CMD_CALLBACK_HANDLER callbackHandler);

typedef void (*CMD_CALLBACK_HANDLER_V3)(EN_IOTA_COMMAND_V3 *message);
void SetCmdCallbackV3(CMD_CALLBACK_HANDLER_V3 callbackHandler);

typedef void (*PROTOCOL_CALLBACK_HANDLER)(EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp);
void SetProtocolCallback(EN_CALLBACK_SETTING item, PROTOCOL_CALLBACK_HANDLER callbackHandler);

typedef void (*MESSAGE_CALLBACK_HANDLER)(EN_IOTA_MESSAGE *protocolRsp, void *mqttv5);
void SetMessageCallback(MESSAGE_CALLBACK_HANDLER callbackHandler);

typedef void (*RAW_MESSAGE_CALLBACK_HANDLER)(EN_IOTA_RAW_MESSAGE *rsp, void *mqttv5);
void SetRawMessageCallback(RAW_MESSAGE_CALLBACK_HANDLER callbackHandler);

typedef void (*PROP_SET_CALLBACK_HANDLER)(EN_IOTA_PROPERTY_SET *rsp);
void SetPropSetCallback(PROP_SET_CALLBACK_HANDLER callbackHandler);

typedef void (*PROP_GET_CALLBACK_HANDLER)(EN_IOTA_PROPERTY_GET *rsp);
void SetPropGetCallback(PROP_GET_CALLBACK_HANDLER callbackHandler);

typedef void (*SHADOW_GET_CALLBACK_HANDLER)(EN_IOTA_DEVICE_SHADOW *rsp);
void SetShadowGetCallback(SHADOW_GET_CALLBACK_HANDLER callbackHandler);

typedef void (*USER_TOPIC_MSG_CALLBACK_HANDLER)(EN_IOTA_USER_TOPIC_MESSAGE *rsp);
void SetUserTopicMsgCallback(USER_TOPIC_MSG_CALLBACK_HANDLER callbackHandler);

typedef void (*USER_TOPIC_RAW_MSG_CALLBACK_HANDLER)(EN_IOTA_USER_TOPIC_RAW_MESSAGE *rsp);
void SetUserTopicRawMsgCallback(USER_TOPIC_RAW_MSG_CALLBACK_HANDLER callbackHandler);

typedef void (*BOOTSTRAP_CALLBACK_HANDLER)(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void SetBootstrapCallback(BOOTSTRAP_CALLBACK_HANDLER callbackHandler);

typedef HW_INT (*DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER)(char *deviceId, char *cmd);
void SetDeviceRuleSendMsgCallback(DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER callbackHandler);

typedef void (*M2M_CALLBACK_HANDLER)(EN_IOTA_M2M_MESSAGE *rsp);
void SetM2mCallback(M2M_CALLBACK_HANDLER callbackHandler);

typedef int (*DEVICE_CONFIG_CALLBACK_HANDLER)(JSON *cfg, char *description);
void SetDeviceConfigCallback(DEVICE_CONFIG_CALLBACK_HANDLER callbackHandler);

typedef void (*BRIDGES_DEVICE_LOGIN)(EN_IOTA_BRIDGES_LOGIN *rsp);
void SetBridgesDeviceLoginCallback(BRIDGES_DEVICE_LOGIN callbackHandler);

typedef void (*BRIDGES_DEVICE_LOGOUT)(EN_IOTA_BRIDGES_LOGOUT *rsp);
void SetBridgesDeviceLogoutCallback(BRIDGES_DEVICE_LOGOUT callbackHandler);

typedef void (*BRIDGES_DEVICE_RESET_SECRET)(EN_IOTA_BRIDGES_RESET_SECRET *rsp);
void SetBridgesDeviceResetSecretCallback(BRIDGES_DEVICE_RESET_SECRET callbackHandler);

typedef void (*BRIDGES_DEVICE_PLATE_DISCONNECT)(EN_IOTA_BRIDGES_PALLET_DISCONNECT *rsp);
void SetBridgesDevicePalletDisConnCallback(BRIDGES_DEVICE_PLATE_DISCONNECT callbackHandler);

typedef void (*UNDEFINED_MESSAGE_CALLBACK_HANDLER)(EN_IOTA_UNDEFINED_MESSAGE *rsp);
void SetUndefinedMessageCallback(UNDEFINED_MESSAGE_CALLBACK_HANDLER callbackHandler);

typedef void (*EVENT_CALLBACK_HANDLER_SPECIFY)(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message);
void SetEvenSubDeviceCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);
void SetNtpCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);
void SetEvenDeviceLogCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);
void SetEvenSoftBusCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);
void SetEvenTunnelManagerCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);
void SetEvenFileManagerCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler);

typedef void (*OTAVERSION_CALLBACK_HANDLER_SPECIFY)(char *objectDeviceId);
void SetEvenOtaVersionUpCallback(OTAVERSION_CALLBACK_HANDLER_SPECIFY callbackHandler);

typedef void (*OTAURL_CALLBACK_HANDLER_SPECIFY)(char *objectDeviceId, int event_type, EN_IOTA_OTA_PARAS *ota_paras);
void SetEvenOtaUrlResponseCallback(OTAURL_CALLBACK_HANDLER_SPECIFY callbackHandler);

// 事件细分回调函数
typedef struct {
    // get information of memory
    void (*onSubDevice)(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message); // 子设备回调函数
    // void (*onOTA)(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message); // ota回调
    void (*onNTP)(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message);  // 时间回调
    void (*onDeviceLog)(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message); // 设备信息
    void (*onSoftBus)(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message); //软总线
    void (*onTunnelManager)(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message); // 远程登录
    void (*onFileManager)(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message); // 文件上传下载
    void (*onDeviceRule)(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message); // 规则引擎
} TagEventsOps;

void SetTagEventsOps(TagEventsOps ops);

// ota回调细化
typedef struct {
    void (*onDeviceVersionUp)(char *objectDeviceId); // ota版本上报回调
    void (*onUrlResponse)(char *objectDeviceId, HW_INT event_type, EN_IOTA_OTA_PARAS *ota_paras); // ota获取平台url回调
} TagOtaOps;

#endif /* CALLBACK_FUNC_H */
