/*Copyright (c) <2020>, <Huawei Technologies Co., Ltd>
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
 * */

#ifndef INCLUDE_SERVICE_CALLBACK_FUNC_H_
#define INCLUDE_SERVICE_CALLBACK_FUNC_H_

#include "stdarg.h"
#include "iota_init.h"

typedef void (*LOG_CALLBACK_HANDLER)(int level, char *format, va_list args);

#define MAX_EVENT_COUNT 3
#define MAX_EVENT_DEVICE_COUNT 10
#define MAX_SERVICE_COUNT 50

void OnLoginSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void OnMessageArrived(void *context, int token, int code, const char *topic, char *message, void *mqttv5);

void SetLogCallback(LOG_CALLBACK_HANDLER logCallbackHandler);

typedef void (*EVENT_CALLBACK_HANDLER)(EN_IOTA_EVENT *message);
int SetEventCallback(EVENT_CALLBACK_HANDLER pfnCallbackHandler);

typedef void (*CMD_CALLBACK_HANDLER)(EN_IOTA_COMMAND *message);
int SetCmdCallback(CMD_CALLBACK_HANDLER pfnCallbackHandler);

typedef void (*CMD_CALLBACK_HANDLER_V3)(EN_IOTA_COMMAND_V3 *message);
int SetCmdCallbackV3(CMD_CALLBACK_HANDLER_V3 pfnCallbackHandler);

typedef void (*PROTOCOL_CALLBACK_HANDLER)(EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp);
int SetProtocolCallback(int item, PROTOCOL_CALLBACK_HANDLER pfnCallbackHandler);

typedef void (*MESSAGE_CALLBACK_HANDLER)(EN_IOTA_MESSAGE *protocolRsp, void *mqttv5);
int SetMessageCallback(MESSAGE_CALLBACK_HANDLER pfnCallbackHandler);

typedef void (*PROP_SET_CALLBACK_HANDLER)(EN_IOTA_PROPERTY_SET *rsp);
int SetPropSetCallback(PROP_SET_CALLBACK_HANDLER pfnCallbackHandler);

typedef void (*PROP_GET_CALLBACK_HANDLER)(EN_IOTA_PROPERTY_GET *rsp);
int SetPropGetCallback(PROP_GET_CALLBACK_HANDLER pfnCallbackHandler);

typedef void (*SHADOW_GET_CALLBACK_HANDLER)(EN_IOTA_DEVICE_SHADOW *rsp);
int SetShadowGetCallback(SHADOW_GET_CALLBACK_HANDLER pfnCallbackHandler);

typedef void (*USER_TOPIC_MSG_CALLBACK_HANDLER)(EN_IOTA_USER_TOPIC_MESSAGE *rsp);
int SetUserTopicMsgCallback(USER_TOPIC_MSG_CALLBACK_HANDLER pfnCallbackHandler);

typedef void (*BOOTSTRAP_CALLBACK_HANDLER)(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
int SetBootstrapCallback(BOOTSTRAP_CALLBACK_HANDLER pfnCallbackHandler);

typedef void (*DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER)(char *deviceId, char *cmd);
void SetDeviceRuleSendMsgCallback(DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER pfnCallbackHandler);

#endif /* INCLUDE_SERVICE_CALLBACK_FUNC_H_ */
