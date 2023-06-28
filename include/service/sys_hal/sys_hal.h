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

#ifndef SYS_HAL_H
#define SYS_HAL_H

#define SIZE_1KB 0x400
#define SIZE_1MB 0x100000

#define B_TO_KB(size) (((size) + (SIZE_1KB >> 1)) / SIZE_1KB)
#define B_TO_MB(size) (((size) + (SIZE_1MB >> 1)) / SIZE_1MB)

typedef struct {
    int *array;
    int arrayLen;
} ArrayInfo;

typedef struct {
    // get information of memory
    long (*getTotalMemory)(void);
    long (*getMemoryUsed)(void);

    // get information of port
    ArrayInfo *(*getPortUsed)(void);
    int (*getCpuUsage)(void);
    long (*getTotalDiskSpace)(void);
    long (*getDiskSpaceUsed)(void);
    int (*getBatteryPercentage)(void);
} TagSysHalOps;

typedef struct {
    const char *name;        // the name of sysHal
    const TagSysHalOps *ops; // the hal API of board
} TagSysHal;

/**
 * @brief install the sys hal to the SDK
 *
 * @param sysHal the information of syshal
 * @return int
 */
int SysHalInstall(const TagSysHal *sysHal);

/**
 * @brief get total memory of the board
 *
 * @return long
 */
long SysHalGetTotalMemory(void);

/**
 * @brief get the memory used
 *
 * @return long the size of memory used
 */
long SysHalGetMemoryUsed(void);

/**
 * @brief Get the port list
 *
 * @return ArrayInfo* the information of port list
 */
ArrayInfo *SysHalGetPortUsed(void);

/**
 * @brief Get the CPU Usage
 *
 * @return int the value of CpuUsage
 */
int SysHalGetCpuUsage(void);

/**
 * @brief Get the amount of total diskspace
 *
 * @return long the value of total diskspace
 */
long SysHalGetTotalDiskSpace(void);

/**
 * @brief Get the amount of used diskspace
 *
 * @return long the value of used diskspace
 */
long SysHalGetDiskSpaceUsed(void);

/**
 * @brief Get the amount of batteryPercentage
 *
 * @return int the value of batteryPercentage
 */
int SysHalGetBatteryPercentage(void);

/**
 * @brief this function initialize the sys hal
 *
 * @return int 0 means succssful, -1 means failed
 */
int SysHalInit(void);

/**
 * @brief this function need to be implemented by the user
 *
 * @return int 0 means succssful, -1 means failed
 */
int SysHalImplInit(void);
#endif /* end SYS_HAL_H */