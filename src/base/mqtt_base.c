/*
 * Copyright (c) <2020>, <Huawei Technologies Co., Ltd>
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
 *   */

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
#include "mqttv5_util.h"

#if defined(WIN32) || defined(WIN64)


typedef int (*pMQTTAsync_connect)(MQTTAsync handle, const MQTTAsync_connectOptions *options);
typedef int (*pMQTTAsync_create)(MQTTAsync *handle, const char *serverURI, const char *clientId, int persistence_type,
    void *persistence_context);
typedef int (*pMQTTAsync_send)(MQTTAsync handle, const char *destinationName, int payloadlen, const void *payload,
    int qos, int retained, MQTTAsync_responseOptions *response);
typedef int (*pMQTTAsync_sendMessage)(MQTTAsync handle, const char *destinationName, const MQTTAsync_message *msg,
    MQTTAsync_responseOptions *response);
typedef int (*pMQTTAsync_setCallbacks)(MQTTAsync handle, void *context, MQTTAsync_connectionLost *cl,
    MQTTAsync_messageArrived *ma, MQTTAsync_deliveryComplete *dc);
typedef int (*pMQTTAsync_subscribe)(MQTTAsync handle, const char *topic, int qos, MQTTAsync_responseOptions *response);
typedef void (*pMQTTAsync_freeMessage)(MQTTAsync_message **msg);
typedef void (*pMQTTAsync_free)(void *ptr);
typedef int (*pMQTTAsync_disconnect)(MQTTAsync handle, const MQTTAsync_disconnectOptions *options);
typedef int (*pMQTTAsync_isConnected)(MQTTAsync handle);
typedef void (*pMQTTAsync_destroy)(MQTTAsync *handle);
typedef void (*pMQTTAsync_destroy)(MQTTAsync *handle);
typedef int (*pMQTTAsync_createWithOptions)(MQTTAsync *handle, const char *serverURI, const char *clientId,
    int persistence_type, void *persistence_context, MQTTAsync_createOptions *options);
typedef int (*pMQTTProperties_add)(MQTTProperties *props, const MQTTProperty *prop);
typedef int (*pMQTTProperties_free)(MQTTProperties *properties);
typedef int (*pMQTTProperty_getType)(enum MQTTPropertyCodes value);
typedef const char *(*pMQTTPropertyName)(enum MQTTPropertyCodes value);
#endif

pthread_mutex_t login_locker = PTHREAD_MUTEX_INITIALIZER;

int gQOS = DEFAULT_QOS;                              // default value of qos is 1
int keepAliveInterval = DEFAULT_KEEP_ALIVE_INTERVAL; // default value of keepAliveInterval is 120s
int connectTimeout = DEFAULT_CONNECT_TIME_OUT;       // default value of connect timeout is 30s
int retryInterval = DEFAULT_RETRYINTERVAL;           // default value of connect retryInterval is 10s

char *serverIp = NULL;
char *port = NULL;
char *username = NULL; // deviceId
char *password = NULL;
int authMode = 0;
char *urlPrefix = TCP_URL_PREFIX;
char *bs_scope_id = NULL;
int bs_reg_mode = 0;

char *workDir = NULL;
char *logDir = NULL;

int verifyCert = 0;
int initFlag = 0;
int checkTimestamp = 0;       // checking timestamp, 0 is not checking㏒? others are checking.The default value is 1;
int mqttClientCreateFlag = 0; // this mqttClientCreateFlag is used to control the invocation of MQTTAsync_create,
                              // otherwise, there would be message leak.

char *ca_path = NULL;
char *cert_path = NULL;
char *key_path = NULL;
char *privateKeyPassword = NULL; // privateKeyPassword in cert mode device

MQTTAsync client = NULL;

#if defined(WIN32) || defined(WIN64)
HMODULE mqttdll = NULL;
#endif

#if defined(MQTTV5)
char **send_mass = NULL; // MQTT v5 Subject alias heap
char topic_alias_len = 0;
#endif

char *encrypted_password = NULL;

int GetEncryptedPassword(char **timestamp, char **encryptedPwd)
{
    if (password == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "MqttBase: GetEncryptedPassword() error, password is NULL, please set the required parameters properly\n");
        return IOTA_PARAMETER_EMPTY;
    }

    char *temp_pwd = NULL;
    if (CopyStrValue(&temp_pwd, (const char *)password, StringLength(password)) < 0) {
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

    if (CopyStrValue(encryptedPwd, (const char *)temp_encrypted_pwd, PASSWORD_ENCRYPT_LENGTH) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "GetEncryptedPassword(): there is not enough memory here.\n");
        MemFree(&temp_pwd);
        MemFree(&temp_encrypted_pwd);
        return IOTA_FAILURE;
    }

    MemFree(&temp_pwd);
    MemFree(&temp_encrypted_pwd);
    return IOTA_SUCCESS;
}

