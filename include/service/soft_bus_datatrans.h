/*
 * Copyright (c) 2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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


#ifndef SOFT_BUS_DATATRANS_H
#define SOFT_BUS_DATATRANS_H
#include <stdbool.h>

#define SOFT_BUS_OH
#define SUCCESSSFUL_RESULT          "success"
#define SUCCESSFUL_RET              0
#define BUS_INFOS                   "bus_infos"
#define BUS_COUNT                   "bus_count"
#define OH_ERROR_INFO               "error_info"
#define OH_ERROR_CODE               "error_code"
#define OH_ERROR_MSG                "error_msg"
#define DEFAULT_ERROR_CODE          "-1"
#define DEFAULT_ERROR_MSG           "unknown"
#define BUS_ID                      "bus_id"
#define BUS_KEY                     "bus_key"
#define DEVICE_COUNT                "device_count"
#define DEVICES_INFO                "devices_info"
#define BUS_KEY_LENGTH              20
#define SOFTBUS_TOTAL_LEN           10
#define SOFTBUS_INFO_LEN            50

typedef struct {
    char *device_id;
    char *device_ip;
} soft_bus_info;

typedef struct {
    int count;
    char *bus_key;
    char *bus_id;
    soft_bus_info g_device_soft_bus_info[SOFTBUS_INFO_LEN];
    long version;
} soft_bus_infos;

typedef struct {
    soft_bus_infos g_soft_bus_info[SOFTBUS_TOTAL_LEN];
    int count;
} soft_bus_total;

soft_bus_total *getSoftBusTotal(void);
bool isValidIP(const char *ip_addr);
bool isValidDeviceID(const char *device_id);
char *getIpAddr(char *device_id);
char *getAuthKey(const char *src_device_id, const char *target_device_id);
char *getDeviceId(void);
char *releaseSoftBusCache(void);

#endif /* SOFT_BUS_DATATRANS_H */