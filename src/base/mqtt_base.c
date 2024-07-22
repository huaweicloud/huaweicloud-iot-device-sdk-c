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

#if defined(WIN32) || defined(WIN64)
#include "windows.h"
#include "io.h"
#else
#include "unistd.h"
#endif

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "securec.h"
#include "string_util.h"
#include "log_util.h"
#include "hmac_sha256.h"
#include "iota_error_type.h"
#include "mqttv5_util.h"
#include "mqtt_base.h"

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

static pthread_mutex_t g_loginLocker = PTHREAD_MUTEX_INITIALIZER;

static int g_QoS = DEFAULT_QOS;                              // default value of qos is 1
static int g_keepAliveInterval = DEFAULT_KEEP_ALIVE_INTERVAL; // default value of g_keepAliveInterval is 120s
static int g_connectTimeout = DEFAULT_CONNECT_TIME_OUT;       // default value of connect timeout is 30s
static int g_retryInterval = DEFAULT_RETRYINTERVAL;           // default value of connect g_retryInterval is 10s
static int g_maxInflight = DEFAULT_MAXINFIGHT;                // This controls how many messages can be in-flight simultaneously.

static char *g_serverIp = NULL;
static char *g_port = NULL;
static char *g_username = NULL; // deviceId
static char *g_password = NULL;
static int g_authMode = 0;
static char *g_urlPrefix = TCP_URL_PREFIX;

// --- 设备发放参数 ----
static char *g_bsScopeId = NULL;
static int g_bsRegMode = 0;
static char *g_bsGroupSecret = NULL;

static char *g_workDir = NULL;
static int g_verifyCert = DEFAULT_SERVERCERTAUTH;
static int g_initFlag = 0;
static int g_checkTimestamp = 0;         // checking timestamp, 0 is not checking, others are checking.The default value is 0;
static int g_mqttClientCreateFlag = 0;   // this mqttClientCreateFlag is used to control the invocation of MQTTAsync_create,
// otherwise, there would be message leak.

// ---- 网关参数 ------
static int g_bridgeMode = 0;

static char *g_caPath = NULL;
static char *g_certPath = NULL;
static char *g_keyPath = NULL;
static char *g_privateKeyPassword = NULL; // privateKeyPassword in cert mode device

static MQTTAsync g_client = NULL;

#if defined(WIN32) || defined(WIN64)
HMODULE mqttdll = NULL;
#endif

static  char *g_encryptedPassword = NULL;

int GetEncryptedPassword(char **timestamp, char **encryptedPwd)
{
    if (g_password == NULL && g_bsGroupSecret == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "MqttBase: GetEncryptedPassword() error, password is NULL, please set the required parameters properly\n");
        return IOTA_PARAMETER_EMPTY;
    }

    char *tempPwd = NULL;
    if (CopyStrValue(&tempPwd, (const char *)g_password, StringLength(g_password)) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "GetEncryptedPassword(): there is not enough memory here.\n");
        return IOTA_FAILURE;
    }
    char *tempEncryptedPwd = NULL;
    StringMalloc(&tempEncryptedPwd, PASSWORD_ENCRYPT_LENGTH + 1);
    if (tempEncryptedPwd == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: GetEncryptedPassword() error, there is not enough memory here.\n");
        (void)memset_s(tempPwd, StringLength(tempPwd), 0, StringLength(tempPwd));
        MemFree(&tempPwd);
        return IOTA_FAILURE;
    }

    int returnValue = IOTA_SUCCESS;
    do {
        if (g_bsRegMode) {
            char *bsTempEncryptedPwd = NULL;
            StringMalloc(&bsTempEncryptedPwd, PASSWORD_ENCRYPT_LENGTH + 1);
            int ret = EncryptWithHMac(g_bsGroupSecret, &g_username, ENCRYPT_LENGTH, bsTempEncryptedPwd, g_checkTimestamp);
            if (ret != IOTA_SUCCESS) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: GetEncryptedPassword() bsRegMode error, encrypt failed %d\n", ret);
                returnValue = IOTA_SECRET_ENCRYPT_FAILED;
                break;
            }
            (void)memset_s(tempPwd, StringLength(tempPwd), 0, StringLength(tempPwd));
            MemFree(&tempPwd);
            tempPwd = bsTempEncryptedPwd;
        }
        
        int ret = EncryptWithHMac(tempPwd, timestamp, ENCRYPT_LENGTH, tempEncryptedPwd, g_checkTimestamp);
        if (ret != IOTA_SUCCESS) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: GetEncryptedPassword() error, encrypt failed %d\n", ret);
            returnValue = IOTA_SECRET_ENCRYPT_FAILED;
            break;
        }

        if (CopyStrValue(encryptedPwd, (const char *)tempEncryptedPwd, PASSWORD_ENCRYPT_LENGTH) < 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "GetEncryptedPassword(): there is not enough memory here.\n");
            returnValue = IOTA_FAILURE;
            break;
        }
    } while(0);

    // clear sensitive password info
    (void)memset_s(tempPwd, StringLength(tempPwd), 0, StringLength(tempPwd));
    (void)memset_s(tempEncryptedPwd, PASSWORD_ENCRYPT_LENGTH, 0, PASSWORD_ENCRYPT_LENGTH);
    MemFree(&tempPwd);
    MemFree(&tempEncryptedPwd);
    return IOTA_SUCCESS;
}