#if defined(MQTTV5)
void HandleCallbackFailure5(const char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void *context,
    MQTTAsync_failureData5 *response)
{
    EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp = (EN_IOTA_MQTT_PROTOCOL_RSP *)malloc(sizeof(EN_IOTA_MQTT_PROTOCOL_RSP));

    if (protocolRsp == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
        return;
    }

    protocolRsp->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (protocolRsp->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
        MemFree(&protocolRsp);
        return;
    }

    if (response) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: %s error, messageId %d, code %d, message %s\n", currentFunctionName,
            response->token, response->code, response->message);

        protocolRsp->mqtt_msg_info->context = context;
        protocolRsp->mqtt_msg_info->messageId = response->token;
        protocolRsp->mqtt_msg_info->code = response->code;

        protocolRsp->message = (char *)response->message;
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

void HandleCallbackSuccess5(const char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void *context,
    MQTTAsync_successData5 *response)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: %s messageId %d\n", currentFunctionName, response ? response->token : -1);

    EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp = (EN_IOTA_MQTT_PROTOCOL_RSP *)malloc(sizeof(EN_IOTA_MQTT_PROTOCOL_RSP));

    if (protocolRsp == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
        return;
    }

    protocolRsp->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (protocolRsp->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
        MemFree(&protocolRsp);
        return;
    }
    protocolRsp->mqtt_msg_info->context = context;
    protocolRsp->mqtt_msg_info->messageId = response ? response->token : 0;
    protocolRsp->mqtt_msg_info->code = IOTA_SUCCESS;

    protocolRsp->message = NULL;
    if (response->properties.count > 0) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "response properties:\n");
        logProperties(&response->properties);
    }

    if (callback) {
        (callback)(protocolRsp);
    }
    MemFree(&protocolRsp->mqtt_msg_info);
    MemFree(&protocolRsp);
}

MQTTV5_DATA DataConversionArrived(MQTTProperties *props)
{
    MQTTV5_DATA mqttv5_por = mqttv5_initializer;
    if (props->count == 0) {
        return mqttv5_por;
    }
    int i = 0;
    int user_identification = 0;
    MQTTV5_USER_PRO *user_head;
    MQTTV5_USER_PRO *p = NULL;

    for (i = 0; i < props->count; ++i) {
        int id = props->array[i].identifier;

        if (id == MQTTPROPERTY_CODE_USER_PROPERTY) {
            MQTTV5_USER_PRO *user = (MQTTV5_USER_PRO *)malloc(sizeof(MQTTV5_USER_PRO));
            user->key = props->array[i].value.data.data;
            user->Value = props->array[i].value.value.data;
            user->nex = NULL;

            if (user_identification == 0) {
                user_identification = 1;
                user_head = user;
            } else {
                p->nex = user;
            }
            p = user;
        } else if (id == MQTTPROPERTY_CODE_CONTENT_TYPE) {
            mqttv5_por.contnt_type = props->array[i].value.data.data;
        } else if (id == MQTTPROPERTY_CODE_RESPONSE_TOPIC) {
            mqttv5_por.response_topic = props->array[i].value.data.data;
        } else if (id == MQTTPROPERTY_CODE_CORRELATION_DATA) {
            mqttv5_por.correlation_data = props->array[i].value.data.data;
        }
    }
    mqttv5_por.properties = user_head;
    return mqttv5_por;
}
void logProperties(MQTTProperties *props)
{
    int i = 0;
#if defined(WIN32) || defined(WIN64)
    pMQTTProperty_getType MQTTProperty_getType = (pMQTTProperty_getType)GetProcAddress(mqttdll, "MQTTProperty_getType");
#endif
    for (i = 0; i < props->count; ++i) {
        int id = props->array[i].identifier;
        const char *name = MQTTPropertyName(id);
        char *intformat = "Property name %s value %d\n";

        switch (MQTTProperty_getType(id)) {
            case MQTTPROPERTY_TYPE_BYTE:
                PrintfLog(EN_LOG_LEVEL_DEBUG, intformat, name, props->array[i].value.byte);
                break;
            case MQTTPROPERTY_TYPE_TWO_BYTE_INTEGER:
                PrintfLog(EN_LOG_LEVEL_DEBUG, intformat, name, props->array[i].value.integer2);
                break;
            case MQTTPROPERTY_TYPE_FOUR_BYTE_INTEGER:
                PrintfLog(EN_LOG_LEVEL_DEBUG, intformat, name, props->array[i].value.integer4);
                break;
            case MQTTPROPERTY_TYPE_VARIABLE_BYTE_INTEGER:
                PrintfLog(EN_LOG_LEVEL_DEBUG, intformat, name, props->array[i].value.integer4);
                break;
            case MQTTPROPERTY_TYPE_BINARY_DATA:
            case MQTTPROPERTY_TYPE_UTF_8_ENCODED_STRING:
                PrintfLog(EN_LOG_LEVEL_DEBUG, "Property name %s value len %s\n", name, props->array[i].value.data.data);
                break;
            case MQTTPROPERTY_TYPE_UTF_8_STRING_PAIR:
                PrintfLog(EN_LOG_LEVEL_DEBUG, "Property name %s key %s value %s\n", name,
                    props->array[i].value.data.data, props->array[i].value.value.data);
                break;
        }
    }
}
MQTTProperties DataConversion(MQTTV5_DATA *properties)
{
    MQTTProperties pro = MQTTProperties_initializer;
    MQTTProperty property;

    if (properties == NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "properties == NULL\n");
        return pro;
    }
    MQTTV5_USER_PRO *user = properties->properties;

    while (user != NULL) {
        property.identifier = MQTTPROPERTY_CODE_USER_PROPERTY;
        property.value.data.data = user->key;
        property.value.data.len = (int)strlen(property.value.data.data);
        property.value.value.data = user->Value;
        property.value.value.len = (int)strlen(property.value.value.data);
        MQTTProperties_add(&pro, &property);
        user = (MQTTV5_USER_PRO *)user->nex;
    }

    if (properties->contnt_type != NULL) {
        property.identifier = MQTTPROPERTY_CODE_CONTENT_TYPE;
        property.value.data.data = properties->contnt_type;
        property.value.data.len = (int)strlen(property.value.data.data);
        MQTTProperties_add(&pro, &property);
    }

    if (properties->response_topic != NULL) {
        property.identifier = MQTTPROPERTY_CODE_RESPONSE_TOPIC;
        property.value.data.data = properties->response_topic;
        property.value.data.len = (int)strlen(property.value.data.data);
        MQTTProperties_add(&pro, &property);
    }

    if (properties->correlation_data != NULL) {
        property.identifier = MQTTPROPERTY_CODE_CORRELATION_DATA;
        property.value.data.data = properties->correlation_data;
        property.value.data.len = (int)strlen(property.value.data.data);
        MQTTProperties_add(&pro, &property);
    }
    return pro;
}
int MQTTV5_Topic_Heap(char *sum)
{
    if (send_mass == NULL || sum == NULL) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "MQTTV5_Topic_Heap() EER\n");
        return -1;
    }

    int index = 0;
    int i = 0;
    for (i = 0; i < topic_alias_len; i++) {
        if (strcmp(sum, send_mass[i]) == 0) {
            index = i;
            break;
        }
    }
    if (i != topic_alias_len) {
        return index + 1;
    } else {
        if (topic_alias_len < TOPIC_ALIAS_MAX) {
            send_mass[topic_alias_len] = (char *)malloc(sizeof(char) * strlen(sum) + 1);
            strcpy_s(send_mass[topic_alias_len], strlen(sum) + 1, sum);
            topic_alias_len++;
        } else {
            return -1;
        }
    }
    return 0;
}
#else

