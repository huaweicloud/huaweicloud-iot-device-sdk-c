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

#if defined(WIN32) || defined(WIN64)
#include "windows.h"
#include "io.h"
#else
#include "unistd.h"
#endif

#include <stdlib.h>
#include <pthread.h>
#include "string_util.h"
#include "log_util.h"
#include "mqtt_base.h"
#include "hmac_sha256.h"
#include "iota_error_type.h"

#if defined(WIN32) || defined(WIN64)
typedef int (*pMQTTAsync_create)(MQTTAsync *handle, const char *serverURI, const char *clientId, int persistence_type, void *persistence_context);
typedef int (*pMQTTAsync_connect)(MQTTAsync handle, const MQTTAsync_connectOptions *options);
typedef int (*pMQTTAsync_send)(MQTTAsync handle, const char *destinationName, int payloadlen, const void *payload, int qos, int retained, MQTTAsync_responseOptions *response);
typedef int (*pMQTTAsync_sendMessage)(MQTTAsync handle, const char *destinationName, const MQTTAsync_message *msg, MQTTAsync_responseOptions *response);
typedef int (*pMQTTAsync_setCallbacks)(MQTTAsync handle, void *context, MQTTAsync_connectionLost *cl, MQTTAsync_messageArrived *ma, MQTTAsync_deliveryComplete *dc);
typedef int (*pMQTTAsync_subscribe)(MQTTAsync handle, const char *topic, int qos, MQTTAsync_responseOptions *response);
typedef void (*pMQTTAsync_freeMessage)(MQTTAsync_message **msg);
typedef void (*pMQTTAsync_free)(void *ptr);
typedef int (*pMQTTAsync_disconnect)(MQTTAsync handle, const MQTTAsync_disconnectOptions *options);
typedef int (*pMQTTAsync_isConnected)(MQTTAsync handle);
typedef void (*pMQTTAsync_destroy)(MQTTAsync *handle);
#endif

pthread_mutex_t login_locker = PTHREAD_MUTEX_INITIALIZER;

int gQOS = DEFAULT_QOS;  //default value of qos is 1
int keepAliveInterval = DEFAULT_KEEP_ALIVE_INTERVAL; //default value of keepAliveInterval is 120s
int connectTimeout = DEFAULT_CONNECT_TIME_OUT; //default value of connect timeout is 30s
int retryInterval = DEFAULT_RETRYINTERVAL; //default value of connect retryInterval is 10s

char *serverIp = NULL;
char *port = NULL;
char *username = NULL; //deviceId
char *password = NULL;
int authMode = 0;
char *urlPrefix = TCP_URL_PREFIX;
char *bs_scope_id = NULL;
int bs_reg_mode = 0;

char *workDir = NULL;
char *logDir = NULL;

int initFlag = 0;
int mqttClientCreateFlag = 0; //this mqttClientCreateFlag is used to control the invocation of MQTTAsync_create, otherwise, there would be message leak.

char *ca_path = NULL;
char *cert_path = NULL;
char *key_path = NULL;
char *privateKeyPassword = NULL; //privateKeyPassword in cert mode device

MQTTAsync client = NULL;

#if defined(WIN32) || defined(WIN64)
HMODULE mqttdll = NULL;
#endif

char *encrypted_password = NULL;