static void LogProperties(MQTTProperties *props)
{
    int i = 0;
#if defined(WIN32) || defined(WIN64)
    pMQTTProperty_getType MQTTProperty_getType = (pMQTTProperty_getType)GetProcAddress(mqttdll, "MQTTProperty_getType");
#endif
    for (i = 0; i < props->count; ++i) {
        enum MQTTPropertyCodes id = props->array[i].identifier;
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
                PrintfLog(EN_LOG_LEVEL_DEBUG, "Property name=%s,value=%s\n", name, props->array[i].value.data.data);
                break;
            case MQTTPROPERTY_TYPE_UTF_8_STRING_PAIR:
                PrintfLog(EN_LOG_LEVEL_DEBUG, "Property name=%s,key=%s, value=%s\n", name,
                    props->array[i].value.data.data, props->array[i].value.value.data);
                break;
            default:
                PrintfLog(EN_LOG_LEVEL_WARNING, "unknown property\n");
                break;
        }
    }
}

#if defined(MQTTV5)
static void HandleCallbackFailure5(const char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void *context,
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

static void HandleCallbackSuccess5(const char *currentFunctionName, MQTT_BASE_CALLBACK_HANDLER callback, void *context,
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
        LogProperties(&response->properties);
    }

    if (callback) {
        (callback)(protocolRsp);
    }
    MemFree(&protocolRsp->mqtt_msg_info);
    MemFree(&protocolRsp);
}

static MQTTV5_DATA DataConversionArrived(MQTTProperties *props)
{
    MQTTV5_DATA mqttv5Porperties = mqttv5_initializer;
    if (props->count == 0) {
        return mqttv5Porperties;
    }
    int i = 0;
    int userIdentification = 0;
    MQTTV5_USER_PRO *userHead;
    MQTTV5_USER_PRO *p = NULL;

    for (i = 0; i < props->count; ++i) {
        int id = props->array[i].identifier;

        if (id == MQTTPROPERTY_CODE_USER_PROPERTY) {
            MQTTV5_USER_PRO *user = (MQTTV5_USER_PRO *)malloc(sizeof(MQTTV5_USER_PRO));
            user->key = props->array[i].value.data.data;
            user->Value = props->array[i].value.value.data;
            user->nex = NULL;

            if (userIdentification == 0) {
                userIdentification = 1;
                userHead = user;
            } else {
                p->nex = user;
            }
            p = user;
        } else if (id == MQTTPROPERTY_CODE_CONTENT_TYPE) {
            mqttv5Porperties.contnt_type = props->array[i].value.data.data;
        } else if (id == MQTTPROPERTY_CODE_RESPONSE_TOPIC) {
            mqttv5Porperties.response_topic = props->array[i].value.data.data;
        } else if (id == MQTTPROPERTY_CODE_CORRELATION_DATA) {
            mqttv5Porperties.correlation_data = props->array[i].value.data.data;
        } else if (id == MQTTPROPERTY_CODE_TOPIC_ALIAS) {
            mqttv5Porperties.topic_alias = props->array[i].value.data.data;
        }
    }
    mqttv5Porperties.properties = userHead;
    return mqttv5Porperties;
}

