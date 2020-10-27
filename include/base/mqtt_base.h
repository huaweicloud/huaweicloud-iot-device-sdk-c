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

#ifndef INCLUDE_MQTT_BASE_H_
#define INCLUDE_MQTT_BASE_H_

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#include "MQTTAsync.h"
#include "iota_init.h"

#define ENCRYPT_LENGTH 					32
#define PASSWORD_ENCRYPT_LENGTH 		64
#define DEFAULT_QOS 					1
#define DEFAULT_KEEP_ALIVE_INTERVAL 	120
#define DEFAULT_CONNECT_TIME_OUT 		30
#define DEFAULT_RETRYINTERVAL 			10
#define TCP_URL_PREFIX  				"tcp://"
#define SSL_URL_PREFIX  				"ssl://"
#define MQTT_PORT         				"1883"
#define MQTTS_PORT         				"8883"

typedef enum {
	EN_MQTT_BASE_CALLBACK_CONNECT_SUCCESS = 0,
	EN_MQTT_BASE_CALLBACK_CONNECT_FAILURE = 1,
	EN_MQTT_BASE_CALLBACK_DISCONNECT_SUCCESS = 2,
	EN_MQTT_BASE_CALLBACK_DISCONNECT_FAILURE = 3,
	EN_MQTT_BASE_CALLBACK_CONNECTION_LOST = 4,
	EN_MQTT_BASE_CALLBACK_PUBLISH_SUCCESS = 5,
	EN_MQTT_BASE_CALLBACK_PUBLISH_FAILURE = 6,
	EN_MQTT_BASE_CALLBACK_SUBSCRIBE_SUCCESS = 7,
	EN_MQTT_BASE_CALLBACK_SUBSCRIBE_FAILURE = 8,
	EN_MQTT_BASE_CALLBACK_MESSAGE_ARRIVED = 15 //the same value as EN_CALLBACK_COMMAND_ARRIVED in Callback.h
} EN_MQTT_BASE_CALLBACK_SETTING;

//see also ENUM_BASE_CONFIG_SETTING in Base.h
typedef enum {
	EN_MQTT_BASE_CONFIG_USERNAME = 0,
	EN_MQTT_BASE_CONFIG_PASSWORD = 1,
	EN_MQTT_BASE_CONFIG_SERVER_IP = 2,
	EN_MQTT_BASE_CONFIG_SERVER_PORT = 3,
	EN_MQTT_BASE_CONFIG_AUTH_MODE = 4,
	EN_MQTT_BASE_CONFIG_LOG_LOCAL_NUMBER = 5,
	EN_MQTT_BASE_CONFIG_LOG_LEVEL = 6,
	EN_MQTT_BASE_CONFIG_KEEP_ALIVE_TIME = 7,
	EN_MQTT_BASE_CONFIG_CONNECT_TIMEOUT = 8,
	EN_MQTT_BASE_CONFIG_RETRY_INTERVAL = 9,
	EN_MQTT_BASE_CONFIG_RESET_SECRET_IN_PROGRESS = 10,
	EN_MQTT_BASE_CONFIG_QOS = 11,
	EN_MQTT_BASE_PRIVATE_KEY_PASSWORD = 12,
	EN_MQTT_BASE_BS_SCOPE_ID = 13,
	EN_MQTT_BASE_BS_MODE = 14
} ENUM_MQTT_BASE_CONFIG;

typedef void (*MQTT_BASE_CALLBACK_HANDLER)(EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp);
typedef void (*MQTT_BASE_CALLBACK_HANDLER_WITH_TOPIC)(void *context, int token, int code, const char *topic, char *message);
typedef void MqttBase_connectionLost(void *context, char *cause);
typedef void MqttBase_deliveryComplete(void *context, int token);
typedef void MqttBase_messageArrived(void *context, char *topic, char *message);
typedef void MqttBase_commonCallback(void *context, int token, char *message);

int GetEncryptedPassword(char **timestamp, char **encryptedPwd);
void HandleCallbackFailure(const char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void *context, MQTTAsync_failureData *response);
void HandleCallbackSuccess(const char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void *context, MQTTAsync_successData *response);
void MqttBase_OnConnectSuccess(void *context, MQTTAsync_successData *response);
void MqttBase_OnConnectFailure(void *context, MQTTAsync_failureData *response);
void MqttBase_OnDisconnectSuccess(void *context, MQTTAsync_successData *response);
void MqttBase_OnDisconnectFailure(void *context, MQTTAsync_failureData *response);
void MqttBase_OnConnectionLost(void *context, char *cause);
void MqttBase_OnSubscribeSuccess(void *context, MQTTAsync_successData *response);
void MqttBase_OnSubscribeFailure(void *context, MQTTAsync_failureData *response);
void MqttBase_OnPublishSuccess(void *context, MQTTAsync_successData *response);
void MqttBase_OnPublishFailure(void *context, MQTTAsync_failureData *response);
int MqttBase_OnMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message);
int MqttBase_init(char *workPath);
int MqttBase_SetConfig(int item, char *value);
char* MqttBase_GetConfig(int item);
int MqttBase_SetCallback(int item, MQTT_BASE_CALLBACK_HANDLER handler);
int MqttBase_SetCallbackWithTopic(int item, MQTT_BASE_CALLBACK_HANDLER_WITH_TOPIC handler);
int MqttBase_CreateConnection(void);
int MqttBase_ReleaseConnection(void);
int MqttBase_subscribe(const char *topic, const int qos);
int MqttBase_publish(const char *topic, void *payload, int len, void* context);
int MqttBase_StringLength(char *str);
int MqttBase_destory(void);

#endif