int GetEncryptedPassword(char **timestamp, char **encryptedPwd) {
	if (password == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: GetEncryptedPassword() error, password is NULL, please set the required parameters properly\n");
		return IOTA_PARAMETER_EMPTY;
	}

	char *temp_pwd = NULL;
	if (CopyStrValue(&temp_pwd, (const char*) password, StringLength(password)) < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "GetEncryptedPassword(): there is not enough memory here.\n");
		return IOTA_FAILURE;
	}

	char *temp_encrypted_pwd = NULL;
	StringMalloc(&temp_encrypted_pwd, PASSWORD_ENCRYPT_LENGTH + 1);
	if (temp_encrypted_pwd == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: GetEncryptedPassword() error, there is not enough memory here.\n");
		MemFree(&temp_pwd);
		return IOTA_FAILURE;
	}

	int ret = EncryWithHMacSha256(temp_pwd, timestamp, ENCRYPT_LENGTH, temp_encrypted_pwd);
	if (ret != IOTA_SUCCESS) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: GetEncryptedPassword() error, encrypt failed %d\n", ret);
		MemFree(&temp_pwd);
		MemFree(&temp_encrypted_pwd);
		return IOTA_SECRET_ENCRYPT_FAILED;
	}

	if (CopyStrValue(encryptedPwd, (const char*) temp_encrypted_pwd, PASSWORD_ENCRYPT_LENGTH) < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "GetEncryptedPassword(): there is not enough memory here.\n");
		MemFree(&temp_pwd);
		MemFree(&temp_encrypted_pwd);
		return IOTA_FAILURE;
	}

	MemFree(&temp_pwd);
	MemFree(&temp_encrypted_pwd);
	return IOTA_SUCCESS;
}

void HandleCallbackFailure(const char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void *context, MQTTAsync_failureData *response) {

	EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp = (EN_IOTA_MQTT_PROTOCOL_RSP*)malloc(sizeof(EN_IOTA_MQTT_PROTOCOL_RSP));

	if (protocolRsp == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
		return;
	}

	protocolRsp->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
	if (protocolRsp->mqtt_msg_info == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
		MemFree(&protocolRsp);
		return;
	}

	if (response) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: %s error, messageId %d, code %d, message %s\n", currentFunctionName, response->token, response->code, response->message);

		protocolRsp->mqtt_msg_info->context = context;
		protocolRsp->mqtt_msg_info->messageId = response->token;
		protocolRsp->mqtt_msg_info->code = response->code;

		protocolRsp->message = (char*) response->message;

	} else {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: %s error, response is NULL\n", currentFunctionName);


		protocolRsp->mqtt_msg_info->context = context;
		protocolRsp->mqtt_msg_info->messageId = 0;
		protocolRsp->mqtt_msg_info->code = IOTA_FAILURE;

		protocolRsp->message = NULL;
	}

	if (callback) {
		(callback)(protocolRsp);
	}

	MemFree(&protocolRsp->mqtt_msg_info);
	MemFree(&protocolRsp);

}

void HandleCallbackSuccess(const char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void *context, MQTTAsync_successData *response) {
	PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: %s messageId %d\n", currentFunctionName, response ? response->token : -1);

	EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp = (EN_IOTA_MQTT_PROTOCOL_RSP*)malloc(sizeof(EN_IOTA_MQTT_PROTOCOL_RSP));

	if (protocolRsp == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
		return;
	}

	protocolRsp->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
	if (protocolRsp->mqtt_msg_info == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
		MemFree(&protocolRsp);
		return;
	}
	protocolRsp->mqtt_msg_info->context = context;
	protocolRsp->mqtt_msg_info->messageId = response ? response->token : 0;
	protocolRsp->mqtt_msg_info->code = IOTA_SUCCESS;

	protocolRsp->message = NULL;

	if (callback) {
		(callback)(protocolRsp);
	}

	MemFree(&protocolRsp->mqtt_msg_info);
	MemFree(&protocolRsp);
}

MQTT_BASE_CALLBACK_HANDLER onConnectS; //in order not to be duplicated with the callback function name set by the application
MQTT_BASE_CALLBACK_HANDLER onConnectF;
MQTT_BASE_CALLBACK_HANDLER onDisconnectS;
MQTT_BASE_CALLBACK_HANDLER onDisconnectF;
MQTT_BASE_CALLBACK_HANDLER onConnectionL;
MQTT_BASE_CALLBACK_HANDLER onSubscribeS;
MQTT_BASE_CALLBACK_HANDLER onSubscribeF;
MQTT_BASE_CALLBACK_HANDLER onPublishS;
MQTT_BASE_CALLBACK_HANDLER onPublishF;
MQTT_BASE_CALLBACK_HANDLER_WITH_TOPIC onMessageA;