static MQTTProperties DataConversion(MQTTV5_DATA *properties)
{
    MQTTProperties pro = MQTTProperties_initializer;
    MQTTProperty property;

    if (properties == NULL) {
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

    if (properties->topic_alias > 0) {
        property.identifier = MQTTPROPERTY_CODE_TOPIC_ALIAS;
        property.value.integer4 = (short)properties->topic_alias;
        MQTTProperties_add(&pro, &property);
    }
    return pro;
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
static void MqttBase_OnConnectSuccess5(void *context, MQTTAsync_successData5 *response)
{
    HandleCallbackSuccess5("MqttBase_OnConnectSuccess5()", onConnectS, context, response);
}

static void MqttBase_OnConnectFailure5(void *context, MQTTAsync_failureData5 *response)
{
    HandleCallbackFailure5("MqttBase_OnConnectFailure5()", onConnectF, context, response);
}

static void MqttBase_OnSubscribeSuccess5(void *context, MQTTAsync_successData5 *response)
{
    HandleCallbackSuccess5("MqttBase_OnSubscribeSuccess5()", onSubscribeS, context, response);
}

static void MqttBase_OnSubscribeFailure5(void *context, MQTTAsync_failureData5 *response)
{
    HandleCallbackFailure5("MqttBase_OnSubscribeFailure5()", onSubscribeF, context, response);
}

static void MqttBase_OnPublishSuccess5(void *context, MQTTAsync_successData5 *response)
{
    HandleCallbackSuccess5("MqttBase_OnPublishSuccess5()", onPublishS, context, response);
}

static void MqttBase_OnPublishFailure5(void *context, MQTTAsync_failureData5 *response)
{
    HandleCallbackFailure5("MqttBase_OnPublishFailure5()", onPublishF, context, response);
}

static void MqttBase_OnDisconnectSuccess5(void *context, MQTTAsync_successData5 *response)
{
    HandleCallbackSuccess5("MqttBase_OnDisconnectSuccess5()", onDisconnectS, context, response);
}

static void MqttBase_OnDisconnectFailure5(void *context, MQTTAsync_failureData5 *response)
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


int MqttBase_OnMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_OnMessageArrived() -------------> \n");
    char *temp_topic = NULL;
    char *temp_payload = NULL;
#if defined(MQTTV5)
    MQTTV5_DATA mqttv5_por = DataConversionArrived(&message->properties);
    if (message->properties.count > 0) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Suback properties:\n");
        LogProperties(&message->properties);
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
    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_OnMessageArrived() topic: %s, payload %s\n", temp_topic,
        temp_payload);

    if (onMessageA) {
#if defined(MQTTV5)
        onMessageA(context, message->msgid, 0, temp_topic, temp_payload, message->payloadlen, &mqttv5_por);
#else
        onMessageA(context, message->msgid, 0, temp_topic, temp_payload, message->payloadlen, NULL);
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

void MqttBase_SetCallback(EN_MQTT_BASE_CALLBACK_SETTING item, MQTT_BASE_CALLBACK_HANDLER handler)
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
    }
}

void MqttBase_SetMessageArrivedCallback(MQTT_BASE_CALLBACK_HANDLER_WITH_TOPIC handler)
{
    onMessageA = handler;
}