void HandleCallbackFailure(const char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void *context,
    MQTTAsync_failureData *response)
{
    EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp = (EN_IOTA_MQTT_PROTOCOL_RSP *)malloc(sizeof(EN_IOTA_MQTT_PROTOCOL_RSP));

    if (protocolRsp == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
        return;
    }

    protocolRsp->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
    if (protocolRsp->mqtt_msg_info == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
        MemFree(&protocolRsp);
        return;
    }

    if (response) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: %s error, messageId %d, code %d, message %s\n", currentFunctionName,
            response->token, response->code, response->message);

        protocolRsp->mqtt_msg_info->context = context;
        protocolRsp->mqtt_msg_info->messageId = response->token;
        protocolRsp->mqtt_msg_info->code = response->code;

        protocolRsp->message = (char *)response->message;
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

void HandleCallbackSuccess(const char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void *context,
    MQTTAsync_successData *response)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: %s messageId %d\n", currentFunctionName, response ? response->token : -1);

    EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp = (EN_IOTA_MQTT_PROTOCOL_RSP *)malloc(sizeof(EN_IOTA_MQTT_PROTOCOL_RSP));

    if (protocolRsp == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
        return;
    }

    protocolRsp->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
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
#endif

// in order not to be duplicated with the callback function name set by the application
MQTT_BASE_CALLBACK_HANDLER onConnectS; 
MQTT_BASE_CALLBACK_HANDLER onConnectF;
MQTT_BASE_CALLBACK_HANDLER onDisconnectS;
MQTT_BASE_CALLBACK_HANDLER onDisconnectF;
MQTT_BASE_CALLBACK_HANDLER onConnectionL;
MQTT_BASE_CALLBACK_HANDLER onSubscribeS;
MQTT_BASE_CALLBACK_HANDLER onSubscribeF;
MQTT_BASE_CALLBACK_HANDLER onPublishS;
MQTT_BASE_CALLBACK_HANDLER onPublishF;
MQTT_BASE_CALLBACK_HANDLER_WITH_TOPIC onMessageA;

#if defined(MQTTV5)
void MqttBase_OnConnectSuccess5(void *context, MQTTAsync_successData5 *response)
{
    if (topic_alias_len == 0 && send_mass == NULL) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "send_mass Init\n");
        send_mass = (char **)malloc(TOPIC_ALIAS_MAX * sizeof(char *));
    }
    HandleCallbackSuccess5("MqttBase_OnConnectSuccess5()", onConnectS, context, response);
}

