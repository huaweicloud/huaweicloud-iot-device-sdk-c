/*
 * Copyright (c) 2023-2025 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#include <stdbool.h>
#include <string.h>
#include "mqtt_base.h"
#include "callback_func.h"
#include "string_util.h"
#include "soft_bus_datatrans.h"
#include "log_util.h"

soft_bus_total g_soft_bus_total;

// 获取g_soft_bus_total
soft_bus_total *getSoftBusTotal()
{
    return &g_soft_bus_total;
}

// 校验当前连接服务的客户端的IP地址是否合法
bool isValidIP(const char *ip_addr)
{
    if (ip_addr == NULL) {
        return false;
    }
    int count_tmp_infos = g_soft_bus_total.count;
    int i;
    for (i = 0; i < count_tmp_infos; i++) {
        int count_tmp = g_soft_bus_total.g_soft_bus_info[i].count;
        int j;
        for (j = 0; j < count_tmp; j++) {
            if (strcmp(g_soft_bus_total.g_soft_bus_info[i].g_device_soft_bus_info[j].device_ip, ip_addr) == 0) {
                return true;
            }
        }
    }
    return false;
}

// 校验当前连接服务的客户端的设备ID是否合法
bool isValidDeviceID(const char *device_id)
{
    if (device_id == NULL) {
        return false;
    }
    int count_tmp_infos = g_soft_bus_total.count;
    int i;
    for (i = 0; i < count_tmp_infos; i++) {
        int count_tmp = g_soft_bus_total.g_soft_bus_info[i].count;
        int j;
        for (j = 0; j < count_tmp; j++) {
            if (strcmp(g_soft_bus_total.g_soft_bus_info[i].g_device_soft_bus_info[j].device_id, device_id) == 0) {
                return true;
            }
        }
    }
    return false;
}

// 获取对应设备的IP地址
char *getIpAddr(char *device_id)
{
    if (device_id == NULL) {
        return NULL;
    }
    int count_tmp_infos = g_soft_bus_total.count;
    int i;
    for (i = 0; i < count_tmp_infos; i++) {
        int count_tmp = g_soft_bus_total.g_soft_bus_info[i].count;
        int j;
        for (j = 0; j < count_tmp; j++) {
            if (strcmp(g_soft_bus_total.g_soft_bus_info[i].g_device_soft_bus_info[j].device_id, device_id) == 0) {
                int size = strlen(g_soft_bus_total.g_soft_bus_info[i].g_device_soft_bus_info[j].device_ip);
                char *result = malloc(size + 1);
                strncpy_s(result, size, g_soft_bus_total.g_soft_bus_info[i].g_device_soft_bus_info[j].device_ip, size);
                result[size] = '\0';
                return result;
            }
        }
    }
    return NULL;
}

// 获取设备认证的PIN码数组, 这个src_device_id是否只能是当前的device_id
char *getAuthKey(const char *src_device_id, const char *target_device_id)
{
    if ((src_device_id == NULL) || (target_device_id == NULL)) {
        return NULL;
    }
    char *username = MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
    if (strcmp(src_device_id, username) != 0) {
        return NULL;
    }
    char *result = NULL;    // bus_key的长度为16
    int count_tmp_infos = g_soft_bus_total.count;
    int i;
    for (i = 0; i < count_tmp_infos; i++) {
        int count_tmp = g_soft_bus_total.g_soft_bus_info[i].count;
        int j;
        for (j = 0; j < count_tmp; j++) {
            if (strcmp(g_soft_bus_total.g_soft_bus_info[i].g_device_soft_bus_info[j].device_id,
                target_device_id) == 0) {
                if (result == NULL) {
                    result = (char *)malloc((BUS_KEY_LENGTH + 1) * sizeof(char));
                    strncpy_s(result, BUS_KEY_LENGTH, g_soft_bus_total.g_soft_bus_info[i].bus_key,
                        strlen(g_soft_bus_total.g_soft_bus_info[i].bus_key) + 1);
                    result[strlen(result)] = '\0';
                } else {
                    int resultLen = strlen(result) + (BUS_KEY_LENGTH + 1);
                    result = ReassignMemory(result, resultLen);
                    strncat_s(result, resultLen, ",", strlen(","));
                    strncat_s(result, resultLen, g_soft_bus_total.g_soft_bus_info[i].bus_key,
                    strlen(g_soft_bus_total.g_soft_bus_info[i].bus_key));
                }
                break;
            }
        }
    }
    return result;
}

// 获取本机的设备ID
char *getDeviceId(void)
{
    return MqttBase_GetConfig(EN_MQTT_BASE_CONFIG_USERNAME);
}

// 释放内存
int releaseSoftBusCache(void)
{
    int total_cnt = g_soft_bus_total.count;
    int i;
    // erase the storage of soft bus infos
    for (i = 0; i < total_cnt; i++) {
        int tmp_count = g_soft_bus_total.g_soft_bus_info[i].count;
        int j;
        for (j = 0; j < tmp_count; j++) {
            MemFree(&(g_soft_bus_total.g_soft_bus_info[i].g_device_soft_bus_info[j].device_id));
            MemFree(&(g_soft_bus_total.g_soft_bus_info[i].g_device_soft_bus_info[j].device_ip));
        }
        g_soft_bus_total.g_soft_bus_info[i].count = 0;
        g_soft_bus_total.g_soft_bus_info[i].version = -1;
        MemFree(&g_soft_bus_total.g_soft_bus_info[i].bus_id);
        MemFree(&g_soft_bus_total.g_soft_bus_info[i].bus_key);
        g_soft_bus_total.g_soft_bus_info[i].version = -1;
    }
    g_soft_bus_total.count = 0;
    return 0;
}

// 数据处理, 把软总线数据josn格式, 解析成结构体soft_bus_total, 并存放在g_soft_bus_total中。
int softBusParasToTotal(JSON *paras)
{
    cJSON *busInfos = cJSON_GetObjectItem(paras, BUS_INFOS);
    if (busInfos == NULL) {
        return -1;
    }

    int  i = 0, j = 0;
    JSON *busInfosEvent = NULL;
    JSON *deviceInfoEvent = NULL;
    cJSON_ArrayForEach(busInfosEvent, busInfos) {
        cJSON *deviceInfo = cJSON_GetObjectItem(busInfosEvent, DEVICES_INFO);
        char *busKey = JSON_GetStringFromObject(busInfosEvent, BUS_KEY, "-1");
        char *busId = JSON_GetStringFromObject(busInfosEvent, BUS_ID, "-1");
        soft_bus_infos *softBusInfos = &g_soft_bus_total.g_soft_bus_info[i];
        cJSON_ArrayForEach(deviceInfoEvent, deviceInfo) {
            char *deviceId = JSON_GetStringFromObject(deviceInfoEvent, "device_id", "-1");
            char *deviceIp = JSON_GetStringFromObject(deviceInfoEvent, "device_ip", "-1");
            if (strcmp(deviceId, "-1") == 0 || strcmp(deviceIp, "-1") == 0) {
                continue;
            }
            soft_bus_info *softBusInfo = &softBusInfos->g_device_soft_bus_info[j];
            MemFree(&softBusInfo->device_id);
            MemFree(&softBusInfo->device_ip);
            softBusInfo->device_id = CombineStrings(1, deviceId);
            softBusInfo->device_ip = CombineStrings(1, deviceIp);
            j++;
            if (j >= SOFTBUS_INFO_LEN) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "the soft_bus_info Exceeding the limit value");
                break;
            }
        }
        softBusInfos->count = j;
        i++;
        j = 0;
        if (i >= SOFTBUS_TOTAL_LEN) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "the soft_bus_infos Exceeding the limit value");
            break;
        }
        MemFree(&softBusInfos->bus_key);
        MemFree(&softBusInfos->bus_id);
        softBusInfos->bus_key = CombineStrings(1, busKey);
        softBusInfos->bus_id = CombineStrings(1, busId);
    }
    g_soft_bus_total.count = i;

    return 0;
}