int MqttBase_init(char *workPath)
{
    g_serverIp = NULL;
    g_port = NULL;
    g_username = NULL; // deviceId
    g_password = NULL;
    g_client = NULL;
    g_bsScopeId = NULL;
    g_authMode = 0;
    g_bsRegMode = 0;
    g_privateKeyPassword = NULL;
    if (g_initFlag) {
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

    g_initFlag = 1;

    pthread_mutex_init(&g_loginLocker, NULL);

    if (CopyStrValue(&g_workDir, (const char *)workPath, StringLength(workPath)) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase_init(): there is not enough memory here.\n");
        return IOTA_FAILURE;
    }
    // ca_path cannot be released until the programe is destoried,must replace with "/conf/bsrootcert.pem" for
    // bootstraping
    g_caPath = CombineStrings(2, g_workDir, "/conf/rootcert.pem");

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

static void MqttBase_SetPort(char *value, int valueLen)
{
    MemFree(&g_port);
    // port should not be free until the program finishes.
    CopyStrValue(&g_port, (const char *)value, valueLen);
    if (StrInStr(g_port, MQTT_PORT)) {
        g_urlPrefix = TCP_URL_PREFIX;
    } else {
        g_urlPrefix = SSL_URL_PREFIX;
    }
    return;
}

int MqttBase_SetConfig(ENUM_MQTT_BASE_CONFIG item, char *value)
{
    int len = StringLength(value);
    if (value == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_SetConfig() error, the value to be set is NULL or empty %d\n", item);
        return IOTA_PARAMETER_EMPTY;
    }

    pthread_mutex_lock(&g_loginLocker);
    switch (item) {
        case EN_MQTT_BASE_CONFIG_SERVER_IP:
            MemFree(&g_serverIp);
            // g_serverIp should not be free until the program finishes.
            CopyStrValue(&g_serverIp, (const char *)value, len);
            break;

        case EN_MQTT_BASE_CONFIG_SERVER_PORT:
            MqttBase_SetPort(value, len);
            break;

        case EN_MQTT_BASE_CONFIG_USERNAME:
            MemFree(&g_username);
            // username should not be free until the program finishes.
            CopyStrValue(&g_username, (const char *)value, len);
            break;
        case EN_MQTT_BASE_CONFIG_PASSWORD:
            MemFree(&g_password);
            // password should not be free until the program finishes.
            CopyStrValue(&g_password, (const char *)value, len);
            break;
        case EN_MQTT_BASE_CONFIG_AUTH_MODE:
            g_authMode = atoi(value);
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
                g_keepAliveInterval = String2Int(value);
            }
            break;
        }
        case EN_MQTT_BASE_CONFIG_CONNECT_TIMEOUT: {
            int tValue = String2Int(value);
            if (tValue > 0) {
                g_connectTimeout = String2Int(value);
            }
            break;
        }
        case EN_MQTT_BASE_CONFIG_RETRY_INTERVAL: {
            int tValue = String2Int(value);
            if (tValue > 0) {
                g_retryInterval = String2Int(value);
            }
            break;
        }
        case EN_MQTT_BASE_CONFIG_RESET_SECRET_IN_PROGRESS: {
            int tValue = String2Int(value);
            // value from the app should be 0, thus make sure the encryptedPassword and
            // loginTimestamp will be created again
            g_mqttClientCreateFlag = tValue;
            if (tValue) {
                PrintfLog(EN_LOG_LEVEL_WARNING, "MqttBase: MqttBase_SetConfig() warning, the value to be set should be "
                    "zero for RESET_SECRET_IN_PROGRESS\n");
            }
            break;
        }
        case EN_MQTT_BASE_CONFIG_QOS: {
            int tValue = String2Int(value);
            g_QoS = tValue;
            PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_SetConfig(), QOS is changed to %d\n", g_QoS);
            break;
        }
        case EN_MQTT_BASE_PRIVATE_KEY_PASSWORD: {
            MemFree(&g_privateKeyPassword);
            CopyStrValue(&g_privateKeyPassword, (const char *)value, len); // private key password in cert mode device
            break;
        }
        case EN_MQTT_BASE_BS_SCOPE_ID:
            MemFree(&g_bsScopeId);
            CopyStrValue(&g_bsScopeId, (const char *)value, len);
            break;
        case EN_MQTT_BASE_BS_MODE:
            g_bsRegMode = atoi(value);
            break;
        case EN_MQTT_BASE_CHECK_STAMP:
            g_checkTimestamp = atoi(value);
            break;
        case EN_MQTT_BASE_BS_GROUP_SECRET:
            MemFree(&g_bsGroupSecret);
            char *valueDecode = base64_decode(value);
            CopyStrValue(&g_bsGroupSecret, (const char *)valueDecode, len);
            MemFree(&valueDecode);
            break;
        case EN_MQTT_BASE_BRIDGE_MODE:
            g_bridgeMode = atoi(value);
            break;
        default:
            PrintfLog(EN_LOG_LEVEL_WARNING,
                "MqttBase: MqttBase_SetConfig() warning, the item to be set is not available\n");
            pthread_mutex_unlock(&g_loginLocker);
            return IOTA_RESOURCE_NOT_AVAILABLE;
    }

    pthread_mutex_unlock(&g_loginLocker);

    return IOTA_SUCCESS;
}