void MqttBase_OnConnectSuccess(void *context, MQTTAsync_successData *response) {
	HandleCallbackSuccess("MqttBase_OnConnectSuccess()", onConnectS, context, response);
}

void MqttBase_OnConnectFailure(void *context, MQTTAsync_failureData *response) {
	HandleCallbackFailure("MqttBase_OnConnectFailure()", onConnectF, context, response);
}

void MqttBase_OnDisconnectSuccess(void *context, MQTTAsync_successData *response) {
	HandleCallbackSuccess("MqttBase_OnDisconnectSuccess()", onDisconnectS, context, response);
}

void MqttBase_OnDisconnectFailure(void *context, MQTTAsync_failureData *response) {
	HandleCallbackFailure("MqttBase_OnDisconnectFailure()", onDisconnectF, context, response);
}

void MqttBase_OnConnectionLost(void *context, char *cause) {
	PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_OnConnectionLost() error, cause: %s\n", cause);

	EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp = (EN_IOTA_MQTT_PROTOCOL_RSP*)malloc(sizeof(EN_IOTA_MQTT_PROTOCOL_RSP));

	if (protocolRsp == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
		return;
	}

	protocolRsp->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
	if (protocolRsp->mqtt_msg_info == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
		MemFree(&protocolRsp);
		return;
	}

	protocolRsp->mqtt_msg_info->context = context;
	protocolRsp->mqtt_msg_info->messageId = 0;
	protocolRsp->mqtt_msg_info->code = IOTA_FAILURE;

	protocolRsp->message = cause;

	if (onConnectionL) {
		(onConnectionL)(protocolRsp);
	}

	MemFree(&protocolRsp->mqtt_msg_info);
	MemFree(&protocolRsp);

}

void MqttBase_OnSubscribeSuccess(void *context, MQTTAsync_successData *response) {
	HandleCallbackSuccess("MqttBase_OnSubscribeSuccess()", onSubscribeS, context, response);
}

void MqttBase_OnSubscribeFailure(void *context, MQTTAsync_failureData *response) {
	HandleCallbackFailure("MqttBase_OnSubscribeFailure()", onSubscribeF, context, response);
}

void MqttBase_OnPublishSuccess(void *context, MQTTAsync_successData *response) {
	HandleCallbackSuccess("MqttBase_OnPublishSuccess()", onPublishS, context, response);
}

void MqttBase_OnPublishFailure(void *context, MQTTAsync_failureData *response) {
	HandleCallbackFailure("MqttBase_OnPublishFailure()", onPublishF, context, response);
}

int MqttBase_OnMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
	PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_OnMessageArrived() -------------> \n");
	char *temp_topic = NULL;
	char *temp_payload = NULL;

	if (CopyStrValue(&temp_topic, (const char*) topicName, topicLen) < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_OnMessageArrived() error, there is not enougth memory here.\n");
		MQTTAsync_freeMessage(&message);
		MQTTAsync_free(topicName);
		return -1;
	}
	if (CopyStrValue(&temp_payload, (const char*) message->payload, message->payloadlen) < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_OnMessageArrived() error, there is not enougth memory here.\n");
		MQTTAsync_freeMessage(&message);
		MQTTAsync_free(topicName);
		MemFree(&temp_topic);
		return -1;
	}
	PrintfLog(EN_LOG_LEVEL_DEBUG, "MqttBase: MqttBase_OnMessageArrived() topic: %s, payload %s\n", temp_topic, temp_payload);

	if (onMessageA) {
		onMessageA(context, message->msgid, 0, temp_topic, temp_payload);
	}

