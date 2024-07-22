/*
 * Copyright (c) 2022-2024 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data_trans.h"
#include "iota_error_type.h"
#include "bridge_topic_data.h"
#include "mqtt_base.h"
#include "log_util.h"
#include "string_util.h"

static char *Birdge_SetTopic(BRIDGE_DEVICE_TYPE mode, char *deviceId, char *requestId)
{
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (username == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "ReportDeviceData: getUserName failed %d\n");
        return NULL;
    }

    char *topic = NULL;
    switch (mode) {
        case BRIDGE_DEVICE_LOGIN:
            topic = CombineStrings(6, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_LOGIN, requestId);
            break;
        case BRIDGE_DEVICE_LOGOUT:
            topic = CombineStrings(6, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_LOGOUT, requestId);
            break;
        case BRIDGE_DEVICE_RESET_SECRET:
            topic = CombineStrings(6, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_RESET_SECRET, requestId);
            break;
        case BRIDGE_DEVICE_MESSAGES_UP:
            topic = CombineStrings(5, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_MESSAGES_UP);
            break;
        case BRIDGE_DEVICE_COMMANDS_RESPONSE:
            topic = CombineStrings(6, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_COMMANDS_RESPONSE, requestId);
            break;
        case BRIDGE_DEVICE_PROPERTIES:
            topic = CombineStrings(5, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_PROPERTIES);
            break;
        case BRIDGE_DEVICE_GATEWAY_PROPERTIES:
            topic = CombineStrings(5, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_GATEWAY_PROPERTIES);
            break;
        case BRIDGE_DEVICE_SET_PROPERTIES:
            topic = CombineStrings(6, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_SET_PROPERTIES, requestId);
            break;
        case BRIDGE_DEVICE_GET_PROPERTIES:
            topic = CombineStrings(6, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_GET_PROPERTIES, requestId);
            break;
        case BRIDGE_DEVICE_GET_SHADOW:
            topic = CombineStrings(6, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_GET_SHADOW, requestId);
            break;
        case BRIDGE_DEVICE_EVENTS_UP:
            topic = CombineStrings(5, BRIDGE_TOPIC_PREFIX, username, BRIDGE_TOPIC_DEVICE, deviceId, BRIDGE_TOPIC_EVENTS_UP);
            break;
        default:
            return NULL;
        return topic;
    }
}

int Bridge_ReportDeviceData(char *payload, BRIDGE_DEVICE_TYPE type, char *deviceId, char *requestId, void *context, void *properties)
{
    char *topic = Birdge_SetTopic(type, deviceId, requestId);
    if (topic == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Bridge_ReportDeviceData(), Topic cannot be NULL\n");
        return IOTA_FAILURE;
    }
    int ret = ReportData(topic, payload, context, properties);
    return ret;
}