char *MqttBase_GetConfig(ENUM_MQTT_BASE_CONFIG item)
{
    switch (item) {
        case EN_MQTT_BASE_CONFIG_SERVER_IP:
            return g_serverIp;
        case EN_MQTT_BASE_CONFIG_SERVER_PORT:
            return g_port;
        case EN_MQTT_BASE_CONFIG_USERNAME:
            return g_username;
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

    if (g_client == NULL) {
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

    // this qos must be 1, otherwise if subscribe failed, the downlink message cannot arrive.
    ret = MQTTAsync_subscribe(g_client, topic, qos, &opts);
    if (ret != MQTTASYNC_SUCCESS) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_subscribe() error, subscribe failed, ret code %d, topic %s\n",
            ret, topic);
        return IOTA_FAILURE;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_subscribe(), topic %s, qos %d, messageId %d\n", topic, qos, opts.token);

    return opts.token;
}

static int MqttBase_publishSet(const char *topic, void *payload, int len, int qos, void *context, void *properties)
{
#if defined(WIN32) || defined(WIN64)
    pMQTTAsync_sendMessage MQTTAsync_sendMessage =
        (pMQTTAsync_sendMessage)GetProcAddress(mqttdll, "MQTTAsync_sendMessage");
#if defined(MQTTV5)
    pMQTTProperties_add MQTTProperties_add = (pMQTTProperties_add)GetProcAddress(mqttdll, "MQTTProperties_add");
    pMQTTProperties_free MQTTProperties_free = (pMQTTProperties_free)GetProcAddress(mqttdll, "MQTTProperties_free");
#endif
#endif

    if (g_client == NULL) {
        return IOTA_FAILURE;
    }

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    opts.context = context;

    pubmsg.payload = payload;
    pubmsg.payloadlen = len;
    pubmsg.qos = qos;
    pubmsg.retained = 0;

#if defined(MQTTV5)
    opts.onSuccess5 = MqttBase_OnPublishSuccess5;
    opts.onFailure5 = MqttBase_OnPublishFailure5;
    pubmsg.properties = DataConversion((MQTTV5_DATA *)properties);

    LogProperties(&pubmsg.properties);
    int ret = MQTTAsync_sendMessage(g_client, topic, &pubmsg, &opts);

#else
    opts.onSuccess = MqttBase_OnPublishSuccess;
    opts.onFailure = MqttBase_OnPublishFailure;
    int ret = MQTTAsync_sendMessage(g_client, topic, &pubmsg, &opts);
#endif
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_publish() error, publish result %d\n", ret);
        return IOTA_FAILURE;
    }
    MQTTProperties_free(&pubmsg.properties);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "PUBLISH: topic=%s, messageId %d, payload is %s==>\n", topic,  opts.token, payload);
    return opts.token;
}

int MqttBase_publish(const char *topic, void *payload, int len, void *context, void *properties)
{
    return MqttBase_publishSet(topic, payload, len, g_QoS, context, properties);
}

int MqttBase_publishSetQos(const char *topic, void *payload, int len, int qos, void *context, void *properties)
{
    return MqttBase_publishSet(topic, payload, len, qos, context, properties);
}

#if defined(MQTTV5)
static MQTTAsync_connectOptions g_connOpts = MQTTAsync_connectOptions_initializer5;
#else
static MQTTAsync_connectOptions g_connOpts = MQTTAsync_connectOptions_initializer;
#endif
static MQTTAsync_SSLOptions g_sslOpts = MQTTAsync_SSLOptions_initializer;

static char *MqttBase_getClientId(char *authMode, int authModeLength, char *loginTimestamp)
{
    /*
     * DO NOT remove the seemingly useless parameter authModeLength because
     * rule "数组作为函数参数时，必须同时将其长度作为函数的参数"
     */
    char *clientId = NULL;
    PrintfLog(EN_LOG_LEVEL_INFO, "username: %s, auth mode: %.*s, timestamp: %s\n",
        g_username, authModeLength, authMode, loginTimestamp);
    if (g_bsRegMode) {
        if (g_authMode) {
            clientId = CombineStrings(3, g_username, "_0_", g_bsScopeId);
        } else {
            char temp[4] = {'_', g_checkTimestamp + '0', '_', 0};
            clientId = CombineStrings(5, g_username, "_0_", g_bsScopeId, temp, loginTimestamp);
        }
    } else {
        clientId = CombineStrings(3, g_username, authMode, loginTimestamp);
    }
    return clientId;
}

