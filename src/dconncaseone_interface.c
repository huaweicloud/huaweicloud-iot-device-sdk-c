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

#include <stdio.h>
#include <stdlib.h>
#include "dconncaseone_interface.h"


/**
 * @brief 初始化套件。
 *
 * @param type 初始化类型: INIT_SERVICE/INIT_CLIENT
 * @param ipAry IP地址字符串。初始化服务端时仅可传入一个 IP 地址，初始化客户端时可传入多个，以半角逗号分隔。
 * @param errorIp 第一个出错的 IP 地址，只有在返回值不为 OK 时有效。内存空间由调用方申请。
 * @return uint32_t 错误码。RESLUT_ENUM。在初始化客户端时，按输入顺序依次初始化，遇到第一个失败的 IP 地址则返回，
 *         后续的 IP 地址不会被处理。
 */
DCONN_API_PUBLIC uint32_t InitDConnCaseOne(uint32_t type, const char *ipAry, char *errorIp) {};

/**
 * @brief 注册回调函数
 *
 * @param callback 回调函数结构体 CallbackParam
 */
DCONN_API_PUBLIC void RegisterCallback(const CallbackParam *callback) {};

/**
 * @brief 发送数据
 *
 * @param targetDeviceId 目标设备 ID
 * @param data 待发送数据
 * @param dataLen 数据字节长度
 * @return int 发送结果。此处并不能代表发送成功，请通过回调函数获取结果。
 */
DCONN_API_PUBLIC uint32_t DConnSendData(const char *targetDeviceId, const char *data, uint32_t dataLen) {};

/**
 * @brief 取消注册回调函数
 *
 */
DCONN_API_PUBLIC void UnRegisterCallback(void) {};

/**
 * @brief 卸载套件。
 *
 */
DCONN_API_PUBLIC void CloseDConnCaseOne(void) {};

/**
 * @brief 获取套件版本号
 *
 * @return const* 字符串格式的版本号
 */
DCONN_API_PUBLIC const char *GetDConnVersion(void) {};