#if defined(WIN32) || defined(WIN64)
	pMQTTAsync_freeMessage MQTTAsync_freeMessage = (pMQTTAsync_freeMessage) GetProcAddress(mqttdll, "MQTTAsync_freeMessage");
	pMQTTAsync_free MQTTAsync_free = (pMQTTAsync_free) GetProcAddress(mqttdll, "MQTTAsync_free");
#endif

	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);

	MemFree(&temp_topic);
	MemFree(&temp_payload);

	return 1; //can not return 0 here, otherwise the message won't update or something wrong would happen
}

int MqttBase_SetCallback(int item, MQTT_BASE_CALLBACK_HANDLER handler) {
	switch (item) {
		case EN_MQTT_BASE_CALLBACK_CONNECT_SUCCESS:
			onConnectS = handler;
			break;
		case EN_MQTT_BASE_CALLBACK_CONNECT_FAILURE:
			onConnectF = handler;
			break;
		case EN_MQTT_BASE_CALLBACK_DISCONNECT_SUCCESS:
			onDisconnectS = handler;
			break;
		case EN_MQTT_BASE_CALLBACK_DISCONNECT_FAILURE:
			onDisconnectF = handler;
			break;
		case EN_MQTT_BASE_CALLBACK_CONNECTION_LOST:
			onConnectionL = handler;
			break;
		case EN_MQTT_BASE_CALLBACK_PUBLISH_SUCCESS:
			onPublishS = handler;
			break;
		case EN_MQTT_BASE_CALLBACK_PUBLISH_FAILURE:
			onPublishF = handler;
			break;
		case EN_MQTT_BASE_CALLBACK_SUBSCRIBE_SUCCESS:
			onSubscribeS = handler;
			break;
		case EN_MQTT_BASE_CALLBACK_SUBSCRIBE_FAILURE:
			onSubscribeF = handler;
			break;
		default:
			PrintfLog(EN_LOG_LEVEL_WARNING, "MqttBase: MqttBase_SetCallback() warning, the item (%d) to be set is not available\n", item);
			return IOTA_RESOURCE_NOT_AVAILABLE;
	}
	return IOTA_SUCCESS;
}


int MqttBase_SetCallbackWithTopic(int item, MQTT_BASE_CALLBACK_HANDLER_WITH_TOPIC handler) {
	switch (item) {
		case EN_MQTT_BASE_CALLBACK_MESSAGE_ARRIVED:
			onMessageA = handler;
			return IOTA_SUCCESS;
		default:
			PrintfLog(EN_LOG_LEVEL_WARNING, "MqttBase: MqttBase_SetCallbackWithTopic() warning, the item (%d) to be set is not available\n", item);
			return IOTA_RESOURCE_NOT_AVAILABLE;
	}
}

int MqttBase_init(char *workPath) {
	serverIp = NULL;
	port = NULL;
	username = NULL; //deviceId
	password = NULL;
	client = NULL;
	bs_scope_id = NULL;
	authMode = 0;
	bs_reg_mode = 0;
	privateKeyPassword = NULL;
	if (initFlag) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_init() error, DO NOT init again\n");
		return IOTA_INITIALIZATION_REPEATED;
	}

	if (workPath == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_init() error, workPath or workDir is NULL\n");
		return IOTA_PARAMETER_EMPTY;
	}

	if (access(workPath, 0)) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_init() error, workPath or workDir is NOT accessible\n");
		return IOTA_RESOURCE_NOT_AVAILABLE;
	}

	initFlag = 1;

	pthread_mutex_init(&login_locker, NULL);

	if (CopyStrValue(&workDir, (const char*) workPath, StringLength(workPath)) < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase_init(): there is not enough memory here.\n");
		return IOTA_FAILURE;
	}

	ca_path = CombineStrings(2, workDir, "/conf/rootcert.pem"); //ca_path cannot be released until the programe is destoried