static int MqttBase_setSslOpts(void)
{
    if (g_verifyCert == 0) {
        g_sslOpts.trustStore = NULL;
        g_sslOpts.enableServerCertAuth = FALSE; // TRUE: enable server certificate authentication, FALSE: disable
        g_sslOpts.sslVersion = MQTT_SSL_VERSION_TLS_1_2;
    } else {
        if (access(g_caPath, 0)) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_setSslOpts() error, ca file is NOT accessible\n");
            return IOTA_CERTIFICATE_NOT_FOUND;
        }
        g_sslOpts.trustStore = g_caPath;
        g_sslOpts.enableServerCertAuth = TRUE; // TRUE: enable server certificate authentication, FALSE: disable
        g_sslOpts.sslVersion = MQTT_SSL_VERSION_TLS_1_2;
        // ssl_opts.verify = 0; // 0 for no verifying the hostname, 1 for verifying the hostname
    }

    if (g_authMode) {
        if (access(g_certPath, 0) || access(g_keyPath, 0)) {
            PrintfLog(EN_LOG_LEVEL_ERROR,
                "MqttBase: MqttBase_setSslOpts() error, cert file or key file is NOT accessible\n");
            return IOTA_CERTIFICATE_NOT_FOUND;
        }

        g_sslOpts.keyStore = g_certPath;
        g_sslOpts.privateKey = g_keyPath;
        g_sslOpts.privateKeyPassword = g_privateKeyPassword;
    }
    return IOTA_SUCCESS;
}

static int MqttBase_setConnOptsByClientCreate(char *loginTimestamp)
{
    g_connOpts.cleansession = 1;
    g_connOpts.keepAliveInterval = g_keepAliveInterval;
    g_connOpts.connectTimeout = g_connectTimeout;
    g_connOpts.retryInterval = g_retryInterval;
    if (g_maxInflight > 0 && g_maxInflight < 65536) {    
        g_connOpts.maxInflight = g_maxInflight;
    }
#if defined(MQTTV5)
    g_connOpts.cleansession = 1;
    g_connOpts.cleanstart = 1;
    g_connOpts.onSuccess5 = MqttBase_OnConnectSuccess5;
    g_connOpts.onFailure5 = MqttBase_OnConnectFailure5;
    g_connOpts.MQTTVersion = MQTTVERSION_5;
#else
    g_connOpts.onSuccess = MqttBase_OnConnectSuccess;
    g_connOpts.onFailure = MqttBase_OnConnectFailure;
#endif

    if (!g_authMode) {
        PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_setConnOptsByClientCreate() secret mode.\n");
        int encryptedRet = GetEncryptedPassword(&loginTimestamp, &g_encryptedPassword);
        if (encryptedRet != IOTA_SUCCESS) {
            PrintfLog(EN_LOG_LEVEL_ERROR,
                "MqttBase: MqttBase_setConnOptsByClientCreate() error, GetEncryptedPassword failed\n");
            return IOTA_SECRET_ENCRYPT_FAILED;
        }
    } else {
        PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_setConnOptsByClientCreate() cert mode.\n");
    }

    if (StrInStr(g_port, MQTTS_PORT)) {
        int ret = MqttBase_setSslOpts();
        if (ret < 0) {
            return ret;
        }
        g_connOpts.ssl = &g_sslOpts;
    }
    return IOTA_SUCCESS;
}

#if MQTT_TRACE_ON

void MQTTBase_TraceCallback(enum MQTTASYNC_TRACE_LEVELS level, char *message)
{
    PrintfLog((level + EN_LOG_LEVEL_MQTT_MAXIMUM - 1), "%s\n", message);
}

void MqttBase_DeBugInit()
{
    MQTTAsync_setTraceCallback(MQTTBase_TraceCallback);
    MQTTAsync_setTraceLevel(MQTT_TRACE_LEVEL);
}
#endif

