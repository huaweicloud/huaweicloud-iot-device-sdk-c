/*
 * Copyright (c) 2024-2025 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "log_util.h"

#include "soft_bus_datatrans.h"
#include "soft_bus_data_process.h"
#include "dconncaseone_interface.h"

static int g_softBusType = -1;

// 获取ip地址
/**
 * get IPv4 address and subnet mask of a network interface
 */
int GetLocalNetworkIpv4(char *ifname, char *ipv4, int ipv4Len)
{
    int rc = -1;
    struct sockaddr_in *addr = NULL;

    struct ifreq ifr;
    memset_s(&ifr, sizeof(struct ifreq), 0, sizeof(struct ifreq));

     /* 0. create a socket */
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd == -1) {
        return -1;
    }

    /* 1. set type of address to retrieve : IPv4 */
    ifr.ifr_addr.sa_family = AF_INET;

    /* 2. copy interface name to ifreq structure */
    if (strncpy_s(ifr.ifr_name, IFNAMSIZ - 1, ifname, IFNAMSIZ - 1) != 0) {
        goto done;
    }

    /* 3. get the IP address */
    if ((rc = ioctl(fd, SIOCGIFADDR, &ifr)) != 0) {
        goto done;
    }  

    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    if (strncpy_s(ipv4, ipv4Len, inet_ntoa(addr->sin_addr), 16) != 0) {
        goto done;
    }

    /* 4. get the mask */
    if ((rc = ioctl(fd, SIOCGIFNETMASK, &ifr)) != 0) {
        goto done;
    }

    /* 5. display */
    PrintfLog(EN_LOG_LEVEL_INFO, "IFNAME:IPv4:MASK = %s:%s\n", ifname, ipv4);

    /* 6. close the socket */
done:
    close(fd);
    return rc;
}

// -1未连接 0连接成功
// 主从设备设置
int SoftBusInit(int softBusInfoNumber)
{
 
    int ret = -1;
    char errorIp[64] = {0};
    soft_bus_total *g_soft_bus_total = getSoftBusTotal();
    soft_bus_info *g_device_soft_bus_info =
            &g_soft_bus_total->g_soft_bus_info[softBusInfoNumber].g_device_soft_bus_info;
    int total = g_soft_bus_total->count;

    if (softBusInfoNumber >= total || softBusInfoNumber < 0) {
        return -1;
    }

    if (strcmp(g_device_soft_bus_info[0].device_id, getDeviceId()) == 0) {
        ret = InitServerKit(g_device_soft_bus_info[0].device_ip, errorIp);
        g_softBusType = INIT_SERVICE;
    } else {
        ret = InitClientKit(g_device_soft_bus_info[0].device_ip, errorIp);
        g_softBusType = INIT_CLIENT;
    }
    
    PrintfLog(EN_LOG_LEVEL_INFO, "SoftBusInit %s, busId = %s, device_ip = %s\n", 
            g_softBusType == INIT_SERVICE ? "Server" : "Client",
            g_soft_bus_total->g_soft_bus_info[softBusInfoNumber].bus_id, g_device_soft_bus_info[0].device_ip);

    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR,"soft bus Init err ret = %d, errorIp = %s\n", ret, errorIp);
        return -1;
    }
    return 0;
}


// 发送数据 本设备发送数据给某个软总线中的所有设备
int SendDataToAllInfoDevice(int softBusInfoNumber, char *data, int dataLen)
{
    if (data == NULL || dataLen <= 0) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "SendDataToAllDevice data is NULL");
        return -1;
    }
    //验证是否主设备
    soft_bus_total *g_soft_bus_total = getSoftBusTotal();
    soft_bus_info *g_device_soft_bus_info = 
            &g_soft_bus_total->g_soft_bus_info[softBusInfoNumber].g_device_soft_bus_info;
    int total = g_soft_bus_total->count;
    if (softBusInfoNumber >= total || softBusInfoNumber < 0 || g_device_soft_bus_info[0].device_id == NULL) {
        return -1;
    }

    if (strcmp(g_device_soft_bus_info[0].device_id, getDeviceId()) != 0) {
        return -1;
    }
    
    int j = 0;
    uint32_t ret = 0;
    int softBusDeviceCount = g_soft_bus_total->g_soft_bus_info[softBusInfoNumber].count;
    for (j = 0; j < softBusDeviceCount; j++) {
        char *deviceId = g_device_soft_bus_info[j].device_id;
        if (deviceId == NULL || strcmp(deviceId, getDeviceId()) == 0) {
            continue;
        }
        
        ret = DConnSendData(deviceId, data, dataLen);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "to device %s Send %u data is: %s, resulf: %d\n", deviceId, dataLen, data, ret);
    }
    return ret;
}

// 发送数据 发送给某个设备
int SendDataToDevice(char *device_id, char *data, int dataLen)
{
    if (data == NULL || dataLen <= 0) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "SendDataToDevice data is NULL");
        return -1;
    }
    
    int ret = DConnSendData(device_id, data, dataLen);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "to device %s Send %u data is: %s, resulf: %d\n", device_id, dataLen, data, ret);
    return ret;
}


// 获取设备认证的PIN码数组, target_device_id为目标id, 默认以第一个设备为主设备
char *GetSlaveAuthKey(const char *target_device_id)
{
    soft_bus_total *g_soft_bus_total = getSoftBusTotal();
    if (target_device_id == NULL || g_soft_bus_total == NULL) {
        return NULL;
    }
    
    int i = 0;
    for (i = 0; i < g_soft_bus_total->count; i++) {
        char *slaveDeviceId = g_soft_bus_total->g_soft_bus_info[i].g_device_soft_bus_info[0].device_id;
        if (strcmp(slaveDeviceId, target_device_id) != 0) {
            continue;
        }
        PrintfLog(EN_LOG_LEVEL_DEBUG, "getAuthKey slaveDeviceId = %s\n", target_device_id);

        return g_soft_bus_total->g_soft_bus_info[i].bus_key;
    }
    return NULL;
}

void CleanAllSoftBus(void)
{
    DestroyKit();
}