void MqttBase_OnConnectFailure5(void *context, MQTTAsync_failureData5 *response)
{
    HandleCallbackFailure5("MqttBase_OnConnectFailure5()", onConnectF, context, response);
}

void MqttBase_OnSubscribeSuccess5(void *context, MQTTAsync_successData5 *response)
{
    HandleCallbackSuccess5("MqttBase_OnSubscribeSuccess5()", onSubscribeS, context, response);
}

void MqttBase_OnSubscribeFailure5(void *context, MQTTAsync_failureData5 *response)
{
    HandleCallbackFailure5("MqttBase_OnSubscribeFailure5()", onSubscribeF, context, response);
}

void MqttBase_OnPublishSuccess5(void *context, MQTTAsync_successData5 *response)
{
    HandleCallbackSuccess5("MqttBase_OnPublishSuccess5()", onPublishS, context, response);
}

void MqttBase_OnPublishFailure5(void *context, MQTTAsync_failureData5 *response)
{
    HandleCallbackFailure5("MqttBase_OnPublishFailure5()", onPublishF, context, response);
}

void MqttBase_OnDisconnectSuccess5(void *context, MQTTAsync_successData5 *response)
{
    HandleCallbackSuccess5("MqttBase_OnDisconnectSuccess5()", onDisconnectS, context, response);
}

void MqttBase_OnDisconnectFailure5(void *context, MQTTAsync_successData5 *response)
{
    HandleCallbackFailure5("MqttBase_OnDisconnectFailure5()", onDisconnectF, context, response);
}
#else
void MqttBase_OnConnectSuccess(void *context, MQTTAsync_successData *response)
{
    HandleCallbackSuccess("MqttBase_OnConnectSuccess()", onConnectS, context, response);
}

void MqttBase_OnConnectFailure(void *context, MQTTAsync_failureData *response)
{
    HandleCallbackFailure("MqttBase_OnConnectFailure()", onConnectF, context, response);
}

void MqttBase_OnSubscribeSuccess(void *context, MQTTAsync_successData *response)
{
    HandleCallbackSuccess("MqttBase_OnSubscribeSuccess()", onSubscribeS, context, response);
}

void MqttBase_OnSubscribeFailure(void *context, MQTTAsync_failureData *response)
{
    HandleCallbackFailure("MqttBase_OnSubscribeFailure()", onSubscribeF, context, response);
}


void MqttBase_OnPublishSuccess(void *context, MQTTAsync_successData *response)
{
    HandleCallbackSuccess("MqttBase_OnPublishSuccess()", onPublishS, context, response);
}

void MqttBase_OnPublishFailure(void *context, MQTTAsync_failureData *response)
{
    HandleCallbackFailure("MqttBase_OnPublishFailure()", onPublishF, context, response);
}

void MqttBase_OnDisconnectSuccess(void *context, MQTTAsync_successData *response)
{
    HandleCallbackSuccess("MqttBase_OnDisconnectSuccess()", onDisconnectS, context, response);
}

void MqttBase_OnDisconnectFailure(void *context, MQTTAsync_failureData *response)
{
    HandleCallbackFailure("MqttBase_OnDisconnectFailure()", onDisconnectF, context, response);
}
#endif


void MqttBase_OnConnectionLost(void *context, char *cause)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_OnConnectionLost() error, cause: %s\n", cause);

    EN_IOTA_MQTT_PROTOCOL_RSP *protocolRsp = (EN_IOTA_MQTT_PROTOCOL_RSP *)malloc(sizeof(EN_IOTA_MQTT_PROTOCOL_RSP));

    if (protocolRsp == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: there is not enough memory here.\n");
        return;
    }

    protocolRsp->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO *)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
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

void MQTTBase_Disconnected(void *context, MQTTProperties *props, enum MQTTReasonCodes rc)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "Callback: disconnected, reason code \"%s\"", MQTTReasonCode_toString(rc));
}


int MqttBase_OnMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_OnMessageArrived() -------------> \n");
    char *temp_topic = NULL;
    char *temp_payload = NULL;
#if defined(MQTTV5)
    MQTTV5_DATA mqttv5_por = DataConversionArrived(&message->properties);
    if (message->properties.count > 0) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Suback properties:\n");
        logProperties(&message->properties);
    }
#endif

    if (CopyStrValue(&temp_topic, (const char *)topicName, topicLen) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "MqttBase: MqttBase_OnMessageArrived() error, there is not enougth memory here.\n");
        MQTTAsync_freeMessage(&message);
        MQTTAsync_free(topicName);
        return -1;
    }
    if (CopyStrValue(&temp_payload, (const char *)message->payload, message->payloadlen) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "MqttBase: MqttBase_OnMessageArrived() error, there is not enougth memory here.\n");
        MQTTAsync_freeMessage(&message);
        MQTTAsync_free(topicName);
        MemFree(&temp_topic);
        return -1;
    }
    PrintfLog(EN_LOG_LEVEL_DEBUG, "MqttBase: MqttBase_OnMessageArrived() topic: %s, payload %s\n", temp_topic,
        temp_payload);

    if (onMessageA) {
#if defined(MQTTV5)
        onMessageA(context, message->msgid, 0, temp_topic, temp_payload, &mqttv5_por);
#else
        onMessageA(context, message->msgid, 0, temp_topic, temp_payload, NULL);
#endif
    }

