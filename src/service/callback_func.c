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

#include "string_util.h"
#include "log_util.h"
#include "mqtt_base.h"
#include "base.h"
#include "callback_func.h"
#include "subscribe.h"
#include "string.h"
#include "iota_error_type.h"

CALLBACK_HANDLER onAuthS;
CALLBACK_HANDLER onMessageDown;
CALLBACK_HANDLER_WITH_TOPIC onCommandReq;
CALLBACK_HANDLER_WITH_TOPIC onPropSet;
CALLBACK_HANDLER_WITH_TOPIC onPropGet;
CALLBACK_HANDLER_WITH_TOPIC onDeviceShadowRsp;
CALLBACK_HANDLER onSubDeviceMessageDown;
CALLBACK_HANDLER_WITH_TOPIC onUserMessageDown;

void OnLoginSuccess(void *context, int token, int code, char *message) {
	SubscribeAll();
	if (onAuthS) {
		(onAuthS)(context, token, code, message);
	}
}

void OnMessageArrived(void *context, int token, int code, const char *topic, char *message) {

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_MESSAGEDOWN)) > 0) {
		if (onMessageDown) {
			(onMessageDown)(context, token, code, message);
		}
		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_COMMAND)) > 0) {
		char *requestId_value = strstr(topic, "=");
		char *requestId = strstr(requestId_value + 1, "");
		if (onCommandReq) {
			(onCommandReq)(context, token, code, message, requestId);
		}
		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_SET)) > 0) {
		char *requestId_value = strstr(topic, "=");
		char *requestId = strstr(requestId_value + 1, "");
		if (onPropSet) {
			(onPropSet)(context, token, code, message, requestId);
		}
		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_GET)) > 0) {
		char *requestId_value = strstr(topic, "=");
		char *requestId = strstr(requestId_value + 1, "");
		if (onPropGet) {
			(onPropGet)(context, token, code, message, requestId);
		}
		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_RSP)) > 0) {
		char *requestId_value = strstr(topic, "=");
		char *requestId = strstr(requestId_value + 1, "");
		if (onDeviceShadowRsp) {
			(onDeviceShadowRsp)(context, token, code, message, requestId);
		}
		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_EVENT_DOWN)) > 0) {
		if (onSubDeviceMessageDown) {
			(onSubDeviceMessageDown)(context, token, code, message);
		}
		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_USER)) > 0) {
		char *topicParas_value = strstr(topic, "user/");
		char *topicParas = strstr(topicParas_value + 5, "");
		if (onUserMessageDown) {
			(onUserMessageDown)(context, token, code, message, topicParas);
		}
		return;
	}

}

int SetCallback(int item, CALLBACK_HANDLER callbackHandler) {
	switch (item) {
		case EN_CALLBACK_CONNECT_SUCCESS:
			onAuthS = callbackHandler;
			return MqttBase_SetCallback(item, OnLoginSuccess);
		case EN_CALLBACK_MESSAGE_DOWN:
			onMessageDown = callbackHandler;
			return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
		case EN_CALLBACK_EVENT_DOWN:
			onSubDeviceMessageDown = callbackHandler;
			return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
		default:
			return MqttBase_SetCallback(item, callbackHandler);
	}
}

int SetCallbackWithTopic(int item, CALLBACK_HANDLER_WITH_TOPIC callbackHandler) {
	switch (item) {
		case EN_CALLBACK_COMMAND_REQUEST:
			onCommandReq = callbackHandler;
			return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
		case EN_CALLBACK_PROPERTIES_SET:
			onPropSet = callbackHandler;
			return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
		case EN_CALLBACK_PROPERTIES_GET:
			onPropGet = callbackHandler;
			return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
		case EN_CALLBACK_USER_TOPIC:
			onUserMessageDown = callbackHandler;
			return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
		case EN_CALLBACK_DEVICE_SHADOW:
			onDeviceShadowRsp = callbackHandler;
			return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);

		default:
			PrintfLog(EN_LOG_LEVEL_WARNING, "Callback: SetCallbackWithTopic() warning, the item (%d) to be set is not available\n", item);
			return IOTA_RESOURCE_NOT_AVAILABLE;
	}
}

void SetLogCallback(LOG_CALLBACK_HANDLER logCallbackHandler) {
	SetPrintfLogCallback(logCallbackHandler);
}
