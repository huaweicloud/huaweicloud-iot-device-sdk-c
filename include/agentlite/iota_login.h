/*
 * Copyright (c) 2020-2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#ifndef IOTA_LOGIN_H
#define IOTA_LOGIN_H

HW_API_FUNC HW_INT IOTA_Connect(void);
HW_API_FUNC HW_INT IOTA_DisConnect(void);
HW_API_FUNC HW_INT IOTA_IsConnected(void);

/**
 * ----------------------------deprecated below-------------------------------------->
 */
#define IOTA_TOPIC_CONNECTED_NTY    "IOTA_TOPIC_CONNECTED_NTY"
#define IOTA_TOPIC_DISCONNECT_NTY   "IOTA_TOPIC_DISCONNECT_NTY"

/** Indicates notification parameters */
typedef enum {
    EN_IOTA_LGN_IE_REASON = 0,
} EN_IOTA_LGN_IE_TYPE;

typedef enum enum_IOTA_LOGIN_REASON_TYPE {
    EN_IOTA_LGN_REASON_NULL = 0,                // EN_ULGN_REASON_NULL,
    EN_IOTA_LGN_REASON_CONNCET_ERR = 1,     // 连接失败 = EN_ULGN_REASON_CONNCET_ERR,
    EN_IOTA_LGN_REASON_SERVER_BUSY = 2,     // 服务器忙 = EN_ULGN_REASON_SERVER_BUSY,
    EN_IOTA_LGN_REASON_AUTH_FAILED = 3,     // 鉴权失败 = EN_ULGN_REASON_AUTH_FAILED,
    EN_IOTA_LGN_REASON_NET_UNAVAILABLE = 5, // 网络不可用 = EN_ULGN_REASON_NET_UNAVAILABLE,
    EN_IOTA_LGN_REASON_DEVICE_NOEXIST = 12, // 设备不存在 = EN_ULGN_REASON_UNREG_USER,
    EN_IOTA_LGN_REASON_DEVICE_RMVED = 13,   // 设备已删除 = EN_ULGN_REASON_RMVED_USER,
    EN_IOTA_LGN_REASON_UNKNOWN = 255
} EN_IOTA_LGN_REASON_TYPE;

#endif

