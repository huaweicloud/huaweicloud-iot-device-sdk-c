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

#endif /* CALLBACK_FUNC_H */
