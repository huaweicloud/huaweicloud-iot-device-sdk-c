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

typedef enum {
	EN_CALLBACK_CONNECT_SUCCESS = 0,
	EN_CALLBACK_CONNECT_FAILURE = 1,
	EN_CALLBACK_DISCONNECT_SUCCESS = 2,
	EN_CALLBACK_DISCONNECT_FAILURE = 3,
	EN_CALLBACK_CONNECTION_LOST = 4,
	EN_CALLBACK_PUBLISH_SUCCESS = 5,
	EN_CALLBACK_PUBLISH_FAILURE = 6,
	EN_CALLBACK_SUBSCRIBE_SUCCESS = 7,
	EN_CALLBACK_SUBSCRIBE_FAILURE = 8,
	EN_CALLBACK_MESSAGE_DOWN = 9,
	EN_CALLBACK_COMMAND_REQUEST = 10,
	EN_CALLBACK_PROPERTIES_SET = 11,
	EN_CALLBACK_PROPERTIES_GET = 12,
	EN_CALLBACK_EVENT_DOWN = 14,
	EN_CALLBACK_COMMAND_ARRIVED = 15,
	EN_CALLBACK_USER_TOPIC = 16,
	EN_CALLBACK_DEVICE_SHADOW = 17
} EN_CALLBACK_SETTING;

/**
 * @param context the application-self defined parameter, current is NULL
 * @param token the inner messageId(1-65535) of the uplink message in this MQTT SDK
 * @param code the reason code in the failure callback
 * @param message the message can be from the IoT platform (e.g. command content) or from the MQTT SDK (e.g. failure explanation message)
 */
typedef void (*CALLBACK_HANDLER)(void *context, int token, int code, char *message);
typedef void (*CALLBACK_HANDLER_WITH_TOPIC)(void *context, int token, int code, char *message, char *topicParas);
typedef void (*LOG_CALLBACK_HANDLER)(int level, char *format, va_list args);

void OnLoginSuccess(void *context, int token, int code, char *message);
void OnMessageArrived(void *context, int token, int code, const char *topic, char *message);

_DLLEXPORT int SetCallback(int item, CALLBACK_HANDLER callbackHandler);
_DLLEXPORT void SetLogCallback(LOG_CALLBACK_HANDLER logCallbackHandler);
_DLLEXPORT int SetCallbackWithTopic(int item, CALLBACK_HANDLER_WITH_TOPIC callbackHandler);

#endif /* INCLUDE_SERVICE_CALLBACK_FUNC_H_ */