#if defined(WIN32) || defined(WIN64)
    pMQTTAsync_freeMessage MQTTAsync_freeMessage =
        (pMQTTAsync_freeMessage)GetProcAddress(mqttdll, "MQTTAsync_freeMessage");
    pMQTTAsync_free MQTTAsync_free = (pMQTTAsync_free)GetProcAddress(mqttdll, "MQTTAsync_free");
#endif

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    MemFree(&temp_topic);
    MemFree(&temp_payload);

    return 1; // can not return 0 here, otherwise the message won't update or something wrong would happen
}

void MqttBase_OnDisconnected(void *context, MQTTProperties *props, enum MQTTReasonCodes rc)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "Callback: disconnected, reason code \"%s\"", MQTTReasonCode_toString(rc));
#if defined(MQTTV5)
    int i = 0;
    for (i = 0; i < topic_alias_len; i++) {
        free(send_mass[i]);
    }
    free(send_mass);
#endif
}

int MqttBase_SetCallback(int item, MQTT_BASE_CALLBACK_HANDLER handler)
{
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
            PrintfLog(EN_LOG_LEVEL_WARNING,
                "MqttBase: MqttBase_SetCallback() warning, the item (%d) to be set is not available\n", item);
            return IOTA_RESOURCE_NOT_AVAILABLE;
    }
    return IOTA_SUCCESS;
}


int MqttBase_SetCallbackWithTopic(int item, MQTT_BASE_CALLBACK_HANDLER_WITH_TOPIC handler)
{
    switch (item) {
        case EN_MQTT_BASE_CALLBACK_MESSAGE_ARRIVED:
            onMessageA = handler;
            return IOTA_SUCCESS;
        default:
            PrintfLog(EN_LOG_LEVEL_WARNING,
                "MqttBase: MqttBase_SetCallbackWithTopic() warning, the item (%d) to be set is not available\n", item);
            return IOTA_RESOURCE_NOT_AVAILABLE;
    }
}

int MqttBase_init(char *workPath)
{
    serverIp = NULL;
    port = NULL;
    username = NULL; // deviceId
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

    if (CopyStrValue(&workDir, (const char *)workPath, StringLength(workPath)) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase_init(): there is not enough memory here.\n");
        return IOTA_FAILURE;
    }
    // ca_path cannot be released until the programe is destoried,must replace with "/conf/bsrootcert.pem" for bootstraping
    ca_path = CombineStrings(2, workDir, "/conf/rootcert.pem");

#if defined(WIN32) || defined(WIN64)
    char *libPath = CombineStrings(2, workDir, "/lib/paho-mqtt3as.dll");
    mqttdll = LoadLibraryEx(libPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    DWORD lastError = GetLastError();
    if (mqttdll == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() error, load mqtt dll library failed %d\n",
            (int)lastError);
        MemFree(&libPath);
        return IOTA_LIBRARY_LOAD_FAILED;
    }
    MemFree(&libPath);
#endif

    return IOTA_SUCCESS;
}
void MqttBase_SetPort(char *value, int valueLen)
{
    MemFree(&port);
    // port should not be free until the program finishes.
    CopyStrValue(&port, (const char *)value, valueLen); 
    if (StrInStr(port, MQTT_PORT)) {
        urlPrefix = TCP_URL_PREFIX;
    } else {
        urlPrefix = SSL_URL_PREFIX;
    }
    return;
}

int MqttBase_SetConfig(int item, char *value)
{
    int len = StringLength(value);

    if (value == NULL || len == 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_SetConfig() error, the value to be set is NULL or empty\n");
        return IOTA_PARAMETER_EMPTY;
    }

    pthread_mutex_lock(&login_locker);
    switch (item) {
        case EN_MQTT_BASE_CONFIG_SERVER_IP:
            MemFree(&serverIp);
            // serverIp should not be free until the program finishes.
            CopyStrValue(&serverIp, (const char *)value, len); 
            break;

        case EN_MQTT_BASE_CONFIG_SERVER_PORT:
            MqttBase_SetPort(value, len);
            break;

        case EN_MQTT_BASE_CONFIG_USERNAME:
            MemFree(&username);
            // username should not be free until the program finishes.
            CopyStrValue(&username, (const char *)value, len); 
            break;
        case EN_MQTT_BASE_CONFIG_PASSWORD:
            MemFree(&password);
            // password should not be free until the program finishes.
            CopyStrValue(&password, (const char *)value, len); 
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
            int tValue = String2Int((const char *)value);
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
            // value from the app should be 0, thus make sure the encryptedPassword and
            // loginTimestamp will be created again
            mqttClientCreateFlag = tValue; 
            if (tValue) {
                PrintfLog(EN_LOG_LEVEL_WARNING, "MqttBase: MqttBase_SetConfig() warning, the value to be set should be "
                    "zero for RESET_SECRET_IN_PROGRESS\n");
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
            CopyStrValue(&privateKeyPassword, (const char *)value, len); // private key password in cert mode device
            break;
        }
        case EN_MQTT_BASE_BS_SCOPE_ID:
            MemFree(&bs_scope_id);
            CopyStrValue(&bs_scope_id, (const char *)value, len);
            break;
        case EN_MQTT_BASE_BS_MODE:
            bs_reg_mode = atoi(value);
            break;
        default:
            PrintfLog(EN_LOG_LEVEL_WARNING,
                "MqttBase: MqttBase_SetConfig() warning, the item to be set is not available\n");
            pthread_mutex_unlock(&login_locker);
            return IOTA_RESOURCE_NOT_AVAILABLE;
    }

    pthread_mutex_unlock(&login_locker);

    return IOTA_SUCCESS;
}