#if defined(WIN32) || defined(WIN64)
	char *libPath = CombineStrings(2, workDir, "/lib/paho-mqtt3as.dll");
	mqttdll = LoadLibraryEx(libPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	DWORD lastError = GetLastError();
	if (mqttdll == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() error, load mqtt dll library failed %d\n", (int) lastError);
		MemFree(&libPath);
		return IOTA_LIBRARY_LOAD_FAILED;
	}
	MemFree(&libPath);
#endif

	return IOTA_SUCCESS;
}

int MqttBase_SetConfig(int item, char *value) {

	int len = StringLength(value);

	if (value == NULL || len == 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_SetConfig() error, the value to be set is NULL or empty\n");
		return IOTA_PARAMETER_EMPTY;
	}

	pthread_mutex_lock(&login_locker);
	switch (item) {
		case EN_MQTT_BASE_CONFIG_SERVER_IP:
			MemFree(&serverIp);
			CopyStrValue(&serverIp, (const char*) value, len); //serverIp should not be free until the program finishes.
			break;
		case EN_MQTT_BASE_CONFIG_SERVER_PORT: {
			MemFree(&port);
			CopyStrValue(&port, (const char*) value, len); //port should not be free until the program finishes.
			if (StrInStr(port, MQTT_PORT)) {
				urlPrefix = TCP_URL_PREFIX;
			} else {
				urlPrefix = SSL_URL_PREFIX;
			}
			break;
		}
		case EN_MQTT_BASE_CONFIG_USERNAME:
			MemFree(&username);
			CopyStrValue(&username, (const char*) value, len); //username should not be free until the program finishes.
			break;
		case EN_MQTT_BASE_CONFIG_PASSWORD:
			MemFree(&password);
			CopyStrValue(&password, (const char*) value, len); //password should not be free until the program finishes.
			break;
		case EN_MQTT_BASE_CONFIG_AUTH_MODE:
			authMode = atoi(value);
			break;
		case EN_MQTT_BASE_CONFIG_LOG_LOCAL_NUMBER: {
			#ifdef _SYS_LOG
			int tValue = String2Int(value);
			if (tValue > 0) {
				SetLogLocalNumber(tValue);
			}
			#endif
			break;
		}
		case EN_MQTT_BASE_CONFIG_LOG_LEVEL: {
			#ifdef _SYS_LOG
			int tValue = String2Int(value);
			if (tValue > 0) {
				SetLogLevel(tValue);
			}
			#endif
			break;
		}
		case EN_MQTT_BASE_CONFIG_KEEP_ALIVE_TIME: {
			int tValue = String2Int((const char*) value);
			if (tValue > 0) {
				keepAliveInterval = String2Int(value);
			}
			break;
		}
		case EN_MQTT_BASE_CONFIG_CONNECT_TIMEOUT: {
			int tValue = String2Int(value);
			if (tValue > 0) {
				connectTimeout = String2Int(value);
			}
			break;
		}
		case EN_MQTT_BASE_CONFIG_RETRY_INTERVAL: {
			int tValue = String2Int(value);
			if (tValue > 0) {
				retryInterval = String2Int(value);
			}
			break;
		}
		case EN_MQTT_BASE_CONFIG_RESET_SECRET_IN_PROGRESS: {
			int tValue = String2Int(value);
			mqttClientCreateFlag = tValue; //value from the app should be 0, thus make sure the encryptedPassword and loginTimestamp will be created again
			if (tValue) {
				PrintfLog(EN_LOG_LEVEL_WARNING, "MqttBase: MqttBase_SetConfig() warning, the value to be set should be zero for RESET_SECRET_IN_PROGRESS\n");
			}
			break;
		}
		case EN_MQTT_BASE_CONFIG_QOS: {
			int tValue = String2Int(value);
			gQOS = tValue;
			PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_SetConfig(), QOS is changed to %d\n", gQOS);
			break;
		}
		case EN_MQTT_BASE_PRIVATE_KEY_PASSWORD: {
			MemFree(&privateKeyPassword);
			CopyStrValue(&privateKeyPassword, (const char*) value, len); //private key password in cert mode device
			break;
		}
		case EN_MQTT_BASE_BS_SCOPE_ID:
			MemFree(&bs_scope_id);
			CopyStrValue(&bs_scope_id, (const char*) value, len);
			break;
		case EN_MQTT_BASE_BS_MODE:
			bs_reg_mode = atoi(value);
			break;
		default:
			PrintfLog(EN_LOG_LEVEL_WARNING, "MqttBase: MqttBase_SetConfig() warning, the item to be set is not available\n");
			pthread_mutex_unlock(&login_locker);
			return IOTA_RESOURCE_NOT_AVAILABLE;
	}

	pthread_mutex_unlock(&login_locker);

	return IOTA_SUCCESS;
}

