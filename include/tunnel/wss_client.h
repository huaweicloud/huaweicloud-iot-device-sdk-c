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

#ifndef WSS_CLIENT_H
#define WSS_CLIENT_H

#include <nopoll.h>

#define TUNNEL_WSSCLIENT_CLOSE_NORMAL           1000
#define TUNNEL_WSSCLIENT_AUTH_FAILED            1002
#define TUNNEL_WSSCLIENT_REPEAT_CONN            4000

#define TUNNEL_WSSCLIENT_CONN_RETRY_TIMES       10
#define TUNNEL_WSSCLIENT_CONN_RETRY_DELAY       1000000
#define TUNNEL_WSSCLIENT_PING_DELAY             20000000
#define TUNNEL_WSSCLIENT_CONN_TIMEOUT           10

#define TUNNEL_WSSCLIENT_RSPMSG_LEN             2304

#define TUNNEL_WSSCLIENT_DEFAULT_PORT           "443"
#define TUNNEL_WSSCLIENT_DEFAULT_PORT_LEN       3

typedef struct {
    char *site;
    char *port;
    char *path;
    char *token;
} URL_INFO;

int WssClientSplitUrl(URL_INFO *info, const char *url, const char *token);
void WssClientCreate(const URL_INFO *info);
void WssClientSendRsp(const char *reqId, const char *optype, char *buff, int len);
void WssClientSendDisConn(const char *reqId, const char *code, const char *msg);
#endif