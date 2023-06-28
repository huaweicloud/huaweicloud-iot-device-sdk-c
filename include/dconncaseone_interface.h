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

/**
 * @copyright Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * @brief 分布式互联套件1 对外接口
 * @version 0.1
 * @date 2022-06-21
 */

#ifndef DCONNCASEONE_INTERFACE_H
#define DCONNCASEONE_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#if defined(__LINUX__) || defined(_UNIX)
#define DCONN_API_PUBLIC __attribute__((visibility("default")))
#else
#define DCONN_API_PUBLIC
#endif

#define INIT_SERVICE (0)  // 初始化服务端
#define INIT_CLIENT (1)   // 初始化客户端

#define DCONN_DEVICE_ID_LEN 128  // 最大设备ID字符串长度
#define DCONN_IP_ADDR_LEN 46  // 最大IP地址字符串长度
#define DCONN_AUTH_KEY_LEN 16  // 最大 Auth Key 长度
#define DCONN_DATA_BODY_LEN 2018  // 最大报文长度

enum {
    // Common 0x00000000 ~ 0x00000FFF
    DC_SUCCESS = 0x00000000,
    DC_ERROR = 0x00000001,
    DC_ERR_BRANCH = 0x00000002,
    DC_ERR_ALLOC_MEMORY = 0x00000003,
    DC_ERR_CREATE_THREAD = 0x00000004,
    DC_ERR_THRESHOLD_EXCEED = 0x00000005,
    DC_ERR_NULLPTR = 0x00004006,
    DC_ERR_INVALID_JSON = 0x00004007,

    // Callback 0x00001000 ~ 0x00001FFF
    DC_ERR_INVALID_DEVICE_ID = 0x00001001,
    DC_ERR_CB_NOT_REGISTER = 0x00001002,
    DC_ERR_INVALID_AUTH_KEY = 0x00001003,

    // mbedtls 0x00002000 ~ 0x00002FFF
    DC_ERR_MBEDTLS_SETKEY = 0x00002001,
    DC_ERR_MBEDTLS_ENCRYPT = 0x00002002,
    DC_ERR_MBEDTLS_SEED = 0x00002003,
    DC_ERR_MBEDTLS_GEN_RANDOM = 0x00002004,
    DC_ERR_MBEDTLS_DECRYPT = 0x00002005,

    // Encode/Decode message 0x00003000 ~ 0x00003FFF
    DC_ERR_HEADER_INVALID = 0x00003001,

    // Network 0x00004000 ~ 0x00004fff
    DC_ERR_CONNECTION_REFUSED = 0x00004001, // 网络无法连接
    DC_ERR_BIND_PORT = 0x00004002,  // 端口被占用
    DC_ERR_SEND_FAILED = 0x00004003,

    // Parameter 0x00005000 ~ 0x00005fff
    DC_ERR_IP_FORMAT = 0x00005001,
    DC_ERR_DEVICE_ID_NOT_FOUND = 0x00005002,
    DC_ERR_DEVICE_NOT_AUTHORIZED = 0x00005003,
    DC_ERR_INVALID_INIT_TYPE = 0x00005004,

    // Limit 0x00006000 ~ 0x00006fff
    DC_ERR_PEER_NUM_EXCEED = 0x00006001,
    DC_ERR_SEND_DATA_EXCEED = 0x00006002,

    // Auth 0x00007000 ~ 0x00007fff
    DC_ERR_AUTH_FAILED = 0x00007001,
    DC_ERR_AUTH_INIT_FAILED = 0x00007002,
    DC_ERR_GET_GROUP_MANAGER = 0x00007003,
    DC_ERR_INVALID_AUTH_STAT = 0x00007004,
    DC_ERR_PROCESS_AUTH_DATA = 0x00007005,
    DC_ERR_CREATE_GROUP = 0x00007006,
    DC_ERR_REG_HICHAIN_CB = 0x00007007,
    DC_ERR_ADD_MEMBER = 0x00007008,
    DC_ERR_INVALID_REQUEST_ID = 0x00007009,
    DC_ERR_HICHAIN_AUTH_MSG_EXCEED = 0x0000700a,
};

/**
 * @brief 数据发送回调函数。
 *
 */
typedef void (*SendDataCallback)(const char *deviceId, uint32_t result);

/**
 * @brief 数据接收回调函数。
 *
 */
typedef void (*ReceiveDataCallback)(const char *deviceId, const char *receiveData, uint32_t datelen);

/**
 * @brief 校验当前连接的服务的客户端IP地址是否合法
 *
 */
typedef bool (*IsValidIP)(const char *ip);

/**
 * @brief 校验当前连接服务的客户端的设备ID是否合法
 *
 */
typedef bool (*IsValidDeviceID)(const char *deviceID);

/**
 * @brief 获取对应设备的 PIN 码
 *
 */
typedef char *(*GetAuthKey)(const char *targetDeviceID);

/**
 * @brief 获取本设备 ID
 *
 */
typedef char *(*GetDeviceID)(void);

/**
 * @brief 回调函数结构体
 *
 */
typedef struct {
    /** Callback for send data */
    SendDataCallback sendDataResultCb;
    /** Callback for recevie message */
    ReceiveDataCallback onReceiveDataCb;

    // 依赖IOT SDK提供的方法
    IsValidIP isValidIP;
    IsValidDeviceID isValidDeviceID;
    GetAuthKey getAuthKey;
    GetDeviceID getDeviceID;
} CallbackParam;

/**
 * @brief 初始化套件。
 *
 * @param type 初始化类型: INIT_SERVICE/INIT_CLIENT
 * @param ipAry IP地址字符串。初始化服务端时仅可传入一个 IP 地址，初始化客户端时可传入多个，以半角逗号分隔。
 * @param errorIp 第一个出错的 IP 地址，只有在返回值不为 OK 时有效。内存空间由调用方申请。
 * @return uint32_t 错误码。RESLUT_ENUM。在初始化客户端时，按输入顺序依次初始化，遇到第一个失败的 IP 地址则返回，
 *         后续的 IP 地址不会被处理。
 */
DCONN_API_PUBLIC uint32_t InitDConnCaseOne(uint32_t type, const char *ipAry, char *errorIp);

/**
 * @brief 注册回调函数
 *
 * @param callback 回调函数结构体 CallbackParam
 */
DCONN_API_PUBLIC void RegisterCallback(const CallbackParam *callback);

/**
 * @brief 发送数据
 *
 * @param targetDeviceId 目标设备 ID
 * @param data 待发送数据
 * @param dataLen 数据字节长度
 * @return int 发送结果。此处并不能代表发送成功，请通过回调函数获取结果。
 */
DCONN_API_PUBLIC uint32_t DConnSendData(const char *targetDeviceId, const char *data, uint32_t dataLen);

/**
 * @brief 取消注册回调函数
 *
 */
DCONN_API_PUBLIC void UnRegisterCallback(void);

/**
 * @brief 卸载套件。
 *
 */
DCONN_API_PUBLIC void CloseDConnCaseOne(void);

/**
 * @brief 获取套件版本号
 *
 * @return const* 字符串格式的版本号
 */
DCONN_API_PUBLIC const char *GetDConnVersion(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* DCONNCASEONE_INTERFACE_H */