char *MqttBase_GetConfig(int item)
{
    switch (item) {
        case EN_MQTT_BASE_CONFIG_SERVER_IP:
            return serverIp;
        case EN_MQTT_BASE_CONFIG_SERVER_PORT:
            return port;
        case EN_MQTT_BASE_CONFIG_USERNAME:
            return username;
        case EN_MQTT_BASE_CONFIG_PASSWORD:
            PrintfLog(EN_LOG_LEVEL_WARNING, "MqttBase: MqttBase_GetConfig() Can't get password from here\n");
            return NULL;
        default:
            PrintfLog(EN_LOG_LEVEL_WARNING,
                "MqttBase: MqttBase_GetConfig() warning, the item to be get is not available\n");
            return NULL;
    }
}

int MqttBase_subscribe(const char *topic, const int qos)
{
#if defined(WIN32) || defined(WIN64)
    pMQTTAsync_subscribe MQTTAsync_subscribe = (pMQTTAsync_subscribe)GetProcAddress(mqttdll, "MQTTAsync_subscribe");
#endif
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

    if (client == NULL) {
        return IOTA_FAILURE;
    }
    int ret;

#if defined(MQTTV5)
    opts.onSuccess5 = MqttBase_OnSubscribeSuccess5;
    opts.onFailure5 = MqttBase_OnSubscribeFailure5;
#else
    opts.onSuccess = MqttBase_OnSubscribeSuccess;
    opts.onFailure = MqttBase_OnSubscribeFailure;
#endif


    ret = MQTTAsync_subscribe(client, topic, qos,
        &opts); // this qos must be 1, otherwise if subscribe failed, the downlink message cannot arrive.

    if (MQTTASYNC_SUCCESS != ret) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_subscribe() error, subscribe failed, ret code %d, topic %s\n",
            ret, topic);
        return IOTA_FAILURE;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_subscribe(), topic %s, messageId %d\n", topic, opts.token);

    return opts.token;
}

int MqttBase_publish(const char *topic, void *payload, int len, void *context, void *properties)
{
#if defined(WIN32) || defined(WIN64)
    pMQTTAsync_sendMessage MQTTAsync_sendMessage =
        (pMQTTAsync_sendMessage)GetProcAddress(mqttdll, "MQTTAsync_sendMessage");
#if defined(MQTTV5)
    pMQTTProperties_add MQTTProperties_add = (pMQTTProperties_add)GetProcAddress(mqttdll, "MQTTProperties_add");
    pMQTTProperties_free MQTTProperties_free = (pMQTTProperties_free)GetProcAddress(mqttdll, "MQTTProperties_free");
#endif
#endif

    if (client == NULL) {
        return IOTA_FAILURE;
    }

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    opts.context = context;

    pubmsg.payload = payload;
    pubmsg.payloadlen = len;
    pubmsg.qos = gQOS;
    pubmsg.retained = 0;

#if defined(MQTTV5)
    opts.onSuccess5 = MqttBase_OnPublishSuccess5;
    opts.onFailure5 = MqttBase_OnPublishFailure5;
    pubmsg.properties = DataConversion((MQTTV5_DATA *)properties);

    printf("-------------------------->>\n");
    logProperties(&pubmsg.properties);
    // PrintfLog(EN_LOG_LEVEL_DEBUG, "topic = %s\n", topic);
    int ret = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);

#else
    opts.onSuccess = MqttBase_OnPublishSuccess;
    opts.onFailure = MqttBase_OnPublishFailure;
    int ret = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
#endif

    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_publish() error, publish result %d\n", ret);
        return IOTA_FAILURE;
    }
    MQTTProperties_free(&pubmsg.properties);
    return opts.token;
}

#if defined(MQTTV5)
MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer5;
#else
MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
#endif
MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;

