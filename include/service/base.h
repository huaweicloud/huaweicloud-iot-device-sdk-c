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

#ifndef BASE_H
#define BASE_H

#define TOPIC_PREFIX                      "$oc/devices/"
#define TOPIC_SUFFIX_USER                 "/user/"
#define TOPIC_SUFFIX_MESSAGEUP            "/sys/messages/up"
#define TOPIC_SUFFIX_COMPRESS             "?encoding=gzip"
#define TOPIC_SUFFIX_MESSAGEDOWN          "/sys/messages/down"
#define TOPIC_SUFFIX_PROPS_REP            "/sys/gateway/sub_devices/properties/report"
#define TOPIC_SUFFIX_COMMAND              "/sys/commands/"
#define TOPIC_SUFFIX_COMMAND_RSP_REQ      "/sys/commands/response/request_id="
#define TOPIC_SUFFIX_PROP_REP             "/sys/properties/report"
#define TOPIC_SUFFIX_PROP_SET             "/sys/properties/set/"
#define TOPIC_SUFFIX_PROP_GET             "/sys/properties/get/"
#define TOPIC_SUFFIX_PROP_SET_RSP_REQ     "/sys/properties/set/response/request_id="
#define TOPIC_SUFFIX_PROP_GET_RSP_REQ     "/sys/properties/get/response/request_id="
#define TOPIC_SUFFIX_SHADOW_REQ           "/sys/shadow/get/request_id="
#define TOPIC_SUFFIX_PROP_RSP             "/sys/shadow/get/response/"
#define TOPIC_SUFFIX_EVENT_UP             "/sys/events/up"
#define TOPIC_SUFFIX_EVENT_DOWN           "/sys/events/down"
#define TOPIC_SUFFIX_SUB_DEVICE_INFO_UP   "/sys/sub_device_manage/messages/up"
#define WILDCARD                          "#"
#define TOPIC_PREFIX_V3                   "/huawei/v1/devices/"
#define TOPIC_SUFFIX_DATA_REQ             "/data/"
#define JSON_V3                           "json"
#define BINARY_V3                         "binary"
#define COMMAND_V3                        "/command/"
#define CLOUD_REQ                         "cloudReq"
#define BOOTSTRAP                         "/sys/bootstrap/up"
#define BOOTSTRAP_DOWN                    "/sys/bootstrap/down"
#define TOPIC_PREFIX_M2M                  "$oc/m2m/to/"
#define TOPIC_SUFFIX_M2M                  "/request_id="
#define M2M_FROM                          "/from/"
#define FORWARD_SLASH                     "/"

// bridge
#define TOPIC_PREFIX_BRIDGE               "$oc/bridges/"
#define TOPIC_SUFFIX_SYS                  "/sys/"
#define TOPIC_SUFFIX_LOGIN                "/sys/login/response/request_id="
#define TOPIC_SUFFIX_LOGOUT               "/sys/logout/response/request_id="
#define TOPIC_SUFFIX_RESET_SECRET         "/sys/reset_secret/response/request_id="
#define TOPIC_SUFFIX_DISCONNECT           "/sys/disconnect"
#define TOPIC_SUFFIX_DEVICES              "/devices/"

// see also EN_MQTT_BASE_CALLBACK_SETTING in MqttBase.h
typedef enum enum_BASE_CONFIG {
    EN_BASE_CONFIG_USERNAME = 0,
    EN_BASE_CONFIG_PASSWORD = 1,
    EN_BASE_CONFIG_SERVER_ADDR = 5,
    EN_BASE_CONFIG_SERVER_PORT = 6,
    EN_BASE_CONFIG_AUTH_MODE = 9,
    EN_BASE_CONFIG_LOG_LOCAL_NUMBER = 10,
    EN_BASE_CONFIG_LOG_LEVEL = 11,
    EN_BASE_CONFIG_KEEP_ALIVE_TIME = 12,
    EN_BASE_CONFIG_CONNECT_TIMEOUT = 13,
    EN_BASE_CONFIG_RETRY_INTERVAL = 14,
    EN_BASE_CONFIG_RESET_SECRET_IN_PROGRESS = 15,
    EN_BASE_CONFIG_QOS = 16,
} ENUM_BASE_CONFIG;

typedef enum {
    EN_CALLBACK_CONNECT_SUCCESS = 0,
    EN_CALLBACK_CONNECT_FAILURE = 1,
    EN_CALLBACK_DISCONNECT_SUCCESS = 2,
    EN_CALLBACK_DISCONNECT_FAILURE = 3,
    EN_CALLBACK_CONNECTION_LOST = 4,
    EN_CALLBACK_PUBLISH_SUCCESS = 5,
    EN_CALLBACK_PUBLISH_FAILURE = 6,
    EN_CALLBACK_SUBSCRIBE_SUCCESS = 7,
    EN_CALLBACK_SUBSCRIBE_FAILURE = 8,
} EN_CALLBACK_SETTING;

int init(char *workPath);
int SetConfig(int item, char *value);
char *GetConfig(int item);
int destory(void);

#endif /* BASE_H */

