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

#ifndef __SOFT_BUS_DATA_PROCESS
#define __SOFT_BUS_DATA_PROCESS

/**
 * 获取本地网络的IPv4地址
 *
 * @param ifname 网络接口名称
 * @param ipv4 用于存储IPv4地址的字符数组
 * @param ipv4Len 数组长度
 * @return 成功返回0，失败返回-1
 */
int GetLocalNetworkIpv4(char *ifname, char *ipv4, int ipv4Len);

/**
 * 判断设备是主设备还是从设备（默认下发的第一个设备为主设备、其他设备为从设备）
 * 可按照自己的业务情况修改。
 * @param softBusInfoNumber 软总线索引
 * @return -1表示错误，0表示设置成功
 */
int SoftBusInit(int softBusInfoNumber);

/**
 * 发送数据：发送数据给软总线中的所有其他设备
 *
 * @param softBusInfoNumber 软总线索引
 * @param data 要发送的数据
 * @param dataLen 要发送的数据长度
 * @return 0表示发送校验成功，此处并不能代表发送成功，请通过回调函数获取结果。
 */
int SendDataToAllInfoDevice(int softBusInfoNumber, char *data, int dataLen);

/**
 * 发送数据：发送给某个设备
 *
 * @param device_id 其他设备的设备ID
 * @param data 要发送的数据
 * @param dataLen 要发送的数据长度
 * @return 0表示发送校验成功，此处并不能代表发送成功，请通过回调函数获取结果。
 */
int SendDataToDevice(char *device_id, char *data, int dataLen);


/**
 * 获取密钥：依据主设备获取密钥
 *
 * @param target_device_id 目的设备id
 * @return 返回目的设备设备密钥，若是非记录中的主设备或查询失败，返回NULL。
 */
char *GetSlaveAuthKey(const char *target_device_id);

/**
 * 断开
 */
void CleanAllSoftBus(void);
#endif
