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

#include "base.h"
#include "log_util.h"
#include "mqtt_base.h"
#include "string_util.h"
#include "subscribe.h"
#include "iota_error_type.h"

int SubscribeMessageDown() {
	char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (userName == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeMessageDown() getUserName failed.\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(3, TOPIC_PREFIX, userName, TOPIC_SUFFIX_MESSAGEDOWN);
	return SubsribeTopic(topic);
}

int SubscribeCommand() {
	char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (userName == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeCommand() getUserName failed.\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(4, TOPIC_PREFIX, userName, TOPIC_SUFFIX_COMMAND, WILDCARD);
	return SubsribeTopic(topic);
}

int SubscribePropSet() {
	char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (userName == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropSet() getUserName failed.\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(4, TOPIC_PREFIX, userName, TOPIC_SUFFIX_PROP_SET, WILDCARD);
	return SubsribeTopic(topic);
}

int SubscribePropget() {
	char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (userName == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropget() getUserName failed.\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(4, TOPIC_PREFIX, userName, TOPIC_SUFFIX_PROP_GET, WILDCARD);
	return SubsribeTopic(topic);
}

int SubscribePropResp() {
	char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (userName == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropResp() getUserName failed.\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(4, TOPIC_PREFIX, userName, TOPIC_SUFFIX_PROP_RSP, WILDCARD);
	return SubsribeTopic(topic);
}

int SubscribeSubDeviceEvent() {
	char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (userName == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeSubDeviceEvent() getUserName failed.\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(3, TOPIC_PREFIX, userName, TOPIC_SUFFIX_EVENT_DOWN);
	return SubsribeTopic(topic);
}

int SubscribeUserTopic(char *topicParas) {
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
	return SubsribeTopic(topic);
}


int SubsribeTopic(char *topic) {
	if (topic == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubsribeTopic() error, the topic is invalid.\n");
		return IOTA_FAILURE;
	}
	int ret = MqttBase_subscribe((const char*) topic);
	MemFree(&topic);

	if (ret < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeCommand() error, subscribe command failed, result %d\n", ret);
	}
	return ret;
}

int SubscribeJsonCmdV3() {
	char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (userName == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeJsonCmdV3() getUserName failed.\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(4, TOPIC_PREFIX_V3, userName, COMMAND_V3, JSON_V3);
	return SubsribeTopic(topic);
}

int SubscribeBinaryCmdV3() {
	char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (userName == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeBinaryCmdV3() getUserName failed.\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(4, TOPIC_PREFIX_V3, userName, COMMAND_V3, BINARY_V3);
	return SubsribeTopic(topic);
}

int SubscribeBootstrap() {
	char *userName = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (userName == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeBinaryCmdV3() getUserName failed.\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(3, TOPIC_PREFIX, userName, BOOTSTRAP_DOWN);
	return SubsribeTopic(topic);
}

void SubscribeAll() {
	if (SubscribeMessageDown() < IOTA_SUCCESS) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeMessageDown failed.\n");
	}
	if (SubscribeCommand() < IOTA_SUCCESS) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeCommand failed.\n");
	}
	if (SubscribePropSet() < IOTA_SUCCESS) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropSet failed.\n");
	}
	if (SubscribePropget() < IOTA_SUCCESS) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropget failed.\n");
	}
	if (SubscribeSubDeviceEvent() < IOTA_SUCCESS) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeSubDeviceEvent failed.\n");
	}
	if (SubscribePropResp() < IOTA_SUCCESS) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribePropResp failed.\n");
	}
	if (SubscribeBootstrap() < IOTA_SUCCESS) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "Subscribe: SubscribeBootstrap failed.\n");
	}
}