char *MqttBase_gitClientId(char *temp_authMode, char *loginTimestamp)
{
    char *clientId = NULL;
    if (bs_reg_mode) {
        clientId = CombineStrings(3, username, "_0_", bs_scope_id);
    } else {
        clientId = CombineStrings(3, username, temp_authMode, loginTimestamp);
    }
    return clientId;
}
int MqttBase_setSslOpts()
{
    if (verifyCert == 0) {
        ssl_opts.trustStore = NULL;
        ssl_opts.enableServerCertAuth = FALSE; // TRUE: enable server certificate authentication, FALSE: disable
        ssl_opts.sslVersion = 3;
    } else {
        if (access(ca_path, 0)) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_setSslOpts() error, ca file is NOT accessible\n");
            return IOTA_CERTIFICATE_NOT_FOUND;
        }
        ssl_opts.trustStore = ca_path;
        ssl_opts.enableServerCertAuth = TRUE; // TRUE: enable server certificate authentication, FALSE: disable
        ssl_opts.sslVersion = 3;
        // ssl_opts.verify = 0; // 0 for no verifying the hostname, 1 for verifying the hostname
    }

    if (authMode) {
        if (access(cert_path, 0) || access(key_path, 0)) {
            PrintfLog(EN_LOG_LEVEL_ERROR,
                "MqttBase: MqttBase_setSslOpts() error, cert file or key file is NOT accessible\n");
            return IOTA_CERTIFICATE_NOT_FOUND;
        }

        ssl_opts.keyStore = cert_path;
        ssl_opts.privateKey = key_path;
        ssl_opts.privateKeyPassword = privateKeyPassword;
    }
    return IOTA_SUCCESS;
}
int MqttBase_setConnOptsByClientCreate(char *loginTimestamp)
{
    conn_opts.cleansession = 1;
    conn_opts.keepAliveInterval = keepAliveInterval;
    conn_opts.connectTimeout = connectTimeout;
    conn_opts.retryInterval = retryInterval;
#if defined(MQTTV5)
    conn_opts.cleansession = 0;
    conn_opts.cleanstart = 1;
    conn_opts.onSuccess5 = MqttBase_OnConnectSuccess5;
    conn_opts.onFailure5 = MqttBase_OnConnectFailure5;
    conn_opts.MQTTVersion = MQTTVERSION_5;
#else
    conn_opts.onSuccess = MqttBase_OnConnectSuccess;
    conn_opts.onFailure = MqttBase_OnConnectFailure;
#endif

    if (!authMode) {
        PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_setConnOptsByClientCreate() secret mode.\n");
        int encryptedRet = GetEncryptedPassword(&loginTimestamp, &encrypted_password);
        if (encryptedRet != IOTA_SUCCESS) {
            PrintfLog(EN_LOG_LEVEL_ERROR,
                "MqttBase: MqttBase_setConnOptsByClientCreate() error, GetEncryptedPassword failed\n");
            return IOTA_SECRET_ENCRYPT_FAILED;
        }
    } else {
        PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_setConnOptsByClientCreate() cert mode.\n");
    }

    if (StrInStr(port, MQTTS_PORT)) {
        int ret = MqttBase_setSslOpts();
        if (ret < 0) {
            return ret;
        }
        conn_opts.ssl = &ssl_opts;
    }
    return IOTA_SUCCESS;
}
int MqttBase_CreateConnection()
{
#if defined(WIN32) || defined(WIN64)
    pMQTTAsync_create MQTTAsync_create = (pMQTTAsync_create)GetProcAddress(mqttdll, "MQTTAsync_create");
    pMQTTAsync_setCallbacks MQTTAsync_setCallbacks =
        (pMQTTAsync_setCallbacks)GetProcAddress(mqttdll, "MQTTAsync_setCallbacks");
#if defined(MQTTV5)
    pMQTTAsync_createWithOptions MQTTAsync_createWithOptions =
        (pMQTTAsync_createWithOptions)GetProcAddress(mqttdll, "MQTTAsync_createWithOptions");
    pMQTTProperties_add MQTTProperties_add = (pMQTTProperties_add)GetProcAddress(mqttdll, "MQTTProperties_add");
#endif
    pMQTTAsync_connect MQTTAsync_connect = (pMQTTAsync_connect)GetProcAddress(mqttdll, "MQTTAsync_connect");
#endif

#if defined(MQTTV5)
    MQTTProperty proprrty;
    MQTTAsync_createOptions creatOpts = MQTTAsync_createOptions_initializer;
#endif
    if (workDir == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() error, workPath can not be NULL.\n");
        return IOTA_FAILURE;
    }
    if (authMode) {
        // cert_path cannot be released until the programe is destoried
        cert_path = CombineStrings(2, workDir, "/conf/deviceCert.pem"); 
        // key_path cannot be released until the programe is destoried
        key_path = CombineStrings(2, workDir, "/conf/deviceCert.key"); 
    }

    pthread_mutex_lock(&login_locker);

    char *temp_authMode = NULL;
    if (checkTimestamp == 0) {
        temp_authMode = "_0_0_";
    } else {
        temp_authMode = "_0_1_";
    }

    if (!mqttClientCreateFlag) {
        if (serverIp == NULL || port == NULL || username == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR,
                "MqttBase: MqttBase_CreateConnection() error, parameters serverIp/port/username can not be NULL.\n");
            pthread_mutex_unlock(&login_locker);
            return IOTA_PARAMETER_EMPTY;
        }
        char *loginTimestamp = GetClientTimesStamp();
        int ret = MqttBase_setConnOptsByClientCreate(loginTimestamp);
        if (ret < 0) {
            MemFree(&loginTimestamp);
            pthread_mutex_unlock(&login_locker);
            return ret;
        }
        char *server_address = CombineStrings(4, urlPrefix, serverIp, ":", port);
        char *clientId = MqttBase_gitClientId(temp_authMode, loginTimestamp);

        MemFree(&loginTimestamp);

        PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_CreateConnection() create in\n");
#if defined(MQTTV5)
        creatOpts.MQTTVersion = MQTTVERSION_5;
        creatOpts.struct_version = 1;
        PrintfLog(EN_LOG_LEVEL_INFO, "server_address = %s\n clientId = %s", server_address, clientId);
        int createRet = MQTTAsync_createWithOptions(&client, server_address, clientId, MQTTCLIENT_PERSISTENCE_NONE,
            NULL, &creatOpts);
#else
        int createRet = MQTTAsync_create(&client, server_address, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);
#endif
        MemFree(&server_address);
        MemFree(&clientId);
        if (createRet) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() MQTTAsync_create error, result %d\n",
                createRet);
        } else {
            mqttClientCreateFlag = 1;
            PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_CreateConnection() mqttClientCreateFlag = 1.\n");
        }

        MQTTAsync_setCallbacks(client, NULL, MqttBase_OnConnectionLost, MqttBase_OnMessageArrived, NULL);
    }

    char *temp_username = NULL;
    if (CopyStrValue(&temp_username, (const char *)username, StringLength(username)) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase_CreateConnection(): there is not enough memory here.\n");
        pthread_mutex_unlock(&login_locker);
        return IOTA_FAILURE;
    }
    conn_opts.username = temp_username;

    int ret = 0;
    if (!authMode) {
        char *temp_pwd = NULL;
        if (CopyStrValue(&temp_pwd, (const char *)encrypted_password, PASSWORD_ENCRYPT_LENGTH) < 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase_CreateConnection(): there is not enough memory here.\n");
            pthread_mutex_unlock(&login_locker);
            MemFree(&temp_username);
            return IOTA_FAILURE;
        }
        conn_opts.password = temp_pwd;
        PrintfLog(EN_LOG_LEVEL_DEBUG,
            "MqttBase: MqttBase_CreateConnection() MQTTAsync_connect begin, serverAddrUrl %s%s:%s, user %s mode %s\n",
            urlPrefix, serverIp, port, username, temp_authMode);
        ret = MQTTAsync_connect(client, &conn_opts);
        MemFree(&temp_pwd);
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG,
            "MqttBase: MqttBase_CreateConnection() MQTTAsync_connect begin, serverAddrUrl %s%s:%s, user %s mode %s\n",
            urlPrefix, serverIp, port, username, temp_authMode);
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

