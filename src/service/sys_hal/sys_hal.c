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

#include <stdio.h>
#include "log_util.h"
#include "sys_hal.h"

static const TagSysHal *g_sysHalCb = NULL;

int SysHalInstall(const TagSysHal *sysHal)
{
    int ret = -1;
    if (g_sysHalCb == NULL) {
        g_sysHalCb = sysHal;
        ret = 0;
    }

    return ret;
}

long SysHalGetTotalMemory(void)
{
    long totalMemory = 0;
    if ((g_sysHalCb != NULL) && (g_sysHalCb->ops != NULL) && (g_sysHalCb->ops->getTotalMemory != NULL)) {
        totalMemory = g_sysHalCb->ops->getTotalMemory();
    }
    return totalMemory;
}

long SysHalGetMemoryUsed(void)
{
    long memoryUsed = 0;
    if ((g_sysHalCb != NULL) && (g_sysHalCb->ops != NULL) && (g_sysHalCb->ops->getMemoryUsed != NULL)) {
        memoryUsed = g_sysHalCb->ops->getMemoryUsed();
    }
    return memoryUsed;
}

ArrayInfo *SysHalGetPortUsed(void)
{
    ArrayInfo *arrayInfo = NULL;
    if ((g_sysHalCb != NULL) && (g_sysHalCb->ops != NULL) && (g_sysHalCb->ops->getPortUsed != NULL)) {
        arrayInfo = g_sysHalCb->ops->getPortUsed();
    }
    return arrayInfo;
}

int SysHalGetCpuUsage(void)
{
    int cpuUsage = 0;
    if ((g_sysHalCb != NULL) && (g_sysHalCb->ops != NULL) && (g_sysHalCb->ops->getCpuUsage != NULL)) {
        cpuUsage = g_sysHalCb->ops->getCpuUsage();
    }
    return cpuUsage;
}

long SysHalGetTotalDiskSpace(void)
{
    long totalDiskSpace = 0;
    if ((g_sysHalCb != NULL) && (g_sysHalCb->ops != NULL) && (g_sysHalCb->ops->getTotalDiskSpace != NULL)) {
        totalDiskSpace = g_sysHalCb->ops->getTotalDiskSpace();
    }
    return totalDiskSpace;
}

long SysHalGetDiskSpaceUsed(void)
{
    long diskSpaceUsed = 0;
    if ((g_sysHalCb != NULL) && (g_sysHalCb->ops != NULL) && (g_sysHalCb->ops->getDiskSpaceUsed != NULL)) {
        diskSpaceUsed = g_sysHalCb->ops->getDiskSpaceUsed();
    }
    return diskSpaceUsed;
}

int SysHalGetBatteryPercentage(void)
{
    int batteryPct = 0;
    if ((g_sysHalCb != NULL) && (g_sysHalCb->ops != NULL) && (g_sysHalCb->ops->getBatteryPercentage != NULL)) {
        batteryPct = g_sysHalCb->ops->getBatteryPercentage();
    }
    return batteryPct;
}

__attribute__((weak)) int SysHalImplInit(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "###please implement SysHalImplnit by yourself###\n\r");
    return -1;
}

int SysHalInit(void)
{
    return SysHalImplInit();
}