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
#include "data_trans.h"
#include "log_util.h"
#include "mqtt_base.h"
#include "string_util.h"
#include "cJSON.h"
#include "iota_error_type.h"

int ReportDeviceData(char *payload, char *topicParas) {
	char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (username == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
		return IOTA_FAILURE;
	}
	char *topic = NULL;

	if (topicParas == NULL) {
		topic = CombineStrings(3, TOPIC_PREFIX, username, TOPIC_SUFFIX_MESSAGEUP);
	} else {
		topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_USER, topicParas);
	}

	return ReportData(topic, payload);
}

int ReportDeviceProperties(char *payload) {
	char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (username == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(3, TOPIC_PREFIX, username, TOPIC_SUFFIX_PROP_REP);

	return ReportData(topic, payload);
}

int ReportBatchDeviceProperties(char *payload) {
	char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (username == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(3, TOPIC_PREFIX, username, TOPIC_SUFFIX_PROPS_REP);

	return ReportData(topic, payload);
}

int ReportCommandReponse(char *requestId, char *pcCommandRespense) {
	char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (username == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_COMMAND_RSP_REQ, requestId);

	return ReportData(topic, pcCommandRespense);
}

int ReportPropSetReponse(char *requestId, char *pcCommandRespense) {
	char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (username == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_PROP_SET_RSP_REQ, requestId);

	return ReportData(topic, pcCommandRespense);
}

int ReportPropGetReponse(char *requestId, char *pcCommandRespense) {
	char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (username == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_PROP_GET_RSP_REQ, requestId);

	return ReportData(topic, pcCommandRespense);
}

int GetPropertiesRequest(char *requestId, char *pcCommandRespense) {
	char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (username == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(4, TOPIC_PREFIX, username, TOPIC_SUFFIX_SHADOW_REQ, requestId);

	PrintfLog(EN_LOG_LEVEL_INFO, "DataTrans: topic is %s\n", topic);

	return ReportData(topic, pcCommandRespense);
}

int EventUp(char *payload) {
	char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (username == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(3, TOPIC_PREFIX, username, TOPIC_SUFFIX_EVENT_UP);

	PrintfLog(EN_LOG_LEVEL_INFO, "DataTrans: topic is %s\n", topic);

	return ReportData(topic, payload);
}

//report sub device version or product information or subDevice scan result, deprecated
int ReportSubDeviceInfo(char *payload) {
	char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
	if (username == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: getUserName failed %d\n");
		return IOTA_FAILURE;
	}
	char *topic = CombineStrings(3, TOPIC_PREFIX, username, TOPIC_SUFFIX_SUB_DEVICE_INFO_UP);

	return ReportData(topic, payload);
}

int ReportData(char *topic, char *payload) {
	if (topic == NULL || payload == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: ReportDeviceData() error, the input is invalid.\n");
		return IOTA_FAILURE;
	}
	int ret = MqttBase_publish((const char*) topic, payload);
	MemFree(&topic);

	if (ret < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "DataTrans: ReportDeviceData() error, publish failed, result %d\n", ret);
		return IOTA_FAILURE;
	}

	return IOTA_SUCCESS;
}