int MqttBase_CreateConnection(void)
{
#if defined(WIN32) || defined(WIN64)
    pMQTTAsync_create MQTTAsync_create = (pMQTTAsync_create)GetProcAddress(mqttdll, "MQTTAsync_create");
    pMQTTAsync_setCallbacks MQTTAsync_setCallbacks =
        (pMQTTAsync_setCallbacks)GetProcAddress(mqttdll, "MQTTAsync_setCallbacks");
    pMQTTAsync_createWithOptions MQTTAsync_createWithOptions =
        (pMQTTAsync_createWithOptions)GetProcAddress(mqttdll, "MQTTAsync_createWithOptions");
    pMQTTProperties_add MQTTProperties_add = (pMQTTProperties_add)GetProcAddress(mqttdll, "MQTTProperties_add");
    pMQTTAsync_connect MQTTAsync_connect = (pMQTTAsync_connect)GetProcAddress(mqttdll, "MQTTAsync_connect");
#endif

    MQTTProperty proprrty;
    MQTTAsync_createOptions creatOpts = MQTTAsync_createOptions_initializer;
    if (g_workDir == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() error, workPath can not be NULL.\n");
        return IOTA_FAILURE;
    }
    if (g_authMode) {
        // cert_path cannot be released until the programe is destoried
        g_certPath = CombineStrings(2, g_workDir, "/conf/deviceCert.pem");
        // key_path cannot be released until the programe is destoried
        g_keyPath = CombineStrings(2, g_workDir, "/conf/deviceCert.key");
    }
#if MQTT_TRACE_ON
    MqttBase_DeBugInit();
#endif
    pthread_mutex_lock(&g_loginLocker);

    char tempAuthMode[CHECK_STAMP_LENGTH] = "_0_0_";
    tempAuthMode[CHECK_STAMP_INDEX] = '0' + g_checkTimestamp;
    if (g_bridgeMode) {
        tempAuthMode[CHECK_STAMP_MODE] = '3';
    }
    if (!g_mqttClientCreateFlag) {
        if ((g_serverIp == NULL) || (g_port == NULL) || (g_username == NULL)) {
            PrintfLog(EN_LOG_LEVEL_ERROR,
                "MqttBase: MqttBase_CreateConnection() error, parameters serverIp/port/username can not be NULL.\n");
            pthread_mutex_unlock(&g_loginLocker);
            return IOTA_PARAMETER_EMPTY;
        }
        char *loginTimestamp = GetClientTimesStamp();
        int ret = MqttBase_setConnOptsByClientCreate(loginTimestamp);
        if (ret < 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR,
                "MqttBase: MqttBase_CreateConnection() error, parameters serverIp/port/username can not be NULL.\n");
            MemFree(&loginTimestamp);
            pthread_mutex_unlock(&g_loginLocker);
            return ret;
        }
        char *serverAddress = CombineStrings(4, g_urlPrefix, g_serverIp, ":", g_port);
        char *clientId = MqttBase_getClientId(tempAuthMode, CHECK_STAMP_LENGTH, loginTimestamp);
        MemFree(&loginTimestamp);

        PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_CreateConnection() create in\n");
#if defined(MQTTV5)
        creatOpts.MQTTVersion = MQTTVERSION_5;
#else
        creatOpts.MQTTVersion = MQTTVERSION_3_1_1;
#endif
        if (MAX_BUFFERED_MESSAGES > 0) {
            creatOpts.maxBufferedMessages = MAX_BUFFERED_MESSAGES;
        }
        creatOpts.sendWhileDisconnected = 1;
        creatOpts.struct_version = 1;
        
        PrintfLog(EN_LOG_LEVEL_INFO, "server_address = %s\n clientId = %s\n", serverAddress, clientId);
        int createRet = MQTTAsync_createWithOptions(&g_client, serverAddress, clientId, MQTTCLIENT_PERSISTENCE_NONE,
            NULL, &creatOpts);

        // int createRet = MQTTAsync_create(&g_client, serverAddress, clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL);

        MemFree(&serverAddress);
        MemFree(&clientId);
        if (createRet) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() MQTTAsync_create error, result %d\n",
                createRet);
        } else {
            g_mqttClientCreateFlag = 1;
            PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_CreateConnection() mqttClientCreateFlag = 1.\n");
        }

        MQTTAsync_setCallbacks(g_client, NULL, MqttBase_OnConnectionLost, MqttBase_OnMessageArrived, NULL);
    }

    char *tempUsername = NULL;
    if (CopyStrValue(&tempUsername, (const char *)g_username, StringLength(g_username)) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase_CreateConnection(): there is not enough memory here.\n");
        pthread_mutex_unlock(&g_loginLocker);
        return IOTA_FAILURE;
    }
    g_connOpts.username = tempUsername;

    int ret = 0;
    if (!g_authMode) {
        char *tempPwd = NULL;
        if (CopyStrValue(&tempPwd, (const char *)g_encryptedPassword, PASSWORD_ENCRYPT_LENGTH) < 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase_CreateConnection(): there is not enough memory here.\n");
            pthread_mutex_unlock(&g_loginLocker);
            MemFree(&tempUsername);
            return IOTA_FAILURE;
        }
        g_connOpts.password = tempPwd;
        PrintfLog(EN_LOG_LEVEL_DEBUG,
            "MqttBase: MqttBase_CreateConnection() MQTTAsync_connect begin, serverAddrUrl %s%s:%s, user %s mode %s\n",
            g_urlPrefix, g_serverIp, g_port, g_username, tempAuthMode);
        ret = MQTTAsync_connect(g_client, &g_connOpts);
        (void)memset_s(tempPwd, PASSWORD_ENCRYPT_LENGTH, 0, PASSWORD_ENCRYPT_LENGTH);
        MemFree(&tempPwd);
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG,
            "MqttBase: MqttBase_CreateConnection() MQTTAsync_connect begin, serverAddrUrl %s%s:%s, user %s mode %s\n",
            g_urlPrefix, g_serverIp, g_port, g_username, tempAuthMode);
        ret = MQTTAsync_connect(g_client, &g_connOpts);
    }

    MemFree(&tempUsername);
    if (ret) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_CreateConnection() login error, result %d\n", ret);
        pthread_mutex_unlock(&g_loginLocker);
        return IOTA_MQTT_CONNECT_FAILED;
    }

    pthread_mutex_unlock(&g_loginLocker);

    return IOTA_SUCCESS;
}