int MqttBase_ReleaseConnection()
{
#if defined(WIN32) || defined(WIN64)
    pMQTTAsync_disconnect MQTTAsync_disconnect = (pMQTTAsync_disconnect)GetProcAddress(mqttdll, "MQTTAsync_disconnect");
#endif

#if defined(MQTTV5)
    MQTTProperties props = MQTTProperties_initializer;
    MQTTProperty proprrty;
#endif
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
#if defined(MQTTV5)
    disc_opts.onSuccess = MqttBase_OnDisconnectSuccess5;
    disc_opts.onFailure = MqttBase_OnDisconnectFailure5;
#else
    disc_opts.onSuccess = MqttBase_OnDisconnectSuccess;
    disc_opts.onFailure = MqttBase_OnDisconnectFailure;
#endif
    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_ReleaseConnection()\n");

#if defined(MQTTV5)
    proprrty.identifier = MQTTPROPERTY_CODE_SESSION_EXPIRY_INTERVAL;
    proprrty.value.integer2 = 0;
    MQTTProperties_add(&props, &proprrty);
    disc_opts.properties = props;
#endif

    int ret = MQTTAsync_disconnect(client, &disc_opts);

    if (ret != MQTTASYNC_SUCCESS) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_ReleaseConnection() error, release result %d\n", ret);
        return IOTA_MQTT_DISCONNECT_FAILED;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_ReleaseConnection() success.\n");
    return IOTA_SUCCESS;
}

int MqttBase_destory()
{
#if defined(WIN32) || defined(WIN64)
    pMQTTAsync_isConnected MQTTAsync_isConnected =
        (pMQTTAsync_isConnected)GetProcAddress(mqttdll, "MQTTAsync_isConnected");
    pMQTTAsync_destroy MQTTAsync_destroy = (pMQTTAsync_destroy)GetProcAddress(mqttdll, "MQTTAsync_destroy");
#endif

    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_destory() in\n");

    if (client && MQTTAsync_isConnected(client)) {
        MqttBase_ReleaseConnection();
    }

    initFlag = 0;

    usleep(1000 * 1000); // wait connection released

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