char* MqttBase_GetConfig(int item) {
	switch (item) {
		case EN_MQTT_BASE_CONFIG_SERVER_IP:
			return serverIp;
		case EN_MQTT_BASE_CONFIG_SERVER_PORT:
			return port;
		case EN_MQTT_BASE_CONFIG_USERNAME:
			return username;
		case EN_MQTT_BASE_CONFIG_PASSWORD:
			return password;

		default:
			PrintfLog(EN_LOG_LEVEL_WARNING, "MqttBase: MqttBase_GetConfig() warning, the item to be get is not available\n");
			return NULL;
	}
}

int MqttBase_subscribe(const char *topic, const int qos) {
#if defined(WIN32) || defined(WIN64)
	pMQTTAsync_subscribe MQTTAsync_subscribe = (pMQTTAsync_subscribe) GetProcAddress(mqttdll, "MQTTAsync_subscribe");
#endif
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

	if (client == NULL) {
		return IOTA_FAILURE;
	}
	int ret;

	opts.onSuccess = MqttBase_OnSubscribeSuccess;
	opts.onFailure = MqttBase_OnSubscribeFailure;

	ret = MQTTAsync_subscribe(client, topic, qos, &opts); //this qos must be 1, otherwise if subscribe failed, the downlink message cannot arrive.

	if (MQTTASYNC_SUCCESS != ret) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_subscribe() error, subscribe failed, ret code %d, topic %s\n", ret, topic);
		return IOTA_FAILURE;
	}

	PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_subscribe(), topic %s, messageId %d\n", topic, opts.token);

	return opts.token;
}

int MqttBase_publish(const char *topic, char *payload) {
#if defined(WIN32) || defined(WIN64)
	pMQTTAsync_sendMessage MQTTAsync_sendMessage = (pMQTTAsync_sendMessage) GetProcAddress(mqttdll, "MQTTAsync_sendMessage");
#endif

	if (client == NULL) {
		return IOTA_FAILURE;
	}

	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

	opts.onSuccess = MqttBase_OnPublishSuccess;
	opts.onFailure = MqttBase_OnPublishFailure;

	pubmsg.payload = payload;
	pubmsg.payloadlen = (int) ConstStringLength(payload);
	pubmsg.qos = gQOS;
	pubmsg.retained = 0;

	int ret = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
	if (ret != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_publish() error, publish result %d\n", ret);
		return IOTA_FAILURE;
	}

	return opts.token;
}

MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;

