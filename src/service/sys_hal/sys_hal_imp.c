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
#include "sys_hal.h"
#include "log_util.h"

static long GetMemoryUsed(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "### please implement GetMemoryUsed by yourself ###\n");
    return -1;
}

static long GetTotalMemory(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "### please implement GetTotalMemory by yourself ###\n");
    return -1;
}

static ArrayInfo *GetPortUsed(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "### please implement GetPortUsed by yourself ###\n");
    return NULL;
}

static int GetCpuUsage(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "### please implement GetCpuUsage by yourself ###\n");
    return -1;
}

static long GetTotalDiskSpace(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "### please implement GetTotalDiskSpace by yourself ###\n");
    return -1;
}

static long GetDiskSpaceUsed(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "### please implement GetDiskSpaceUsed by yourself ###\n");
    return -1;
}

static int GetBatteryPercentage(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "### please implement GetBatteryPercentage by yourself ###\n");
    return -1;
}

static const TagSysHalOps g_sysHalOps = {
    .getMemoryUsed = GetMemoryUsed,
    .getTotalMemory = GetTotalMemory,
    .getPortUsed = GetPortUsed,
    .getCpuUsage = GetCpuUsage,
    .getTotalDiskSpace = GetTotalDiskSpace,
    .getDiskSpaceUsed = GetDiskSpaceUsed,
    .getBatteryPercentage = GetBatteryPercentage,
};

static const TagSysHal g_sysHal = {
    .name = "SysHal",
    .ops = &g_sysHalOps,
};

int SysHalImplInit(void)
{
    int ret = -1;
    ret = SysHalInstall(&g_sysHal);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "the return value of SysHalImplit is %d\n", ret);
    return ret;
}