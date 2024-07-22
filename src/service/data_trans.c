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

#include <string.h>
#include "base.h"
#include "log_util.h"
#include "mqtt_base.h"
#include "string_util.h"
#include "cJSON.h"
#include "iota_error_type.h"
#include "data_trans.h"

static int ReportCompressedData(char *topic, char *payload, void *context, void *properties)
{
    int len = strlen(payload);

    unsigned char *compressed = NULL;
    StringMalloc(&compressed, len * 2);
    if (compressed == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "compressed error, there is not enough memory here.\n");
        return IOTA_FAILURE;
    }

    int gz_size = gZIPCompress(payload, len, compressed, len * 2);
    int ret = MqttBase_publish((const char *)topic, compressed, gz_size, context, properties);
    MemFree(&topic);
    MemFree(&compressed);

    if (ret < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: ReportCompressedData() error, publish failed, result %d\n", ret);
        return IOTA_FAILURE;
    }

    return IOTA_SUCCESS;
}

int ReportDeviceData(char *payload, char *topicParas, int compressFlag, void *context, void *properties)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = NULL;

    if (topicParas == NULL) {
        if (compressFlag == 0) {
            topic = CombineStrings(3, TOPIC_PREFIX, username, TOPIC_SUFFIX_MESSAGEUP);
        } else if (compressFlag == 1) {
            topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_MESSAGEUP, TOPIC_SUFFIX_COMPRESS);

            return ReportCompressedData(topic, payload, context, properties);
        } else {
            PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: compressFlag is invalid.\n");
            return IOTA_FAILURE;
        }
    } else {
        topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_USER, topicParas);
    }

    return ReportData(topic, payload, context, properties);
}

int ReportDeviceProperties(char *payload, int compressFlag, void *context, void *properties)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = NULL;

    if (compressFlag == 0) {
        topic = CombineStrings(3, TOPIC_PREFIX, username, TOPIC_SUFFIX_PROP_REP);
    } else if (compressFlag == 1) {
        topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_PROP_REP, TOPIC_SUFFIX_COMPRESS);

        return ReportCompressedData(topic, payload, context, properties);
    } else {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: compressFlag is invalid.\n");
        return IOTA_FAILURE;
    }
    return ReportData(topic, payload, context, properties);
}

int ReportBatchDeviceProperties(char *payload, int compressFlag, void *context, void *properties)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = NULL;

    if (compressFlag == 0) {
        topic = CombineStrings(3, TOPIC_PREFIX, username, TOPIC_SUFFIX_PROPS_REP);
    } else if (compressFlag == 1) {
        topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_PROPS_REP, TOPIC_SUFFIX_COMPRESS);

        return ReportCompressedData(topic, payload, context, properties);
    } else {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: compressFlag is invalid.\n");
        return IOTA_FAILURE;
    }

    return ReportData(topic, payload, context, NULL);
}

int ReportCommandReponse(char *requestId, char *commandResponse, void *context, void *properties)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_COMMAND_RSP_REQ, requestId);

    return ReportData(topic, commandResponse, context, properties);
}

int ReportPropSetReponse(char *requestId, char *commandResponse, void *context)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_PROP_SET_RSP_REQ, requestId);

    return ReportData(topic, commandResponse, context, NULL);
}

int ReportPropGetReponse(char *requestId, char *commandResponse, void *context)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_PROP_GET_RSP_REQ, requestId);

    return ReportData(topic, commandResponse, context, NULL);
}

int GetPropertiesRequest(char *requestId, char *commandResponse, void *context)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_SHADOW_REQ, requestId);

    PrintfLog(EN_LOG_LEVEL_INFO, "DataTrans: topic is %s\n", topic);

    return ReportData(topic, commandResponse, context, NULL);
}

int EventUp(char *payload, void *context)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(3, TOPIC_PREFIX, username, TOPIC_SUFFIX_EVENT_UP);

    return ReportData(topic, payload, context, NULL);
}

// report sub device version or product information or subDevice scan result, deprecated
int ReportSubDeviceInfo(char *payload, void *context)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(3, TOPIC_PREFIX, username, TOPIC_SUFFIX_SUB_DEVICE_INFO_UP);

    return ReportData(topic, payload, context, NULL);
}

int ReportData(char *topic, char *payload, void *context, void *properties)
{
    if (topic == NULL || payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: ReportDeviceData() error, the input is invalid.\n");
        return IOTA_FAILURE;
    }
    int ret = MqttBase_publish((const char *)topic, payload, (int)strlen(payload), context, properties);
    MemFree(&topic);
    if (ret < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: ReportDeviceData() error, publish failed, result %d\n", ret);
        return IOTA_FAILURE;
    }

    return IOTA_SUCCESS;
}

int ReportDataSetQos(char *topic, char *payload, int qos, void *context, void *properties)
{
    if (topic == NULL || payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: ReportDeviceData() error, the input is invalid.\n");
        return IOTA_FAILURE;
    }
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: ReportDeviceData() error, qos not 0 or 1.\n");
        return IOTA_FAILURE;
    }
    int ret = MqttBase_publishSetQos((const char *)topic, payload, (int)strlen(payload), qos, context, properties);
    MemFree(&topic);
    if (ret < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: ReportDeviceData() error, publish failed, result %d\n", ret);
        return IOTA_FAILURE;
    }

    return ret;
}

// codecMode: 0 is json mode, others are binary mode
int ReportDevicePropertiesV3(char *payload, int codecMode, void *context)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = NULL;
    if (codecMode == 0) {
        topic = CombineStrings(4, TOPIC_PREFIX_V3, username, TOPIC_SUFFIX_DATA_REQ, JSON_V3);
    } else {
        topic = CombineStrings(4, TOPIC_PREFIX_V3, username, TOPIC_SUFFIX_DATA_REQ, BINARY_V3);
    }

    return ReportData(topic, payload, context, NULL);
}

int Bootstrap(char *payload)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
        return IOTA_FAILURE;
    }
    char *topic = NULL;
    topic = CombineStrings(3, TOPIC_PREFIX, username, BOOTSTRAP);
    int ret = MqttBase_publish((const char *)topic, payload, strlen(payload), NULL, NULL);
    MemFree(&topic);

    if (ret < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: Bootstrap() error, publish failed, result %d\n", ret);
        return IOTA_FAILURE;
    }

    return ret;
}

int OCM2MSendMsg(char *to, char *from, char *payload, char *requestId, char *context)
{
    char *topic = CombineStrings(6, TOPIC_PREFIX_M2M, to, M2M_FROM, from, TOPIC_SUFFIX_M2M, requestId);
    int ret = MqttBase_publish((const char*) topic, payload, (int)strlen(payload), context, NULL);
    MemFree(&topic);

    if (ret < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: OCM2MSendMsg() error, publish failed, result %d\n", ret);
        return IOTA_FAILURE;
    }

    return ret;
}