int MqttBase_CreateConnection() {
#if defined(WIN32) || defined(WIN64)
	pMQTTAsync_create MQTTAsync_create = (pMQTTAsync_create) GetProcAddress(mqttdll, "MQTTAsync_create");
	pMQTTAsync_connect MQTTAsync_connect = (pMQTTAsync_connect) GetProcAddress(mqttdll, "MQTTAsync_connect");
	pMQTTAsync_setCallbacks MQTTAsync_setCallbacks = (pMQTTAsync_setCallbacks) GetProcAddress(mqttdll, "MQTTAsync_setCallbacks");
#endif

	if (workDir == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() error, workPath can not be NULL.\n");
		return IOTA_FAILURE;
	}
	if (authMode) {
		cert_path = CombineStrings(2, workDir, "/conf/deviceCert.pem"); //cert_path cannot be released until the programe is destoried
		key_path = CombineStrings(2, workDir, "/conf/deviceCert.key"); //key_path cannot be released until the programe is destoried
	}

	pthread_mutex_lock(&login_locker);

	char *temp_authMode = "_0_0_";

	if (!mqttClientCreateFlag) {
		if (serverIp == NULL || port == NULL || username == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() error, parameters serverIp/port/username can not be NULL.\n");
			pthread_mutex_unlock(&login_locker);
			return IOTA_PARAMETER_EMPTY;
		}

		conn_opts.cleansession = 1;
		conn_opts.keepAliveInterval = keepAliveInterval;
		conn_opts.connectTimeout = connectTimeout;
		conn_opts.retryInterval = retryInterval;
		conn_opts.onSuccess = MqttBase_OnConnectSuccess;
		conn_opts.onFailure = MqttBase_OnConnectFailure;

		char *loginTimestamp = GetClientTimesStamp();

		if (!authMode) {
			PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_CreateConnection() secret mode.\n");
			int encryptedRet = GetEncryptedPassword(&loginTimestamp, &encrypted_password);
			if (encryptedRet != IOTA_SUCCESS) {
				PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() error, GetEncryptedPassword failed\n");
				MemFree(&loginTimestamp);
				pthread_mutex_unlock(&login_locker);
				return IOTA_SECRET_ENCRYPT_FAILED;
			}
		} else {
			PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_CreateConnection() cert mode.\n");
		}

		if (StrInStr(port, MQTTS_PORT)) {
			if (access(ca_path, 0)) {
				PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() error, ca file is NOT accessible\n");
				MemFree(&loginTimestamp);
				pthread_mutex_unlock(&login_locker);
				return IOTA_CERTIFICATE_NOT_FOUND;
			}
			ssl_opts.trustStore = ca_path;
			ssl_opts.enabledCipherSuites = "TLSv1.2";
			ssl_opts.enableServerCertAuth = TRUE; // TRUE: enable server certificate authentication, FALSE: disable
			// ssl_opts.verify = 0; // 0 for no verifying the hostname, 1 for verifying the hostname

			if (authMode) {
				if (access(cert_path, 0) || access(key_path, 0)) {
					PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() error, cert file or key file is NOT accessible\n");
					MemFree(&loginTimestamp);
					pthread_mutex_unlock(&login_locker);
					return IOTA_CERTIFICATE_NOT_FOUND;
				}

				ssl_opts.keyStore = cert_path;
				ssl_opts.privateKey = key_path;
				ssl_opts.privateKeyPassword = privateKeyPassword;
			}

			conn_opts.ssl = &ssl_opts;
		}

		char *server_address = CombineStrings(4, urlPrefix, serverIp, ":", port);
		char *clientId = NULL;
		if (bs_reg_mode) {
			clientId = CombineStrings(3, username, "_0_", bs_scope_id);
		} else {
			clientId = CombineStrings(3, username, temp_authMode, loginTimestamp);
		}
		MemFree(&loginTimestamp);

		PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_CreateConnection() create in\n");
		int createRet = MQTTAsync_create(&client, server_address, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
		MemFree(&server_address);
		MemFree(&clientId);
		if (createRet) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() MQTTAsync_create error, result %d\n", createRet);
		} else {
			mqttClientCreateFlag = 1;
			PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_CreateConnection() mqttClientCreateFlag = 1.\n");
		}

		MQTTAsync_setCallbacks(client, NULL, MqttBase_OnConnectionLost, MqttBase_OnMessageArrived, NULL);
	}

	char *temp_username = NULL;
	if (CopyStrValue(&temp_username, (const char*) username, StringLength(username)) < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase_CreateConnection(): there is not enough memory here.\n");
		pthread_mutex_unlock(&login_locker);
		return IOTA_FAILURE;
	}
	conn_opts.username = temp_username;

	int ret = 0;
	if (!authMode) {
		char *temp_pwd = NULL;
		if (CopyStrValue(&temp_pwd, (const char*) encrypted_password, PASSWORD_ENCRYPT_LENGTH) < 0) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase_CreateConnection(): there is not enough memory here.\n");
			pthread_mutex_unlock(&login_locker);
			MemFree(&temp_username);
			return IOTA_FAILURE;
		}
		conn_opts.password = temp_pwd;
		PrintfLog(EN_LOG_LEVEL_DEBUG, "MqttBase: MqttBase_CreateConnection() MQTTAsync_connect begin, serverAddrUrl %s%s:%s, user %s mode %s\n", urlPrefix, serverIp, port, username, temp_authMode);
		ret = MQTTAsync_connect(client, &conn_opts);
		MemFree(&temp_pwd);
	} else {
		PrintfLog(EN_LOG_LEVEL_DEBUG, "MqttBase: MqttBase_CreateConnection() MQTTAsync_connect begin, serverAddrUrl %s%s:%s, user %s mode %s\n", urlPrefix, serverIp, port, username, temp_authMode);
		ret = MQTTAsync_connect(client, &conn_opts);
	}

	MemFree(&temp_username);
	if (ret) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() login error, result %d\n", ret);
		pthread_mutex_unlock(&login_locker);
		return IOTA_MQTT_CONNECT_FAILED;
	}

	pthread_mutex_unlock(&login_locker);

	return IOTA_SUCCESS;
}

