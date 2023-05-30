/*
 * Copyright (c) 2022-2022 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "hw_type.h"
#include "iota_datatrans.h"
#include "iota_error_type.h"
#include "data_trans.h"
#include "log_util.h"
#include "string_util.h"
#include "detect_anomaly.h"

#define SWITCH_ON 1
#define INT_TO_PERCENT 100
#define SERVICE_NUM 1

static HW_BOOL g_reportMemoryTaskRunning = HW_FALSE;
static pthread_t g_reportMemoryTaskId;
static SecurityDetection g_securityDetection;

int Detect_GetShadowDetectAnomaly(void)
{
    return IOTA_GetDeviceShadow(SECURITY_DETECTION_CONFIG_REQUEST_ID, NULL, SECURITY_DETECTION_CONFIG, NULL);
}

static void Detect_ReportShadowDesired(SecurityDetection securityDetection)
{
    cJSON *properties = cJSON_CreateObject();
    ST_IOTA_SERVICE_DATA_INFO services[1];

    cJSON_AddNumberToObject(properties, MEMORY_CHECK, securityDetection.memoryCheck);
    cJSON_AddNumberToObject(properties, MEMORY_THRESHOLD, securityDetection.memoryThreshold);
    cJSON_AddNumberToObject(properties, PORT_CHECK, securityDetection.portCheck);
    cJSON_AddNumberToObject(properties, CPU_USAGE_CHECK, securityDetection.cpuUsageCheck);
    cJSON_AddNumberToObject(properties, CPU_USAGE_THRESHOLD, securityDetection.cpuUsageThreshold);
    cJSON_AddNumberToObject(properties, DISK_SPACE_CHECK, securityDetection.diskSpaceCheck);
    cJSON_AddNumberToObject(properties, DISK_SPACE_THRESHOLD, securityDetection.diskSpaceThreshold);
    cJSON_AddNumberToObject(properties, BATTERY_PERCENTAGE_CHECK, securityDetection.batteryPercentageCheck);
    cJSON_AddNumberToObject(properties, BATTERY_PERCENTAGE_THRESHOLD, securityDetection.batteryPercentageThreshold);
    char *service = cJSON_Print(properties);
    cJSON_Delete(properties);

    services[0].event_time = NULL;
    services[0].service_id = SECURITY_DETECTION_CONFIG;
    services[0].properties = service;
    int messageId = IOTA_PropertiesReport(services, SERVICE_NUM, 0, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Detect_ReportShadowDesired: IoTA_PropertiesReport() failed, messageId %d\n",
            messageId);
    }
    MemFree(&service);
}

static void Detect_ReportModuleInfo(char *reportType, cJSON *content)
{
    cJSON *root;
    cJSON *services;
    cJSON *serviceEvent;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    serviceEvent = cJSON_CreateObject();
    char *eventTimesStamp = GetEventTimesStamp();

    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, LOG);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, SECURITY_LOG_REPORT);
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, eventTimesStamp);

    cJSON *paras = cJSON_CreateObject();
    cJSON_AddStringToObject(paras, TIMESTAMP, eventTimesStamp);
    cJSON_AddStringToObject(paras, TYPE, reportType);

    cJSON_AddItemToObject(paras, CONTENT, content);
    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    MemFree(&eventTimesStamp);

    if (payload == NULL) {
         PrintfLog(EN_LOG_LEVEL_ERROR, "Detect_ReportMemoryInfo with payload = %s failed\n", payload);
    } else {
        (void)EventUp(payload, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Detect_ReportMemoryInfo with payload = %s\n", payload);
        MemFree(&payload);
    }
}

static void Detect_ReportMemoryInfo(void)
{
    int isLeakAlarm = 0;
    MemoryInfo memoryInfo;
    memoryInfo.total = SysHalGetTotalMemory();
    memoryInfo.used = SysHalGetMemoryUsed();

    if (memoryInfo.used >= g_securityDetection.memoryThreshold * memoryInfo.total / INT_TO_PERCENT) {
        isLeakAlarm = 1;
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the memory usage exceeds threshold\n");
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the memory is enough, memoryUsed = %ld\n", memoryInfo.used);
    }
    memoryInfo.leakAlarm = isLeakAlarm;
    cJSON *content = cJSON_CreateObject();
    cJSON_AddNumberToObject(content, USED, memoryInfo.used);
    cJSON_AddNumberToObject(content, TOTAL, memoryInfo.total);
    cJSON_AddNumberToObject(content, LEAK_ALARM, memoryInfo.leakAlarm);
    Detect_ReportModuleInfo(MEMORY_REPORT, content);
}

static void Detect_ReportPortInfo(void)
{
    cJSON *used = NULL;
    ArrayInfo *arrayInfo = SysHalGetPortUsed();
    cJSON *content = cJSON_CreateObject();
    if (arrayInfo != NULL) {
        used = cJSON_CreateIntArray(arrayInfo->array, arrayInfo->arrayLen);
    }
    cJSON_AddItemToObject(content, USED, used);
    Detect_ReportModuleInfo(PORT_REPORT, content);
}

static void Detect_ReportCpuUsageInfo(void)
{
    int cpuAlarm = 0;
    CpuUsageInfo cpuUsageInfo;
    cpuUsageInfo.cpuUsage = SysHalGetCpuUsage();
    if (cpuUsageInfo.cpuUsage >= g_securityDetection.cpuUsageThreshold) {
        cpuAlarm = 1;
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the cpu usage exceeds threshold\n");
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the cpu is enough, cpuUsage = %ld\n", cpuUsageInfo.cpuUsage);
    }
    cpuUsageInfo.cpuAlarm = cpuAlarm;
     
    cJSON *content = cJSON_CreateObject();
    cJSON_AddNumberToObject(content, CPU_USAGE, cpuUsageInfo.cpuUsage);
    cJSON_AddNumberToObject(content, CPU_USAGE_ALARM, cpuUsageInfo.cpuAlarm);
    Detect_ReportModuleInfo(CPU_USAGE_REPORT, content);
}

static void Detect_ReportDiskSpaceInfo(void)
{
    int diskAlarm = 0;
    DiskSpaceInfo diskSpaceInfo;
    diskSpaceInfo.diskSpaceTotal = SysHalGetTotalDiskSpace();
    diskSpaceInfo.diskSpaceUsed = SysHalGetDiskSpaceUsed();
    if (diskSpaceInfo.diskSpaceUsed >= g_securityDetection.diskSpaceThreshold * diskSpaceInfo.diskSpaceTotal
        / INT_TO_PERCENT) {
        diskAlarm = 1;
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the disk space used exceeds threshold\n");
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the disk space is enough, diskSpaceUsed = %ld\n", diskSpaceInfo.diskSpaceUsed);
    }
    diskSpaceInfo.diskSpaceAlarm = diskAlarm;

    cJSON *content = cJSON_CreateObject();
    cJSON_AddNumberToObject(content, DISK_SPACE_USED, diskSpaceInfo.diskSpaceUsed);
    cJSON_AddNumberToObject(content, DISK_SPACE_TOTAL, diskSpaceInfo.diskSpaceTotal);
    cJSON_AddNumberToObject(content, DISK_SPACE_ALARM, diskSpaceInfo.diskSpaceAlarm);
    Detect_ReportModuleInfo(DISK_SPACE_REPORT, content);
}

static void Detect_ReportBatteryInfo(void)
{
    int batteryAlarm = 0;
    BatteryInfo batteryInfo;
    batteryInfo.batteryPct = SysHalGetBatteryPercentage();
    if (batteryInfo.batteryPct <= g_securityDetection.batteryPercentageThreshold) {
        batteryAlarm = 1;
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the battery is low \n");
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the battery is enough, batteryPct = %ld\n", batteryInfo.batteryPct);
    }
    batteryInfo.batteryAlarm = batteryAlarm;

    cJSON *content = cJSON_CreateObject();
    cJSON_AddNumberToObject(content, BATTERY_PERCENTAGE, batteryInfo.batteryPct);
    cJSON_AddNumberToObject(content, BATTERY_PERCENTAGE_ALARM, batteryInfo.batteryAlarm);
    Detect_ReportModuleInfo(BATTERY_REPORT, content);
}

static void *Detect_ReportDetectionInfoEntry(void *args)
{
    while (1) {
        if (g_securityDetection.memoryCheck == SWITCH_ON) {
            Detect_ReportMemoryInfo();
        }
        if (g_securityDetection.portCheck == SWITCH_ON) {
            Detect_ReportPortInfo();
        }
        if (g_securityDetection.cpuUsageCheck == SWITCH_ON) {
            Detect_ReportCpuUsageInfo();
        }
        if (g_securityDetection.diskSpaceCheck == SWITCH_ON) {
            Detect_ReportDiskSpaceInfo();
        }
        if (g_securityDetection.batteryPercentageCheck == SWITCH_ON) {
            Detect_ReportBatteryInfo();
        }
        
        usleep(DETECT_REPORT_FREQUENCY * 1000 * 1000);
    }
    return args; // for pthread interface
}

static void Detect_ReportDetectionInfo(void)
{
    int securityConfig = g_securityDetection.memoryCheck || g_securityDetection.portCheck
        || g_securityDetection.cpuUsageCheck || g_securityDetection.diskSpaceCheck
        || g_securityDetection.batteryPercentageCheck;
    if ((securityConfig == SWITCH_ON) && !g_reportMemoryTaskRunning) {
        pthread_create(&g_reportMemoryTaskId, NULL, Detect_ReportDetectionInfoEntry, NULL);
        g_reportMemoryTaskRunning = HW_TRUE;
    } else if ((securityConfig != SWITCH_ON) && g_reportMemoryTaskRunning) {
        (void)pthread_cancel(g_reportMemoryTaskId);
        pthread_join(g_reportMemoryTaskId, NULL);
        g_reportMemoryTaskRunning = HW_FALSE;
    }
}

void Detect_ParseShadowGetOrPropertiesSet(char *propertiesDown)
{
    cJSON *properties;
    cJSON *memoryCheck;
    cJSON *memoryThreshold;
    cJSON *portCheck;
    cJSON *cpuUsageCheck;
    cJSON *cpuUsageThreshold;
    cJSON *diskSpaceCheck;
    cJSON *diskSpaceThreshold;
    cJSON *batteryPctCheck;
    cJSON *batteryPctThreshold;

    properties = cJSON_Parse(propertiesDown);
    if (properties == NULL) {
        return;
    }

    memoryCheck = cJSON_GetObjectItem(properties, MEMORY_CHECK);
    if (memoryCheck != NULL) {
        g_securityDetection.memoryCheck = cJSON_GetNumberValue(memoryCheck);
    }

    memoryThreshold = cJSON_GetObjectItem(properties, MEMORY_THRESHOLD);
    if (memoryThreshold != NULL) {
        g_securityDetection.memoryThreshold = cJSON_GetNumberValue(memoryThreshold);
    }

    portCheck = cJSON_GetObjectItem(properties, PORT_CHECK);
    if (portCheck != NULL) {
        g_securityDetection.portCheck = cJSON_GetNumberValue(portCheck);
    }

    cpuUsageCheck = cJSON_GetObjectItem(properties, CPU_USAGE_CHECK);
    if (cpuUsageCheck != NULL) {
        g_securityDetection.cpuUsageCheck = cJSON_GetNumberValue(cpuUsageCheck);
    }
    
    cpuUsageThreshold = cJSON_GetObjectItem(properties, CPU_USAGE_THRESHOLD);
    if (cpuUsageThreshold != NULL) {
        g_securityDetection.cpuUsageThreshold = cJSON_GetNumberValue(cpuUsageThreshold);
    }
    
    diskSpaceCheck = cJSON_GetObjectItem(properties, DISK_SPACE_CHECK);
    if (diskSpaceCheck != NULL) {
        g_securityDetection.diskSpaceCheck = cJSON_GetNumberValue(diskSpaceCheck);
    }

    diskSpaceThreshold = cJSON_GetObjectItem(properties, DISK_SPACE_THRESHOLD);
    if (diskSpaceThreshold != NULL) {
        g_securityDetection.diskSpaceThreshold = cJSON_GetNumberValue(diskSpaceThreshold);
    }

    batteryPctCheck = cJSON_GetObjectItem(properties, BATTERY_PERCENTAGE_CHECK);
    if (batteryPctCheck != NULL) {
        g_securityDetection.batteryPercentageCheck = cJSON_GetNumberValue(batteryPctCheck);
    }

    batteryPctThreshold = cJSON_GetObjectItem(properties, BATTERY_PERCENTAGE_THRESHOLD);
    if (batteryPctThreshold != NULL) {
        g_securityDetection.batteryPercentageThreshold = cJSON_GetNumberValue(batteryPctThreshold);
    }

    Detect_ReportDetectionInfo();
    Detect_ReportShadowDesired(g_securityDetection);

    cJSON_Delete(properties);
    return;
}