int MqttBase_ReleaseConnection(void)
{
#if defined(WIN32) || defined(WIN64)
    pMQTTAsync_disconnect MQTTAsync_disconnect = (pMQTTAsync_disconnect)GetProcAddress(mqttdll, "MQTTAsync_disconnect");
#endif

#if defined(MQTTV5)
    MQTTProperties props = MQTTProperties_initializer;
    MQTTProperty proprrty;
#endif
    MQTTAsync_disconnectOptions discOpts = MQTTAsync_disconnectOptions_initializer;
#if defined(MQTTV5)
    discOpts.onSuccess5 = MqttBase_OnDisconnectSuccess5;
    discOpts.onFailure5 = MqttBase_OnDisconnectFailure5;
#else
    discOpts.onSuccess = MqttBase_OnDisconnectSuccess;
    discOpts.onFailure = MqttBase_OnDisconnectFailure;
#endif

#if defined(MQTTV5)
    proprrty.identifier = MQTTPROPERTY_CODE_SESSION_EXPIRY_INTERVAL;
    proprrty.value.integer2 = 0;
    MQTTProperties_add(&props, &proprrty);
    discOpts.properties = props;
#endif

    int ret = MQTTAsync_disconnect(g_client, &discOpts);
    if (ret != MQTTASYNC_SUCCESS) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "MqttBase: MqttBase_ReleaseConnection() error, release result %d\n", ret);
        return IOTA_MQTT_DISCONNECT_FAILED;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_ReleaseConnection() success.\n");
    return IOTA_SUCCESS;
}

int MqttBase_destory(void)
{
#if defined(WIN32) || defined(WIN64)
    pMQTTAsync_isConnected MQTTAsync_isConnected =
        (pMQTTAsync_isConnected)GetProcAddress(mqttdll, "MQTTAsync_isConnected");
    pMQTTAsync_destroy MQTTAsync_destroy = (pMQTTAsync_destroy)GetProcAddress(mqttdll, "MQTTAsync_destroy");
#endif

    PrintfLog(EN_LOG_LEVEL_INFO, "MqttBase: MqttBase_destory() in\n");

    if (g_client && MQTTAsync_isConnected(g_client)) {
        MqttBase_ReleaseConnection();
    }

    g_initFlag = 0;
    g_bsRegMode = 0;
    g_bridgeMode = 0;
    usleep(1000 * 1000); // wait connection released

    MemFree(&g_serverIp);
    MemFree(&g_port);
    MemFree(&g_username);
    MemFree(&g_password);
    MemFree(&g_workDir);
    MemFree(&g_encryptedPassword);
    MemFree(&g_caPath);
    MemFree(&g_bsScopeId);
    MemFree(&g_bsGroupSecret);

    if (g_authMode) {
        MemFree(&g_certPath);
        MemFree(&g_keyPath);
    }
    MemFree(&g_privateKeyPassword);

    g_mqttClientCreateFlag = 0;

    MQTTAsync_destroy(&g_client);
    return IOTA_SUCCESS;
}

int MqttBase_IsConnected(void)
{
    return MQTTAsync_isConnected(g_client);
}