int MqttBase_ReleaseConnection() {
#if defined(WIN32) || defined(WIN64)
	pMQTTAsync_disconnect MQTTAsync_disconnect = (pMQTTAsync_disconnect) GetProcAddress(mqttdll, "MQTTAsync_disconnect");
#endif

	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	disc_opts.onSuccess = MqttBase_OnDisconnectSuccess;
	disc_opts.onFailure = MqttBase_OnDisconnectFailure;

	PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_ReleaseConnection()\n");

	int ret = MQTTAsync_disconnect(client, &disc_opts);

	if (ret != MQTTASYNC_SUCCESS) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_ReleaseConnection() error, release result %d\n", ret);
		return IOTA_MQTT_DISCONNECT_FAILED;
	}

	PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_ReleaseConnection() success.\n");
	return IOTA_SUCCESS;
}

int MqttBase_destory() {
#if defined(WIN32) || defined(WIN64)
	pMQTTAsync_isConnected MQTTAsync_isConnected = (pMQTTAsync_isConnected) GetProcAddress(mqttdll, "MQTTAsync_isConnected");
	pMQTTAsync_destroy MQTTAsync_destroy = (pMQTTAsync_destroy) GetProcAddress(mqttdll, "MQTTAsync_destroy");
#endif

	PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_destory() in\n");

	if (client && MQTTAsync_isConnected(client)) {
		MqttBase_ReleaseConnection();
	}

	initFlag = 0;

	usleep(1000 * 1000);  // wait connection released

	MemFree(&serverIp);
	MemFree(&port);
	MemFree(&username);
	MemFree(&password);
	MemFree(&workDir);
	MemFree(&encrypted_password);
	MemFree(&ca_path);
	MemFree(&bs_scope_id);

	if (authMode) {
		MemFree(&cert_path);
		MemFree(&key_path);
	}
	MemFree(&privateKeyPassword);

	mqttClientCreateFlag = 0;

	MQTTAsync_destroy(&client);
	return IOTA_SUCCESS;
}

