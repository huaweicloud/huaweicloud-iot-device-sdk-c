/*
 * Copyright (c) 2022-2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#ifndef DETECT_ANOMALY_H
#define DETECT_ANOMALY_H

#include "cJSON.h"
#include "sys_hal.h"
#include "iota_init.h"

typedef struct {
    int memoryThreshold;
    int cpuUsageThreshold;
    int diskSpaceThreshold;
    int batteryPercentageThreshold;
} SecurityDetection;

typedef struct {
    long used;     // the amount of memory used
    long total;    // the total memory size
    int leakAlarm; // the memory leak switch, 0:no, 1:yes
} MemoryInfo;

typedef struct {
    long diskSpaceUsed;
    long diskSpaceTotal;
    int diskSpaceAlarm;
} DiskSpaceInfo;

typedef struct {
    int cpuUsage;
    int cpuAlarm;
} CpuUsageInfo;

typedef struct {
    int batteryPct;    // the value of batteryPercentage
    int batteryAlarm;  // 1 means alarm, 0 means no alarm
} BatteryInfo;

/**
 * @brief get device shadow for "$security_detection_config"
 *
 * @return int 0:success, otherwise failed
 */
int Detect_GetShadowDetectAnomaly(void);

/**
 * @brief deal with the properties from shadow or propertiesSet
 *
 * @param propertieDown the properties from shadow or propertiesSet
 */
void Detect_ParseShadowGetOrPropertiesSet(char *propertieDown);


#endif /* DETECT_ANOMALY_H */