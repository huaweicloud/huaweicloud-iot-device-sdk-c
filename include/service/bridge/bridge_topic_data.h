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
#ifndef BRIDGE_TOPIC_H
#define BRIDGE_TOPIC_H

#define BRIDGE_TOPIC_PREFIX             "$oc/bridges/"
#define BRIDGE_TOPIC_DEVICE             "/devices/"
#define BRIDGE_TOPIC_LOGIN              "/sys/login/request_id="
#define BRIDGE_TOPIC_LOGOUT             "/sys/logout/request_id="
#define BRIDGE_TOPIC_RESET_SECRET       "/sys/reset_secret/request_id="
#define BRIDGE_TOPIC_MESSAGES_UP        "/sys/messages/up"
#define BRIDGE_TOPIC_COMMANDS_RESPONSE  "/sys/commands/response/request_id="
#define BRIDGE_TOPIC_PROPERTIES         "/sys/properties/report"
#define BRIDGE_TOPIC_GATEWAY_PROPERTIES "/gateway/sub_devices/properties/report"
#define BRIDGE_TOPIC_SET_PROPERTIES     "/sys/properties/set/response/request_id="
#define BRIDGE_TOPIC_GET_PROPERTIES     "/sys/properties/get/response/request_id="
#define BRIDGE_TOPIC_GET_SHADOW         "/sys/shadow/get/request_id="
#define BRIDGE_TOPIC_EVENTS_UP          "/sys/events/up"

typedef enum {
    BRIDGE_DEVICE_LOGIN,
    BRIDGE_DEVICE_LOGOUT,
    BRIDGE_DEVICE_RESET_SECRET,
    BRIDGE_DEVICE_MESSAGES_UP,
    BRIDGE_DEVICE_COMMANDS_RESPONSE,
    BRIDGE_DEVICE_PROPERTIES,
    BRIDGE_DEVICE_GATEWAY_PROPERTIES,
    BRIDGE_DEVICE_SET_PROPERTIES,
    BRIDGE_DEVICE_GET_PROPERTIES,
    BRIDGE_DEVICE_GET_SHADOW,
    BRIDGE_DEVICE_EVENTS_UP
} BRIDGE_DEVICE_TYPE;

int Bridge_ReportDeviceData(char *payload, BRIDGE_DEVICE_TYPE type, char *deviceId, char *requestId, void *context, void *properties);

#endif
