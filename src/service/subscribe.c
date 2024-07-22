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

#include "base.h"
#include "log_util.h"
#include "mqtt_base.h"
#include "string_util.h"
#include "iota_error_type.h"
#include "subscribe.h"

static int SubsribeTopic(char *topic, const int qos);

int SubscribeMessageDownQos(int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeMessageDown() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeMessageDown() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(3, TOPIC_PREFIX, userName, TOPIC_SUFFIX_MESSAGEDOWN);
    return SubsribeTopic(topic, qos);
}

int SubscribeMessageDown(void)
{
    SubscribeMessageDownQos(0);
}

int SubscribeCommandQos(int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeCommand() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeCommand() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX, userName, TOPIC_SUFFIX_COMMAND, WILDCARD);
    return SubsribeTopic(topic, qos);
}

int SubscribeCommand(void)
{
    SubscribeCommandQos(0);
}

int SubscribePropSetQos(int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropSet() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropSet() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX, userName, TOPIC_SUFFIX_PROP_SET, WILDCARD);
    return SubsribeTopic(topic, qos);
}

int SubscribePropSet(void)
{
    SubscribePropSetQos(0);
}

int SubscribePropgetQos(int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropget() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropget() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX, userName, TOPIC_SUFFIX_PROP_GET, WILDCARD);
    return SubsribeTopic(topic, qos);
}

int SubscribePropget(void)
{
    SubscribePropgetQos(0);
}

int SubscribePropRespQos(int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropResp() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropResp() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX, userName, TOPIC_SUFFIX_PROP_RSP, WILDCARD);
    return SubsribeTopic(topic, qos);
}

int SubscribePropResp(void)
{
    SubscribePropRespQos(0);
}

int SubscribeSubDeviceEventQos(int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeSubDeviceEvent() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeSubDeviceEvent() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(3, TOPIC_PREFIX, userName, TOPIC_SUFFIX_EVENT_DOWN);
    return SubsribeTopic(topic, qos);
}

int SubscribeSubDeviceEvent(void)
{
    SubscribeSubDeviceEventQos(0);
}

int SubscribeUserTopicQos(char *topicParas, int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeUserTopic() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    if (topicParas == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeUserTopic() the topicParas is invalid.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeUserTopic() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX, userName, TOPIC_SUFFIX_USER, topicParas);
    return SubsribeTopic(topic, qos);
}

int SubscribeUserTopic(char *topicParas)
{
    SubscribeUserTopicQos(topicParas, 0);
}

int SubscribeCustomTopic(char *topic, const int qos) 
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeBinaryCmdV3() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *topicMalloc = CombineStrings(1, topic);
    return SubsribeTopic(topicMalloc, qos);
}

static int SubsribeTopic(char *topic, const int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeBinaryCmdV3() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    if (topic == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubsribeTopic() error, the topic is invalid.\n");
        return IOTA_FAILURE;
    }
    int ret = MqttBase_subscribe((const char *)topic, qos);
    MemFree(&topic);

    if (ret < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeCommand() error, subscribe command failed, result %d\n",
            ret);
    }
    return ret;
}

int SubscribeJsonCmdV3Qos(int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeBinaryCmdV3() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeJsonCmdV3() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX_V3, userName, COMMAND_V3, JSON_V3);
    return SubsribeTopic(topic, qos);
}

int SubscribeJsonCmdV3(void)
{
    SubscribeJsonCmdV3Qos(0);
}

int SubscribeBinaryCmdV3Qos(int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeBinaryCmdV3() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeBinaryCmdV3() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX_V3, userName, COMMAND_V3, BINARY_V3);
    return SubsribeTopic(topic, qos);
}

int SubscribeBinaryCmdV3(void)
{
    SubscribeBinaryCmdV3Qos(0);
}

int SubscribeBootstrapQos(int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeBinaryCmdV3() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeBinaryCmdV3() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(3, TOPIC_PREFIX, userName, BOOTSTRAP_DOWN);
    return SubsribeTopic(topic, qos);
}

int SubscribeBootstrap(void)
{
    SubscribeBootstrapQos(0);
}

int SubscribeM2mQos(int qos)
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeM2m() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (userName == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeM2m() getUserName failed.\n");
        return IOTA_FAILURE;
    }
    char *topic = CombineStrings(4, TOPIC_PREFIX_M2M, userName, FORWARD_SLASH, WILDCARD);
    return SubsribeTopic(topic, qos);
}

int SubscribeM2m(void)
{
    SubscribeM2mQos(0);
}
 
void SubscribeAllQos(int qos) 
{
    if (qos < 0 || qos > 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeAll() qos only supports 0 or 1.\n");
        return IOTA_FAILURE;
    }
    if (SubscribeMessageDownQos(qos) < IOTA_SUCCESS) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeMessageDown failed.\n");
    }
    if (SubscribeCommandQos(qos) < IOTA_SUCCESS) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeCommand failed.\n");
    }
    if (SubscribePropSetQos(qos) < IOTA_SUCCESS) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropSet failed.\n");
    }
    if (SubscribePropgetQos(qos) < IOTA_SUCCESS) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropget failed.\n");
    }
    if (SubscribeSubDeviceEventQos(qos) < IOTA_SUCCESS) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeSubDeviceEvent failed.\n");
    }
    if (SubscribePropRespQos(qos) < IOTA_SUCCESS) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropResp failed.\n");
    }
}

void SubscribeAll(void)
{
    SubscribeAllQos(0);
}
