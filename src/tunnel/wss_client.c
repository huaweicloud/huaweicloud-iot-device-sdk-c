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

#include <pthread.h>
#include <nopoll.h>
#include "securec.h"
#include "log_util.h"
#include "iota_error_type.h"
#include "string_util.h"
#include "json_util.h"
#include "cJSON.h"
#include "iota_datatrans.h"
#include "ssh_client.h"
#include "wss_client.h"

noPollConn *g_Conn = NULL;
noPollCtx *g_Ctx = NULL;
URL_INFO g_UrlInfo = {NULL, NULL, NULL, NULL};

void WssClientDestroyWsClient(void);

void WssClientSaveConfigInfo(const URL_INFO *info)
{
    MemFree(&g_UrlInfo.path);
    MemFree(&g_UrlInfo.port);
    MemFree(&g_UrlInfo.site);
    MemFree(&g_UrlInfo.token);
    CopyStrValue(&g_UrlInfo.path, info->path, strlen(info->path));
    CopyStrValue(&g_UrlInfo.port, info->port, strlen(info->port));
    CopyStrValue(&g_UrlInfo.site, info->site, strlen(info->site));
    CopyStrValue(&g_UrlInfo.token, info->token, strlen(info->token));
}

/**
 * @Description: websocket message listener
 * @param ctx: websocket context
 * @param conn: websocket connection
 * @param msg: message content
 * @param user_data: user define data
 * @return: NULL
 */
void WssClientListenerOnMsg(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data)
{
    const char *content = NULL;
    noPollMsg *aux = NULL;
    noPollMsg *preMsg = NULL;
    JSON *root = NULL;
    char *opType = NULL;
    char *serviceType = NULL;
    char *reqId = NULL;
    pthread_t threadRun;


    // fragment message assemble
    if (nopoll_msg_is_fragment(msg)) {
        PrintfLog(EN_LOG_LEVEL_INFO, "WssClientListenerOnMsg: Found fragment, FIN = %d\n", nopoll_msg_is_final(msg));
        /* call to join this message */
        aux = preMsg;
        preMsg = nopoll_msg_join(preMsg, msg);
        nopoll_msg_unref(aux);
        if (!nopoll_msg_is_final(msg)) {
            return;
        }
        PrintfLog(EN_LOG_LEVEL_INFO, "WssClientListenerOnMsg: Found final fragmen\n");
    } else {
        PrintfLog(EN_LOG_LEVEL_INFO, "WssClientListenerOnMsg: Get Complete Message\n");
        preMsg = nopoll_msg_join(preMsg, msg);
    }
    if (!preMsg) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "WssClientListenerOnMsg: Message alloc failed\n");
        return;
    }

    do {
        content = (const char *)nopoll_msg_get_payload(preMsg);
        root = JSON_Parse(content);
        if (root == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "WssClientListenerOnMsg: Parse Message Failed.\n");
            break;
        }

        serviceType = JSON_GetStringFromObject(root, TUNNEL_SSH_SERTYPE, NULL);
        if (strcmp(serviceType, TUNNEL_SSH_SERTYPE_SSH) != 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "WssClientListenerOnMsg: Wrong tunnel_service_type, Please check.\n");
            JSON_Delete(root);
            root = NULL;
            break;
        }
        reqId = JSON_GetStringFromObject(root, TUNNEL_SSH_REQID, NULL);
        opType = JSON_GetStringFromObject(root, TUNNEL_SSH_OPTYPE, NULL);
        if (strcmp(opType, TUNNEL_SSH_OPTYPE_CONN) == 0) {
            WssClientSendRsp(reqId, TUNNEL_SSH_OPTYPE_CONNRSP, NULL, 0);
            if (SSHClientCreate(root) != IOTA_SUCCESS) {
                WssClientSendDisConn(reqId, "400", "WssClientListenerOnMsg: Create SSH channel failed");
            }
        } else if (strcmp(opType, TUNNEL_SSH_OPTYPE_CMD) == 0) {
            SSHClientRunCmd(root);
        }
    } while (0);

    /* release reference */
    nopoll_msg_unref(preMsg);
    preMsg = NULL;
    JSON_Delete(root);
    return;
}

void *WssClientKeepAlive(void *nopollConn)
{
    noPollConn *conn = (noPollConn *)nopollConn;
    while (HW_TRUE) {
        nopoll_sleep(TUNNEL_WSSCLIENT_PING_DELAY);
        int ret = nopoll_conn_socket(conn);
        if (ret == -1 || ret == NOPOLL_INVALID_SOCKET || conn != g_Conn) {
            break;
        }
        nopoll_conn_send_ping(conn);
    }
}

void *WssClientCloseConn(void *nopollConn)
{
    noPollConn *conn = (noPollConn *)nopollConn;
    nopoll_conn_close(conn);
}

void *WssClientWaitThreadFunc(void *nopollCtx)
{
    noPollCtx *ctx = (noPollCtx*)nopollCtx;
    nopoll_loop_wait(ctx, 0);
}

void WssClientListenerOnClose(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data)
{
    int closeStatus = nopoll_conn_get_close_status(conn);
    const char *closeReason = nopoll_conn_get_close_reason(conn);
    int delay = TUNNEL_WSSCLIENT_CONN_RETRY_DELAY;
    int i;

    if (g_Conn && conn != g_Conn) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "closeStatus:%d, closeReason:%s, conn:%p\n", closeStatus, closeReason, conn);
    do {
        // close ssh channel
        SSHClientSessionDestroy();
        // doesn't try to reconnect when it's normal close, auth failure or reestablish connection
        if ((closeStatus == TUNNEL_WSSCLIENT_CLOSE_NORMAL) || (closeStatus == TUNNEL_WSSCLIENT_AUTH_FAILED) ||
            (closeStatus == TUNNEL_WSSCLIENT_REPEAT_CONN)) {
            break;
        }

        // after disconnection, delay a few seconds to reconnection
        for (i = 1; i <= TUNNEL_WSSCLIENT_CONN_RETRY_TIMES; ++i) {
            nopoll_sleep(delay);
            delay += TUNNEL_WSSCLIENT_CONN_RETRY_DELAY;
            if (WssClientConnect(HW_TRUE) != 0) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "WssClientListenerOnClose: Try to Reconnect %d times failed.\n", i);
            } else {
                PrintfLog(EN_LOG_LEVEL_INFO, "WssClientListenerOnClose: Reconnect Succees at %d times.\n", i);
                nopoll_conn_set_on_close(g_Conn, WssClientListenerOnClose, NULL);
                return;
            }
        }
    } while (0);

    g_Conn = NULL;
    nopoll_loop_stop(ctx);
    nopoll_ctx_unref(ctx);
    g_Ctx = NULL;
    PrintfLog(EN_LOG_LEVEL_INFO, "WssClientListenerOnClose: Websocket closed\n");
    return;
}

/**
 * @Description: establish connection of websocket
 * @param isRetry: true represents that the current program is in the reconnection process, otherwise it's false
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
int WssClientConnect(nopoll_bool isRetry)
{
    pthread_t threadPing;
    pthread_t threadClose;
    noPollConnOpts *opts = nopoll_conn_opts_new();

    nopoll_conn_opts_ssl_peer_verify(opts, HW_FALSE);
    nopoll_conn_opts_set_ssl_protocol(opts, NOPOLL_METHOD_TLSV1_2);
    nopoll_conn_opts_set_extra_headers(opts, g_UrlInfo.token);

    // only one connection will be established on one device, the new connection will close the old one
    if (!isRetry && g_Conn) {
        nopoll_conn_set_on_close(g_Conn, NULL, NULL);
        nopoll_conn_close(g_Conn);
        g_Conn = NULL;
        SSHClientSessionDestroy();
    }
    g_Conn = nopoll_conn_tls_new(g_Ctx, opts, g_UrlInfo.site, g_UrlInfo.port, NULL, g_UrlInfo.path, NULL, NULL);
    if (!g_Conn) {
        return IOTA_RESOURCE_NOT_AVAILABLE;
    }
    // if the current program is in the reconnection process,
    // the callback function will not be set in case of the non-reentrant
    if (!isRetry) {
        nopoll_conn_set_on_close(g_Conn, WssClientListenerOnClose, NULL);
    }
    /* check connection, connection timeout 5 seconds */
    if (!nopoll_conn_wait_until_connection_ready(g_Conn, TUNNEL_WSSCLIENT_CONN_TIMEOUT)) {
        if (!isRetry) {
            pthread_create(&threadClose, NULL, WssClientCloseConn, (void *)g_Conn);
            pthread_detach(threadClose);
        }
        return IOTA_WSS_CONNECT_FAILED;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "WssClientConnect: WssClient connect success.\n");
    nopoll_conn_set_on_msg(g_Conn, WssClientListenerOnMsg, NULL);

    /* websocket keep alive */
    if (pthread_create(&threadPing, NULL, WssClientKeepAlive, (void *)g_Conn) != IOTA_SUCCESS) {
        return IOTA_THREAD_CREATE_FAILED;
    } else {
        pthread_detach(threadPing);
    }
    return IOTA_SUCCESS;
}

/**
 * @Description: try to create websocket
 * @param urlInfo: config data of websocket
 * @return: NULL
 */
void WssClientCreate(const URL_INFO *urlInfo)
{
    noPollConnOpts *opts = NULL;
    pthread_t threadWait;
    int ret = IOTA_SUCCESS;
    int i;
    int delay = 0;

    do {
        if (!g_Ctx) {
            g_Ctx = nopoll_ctx_new();
            if (!g_Ctx || nopoll_ctx_conns(g_Ctx) != 0) {
                ret = IOTA_RESOURCE_NOT_AVAILABLE;
                break;
            }
            /* create context loop */
            ret = pthread_create(&threadWait, NULL, WssClientWaitThreadFunc, g_Ctx);
            if (ret != IOTA_SUCCESS) {
                ret = IOTA_THREAD_CREATE_FAILED;
                break;
            }
            pthread_detach(threadWait);
        }

        /* save the config data to help reconnect */
        WssClientSaveConfigInfo(urlInfo);
        for (i = 1; i <= TUNNEL_WSSCLIENT_CONN_RETRY_TIMES; ++i) {
            nopoll_sleep(delay);
            ret = WssClientConnect(HW_FALSE);
            if (ret == IOTA_SUCCESS) {
                break;
            }
            delay += TUNNEL_WSSCLIENT_CONN_RETRY_DELAY;
        }
        if (ret != IOTA_SUCCESS) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "WssClientCreate: Websocket connect failed, ErrorNO:%d!\n", ret);
        }
        return;
    } while (0);

    PrintfLog(EN_LOG_LEVEL_ERROR, "WssClientCreate: Websocket created failed, and would not retry, errorNO:%d\n", ret);
    WssClientDestroyWsClient();
}

/**
 * @Description: clear the config data of websocket
 * @param NULL
 * @return: void
 */
void WssClientDestroyWsClient(void)
{
    /* close the connection */
    if (g_Conn != NULL) {
        nopoll_conn_close(g_Conn);
        g_Conn = NULL;
    }

    /* release ctx */
    if (g_Ctx != NULL) {
        nopoll_loop_stop(g_Ctx);
        nopoll_ctx_unref(g_Ctx);
        g_Ctx = NULL;
    }
}

/**
 * @Description: parse the url and token to get url_info; if return IOTA_SUCCESS,
 *     caller must memfree info outside, otherwise the memory frees inside.
 * @param info: config data of websocket
 * @param url: websocket url.
 * @param token: websocket token.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
int WssClientSplitUrl(URL_INFO *info, const char* url, const char* token)
{
    char *protocol = "wss://";
    char *header = "\r\ntunnel_access_token:";
    const char *siteHead = url;
    char *protocolHead = NULL;
    char *portHead = NULL;
    char *pathHead = NULL;
    int pathLen;
    int siteLen;
    int portLen;
    int tokenLen = strlen(header) + strlen(token) + 1;
    int ret = 0;

    if (!siteHead) return IOTA_PARAMETER_ERROR;
    protocolHead = strstr(siteHead, protocol);
    if (!protocolHead) return IOTA_PARAMETER_ERROR;
    siteHead += strlen(protocol);

    pathHead = strchr(siteHead, '/');
    if (!pathHead) {
        return IOTA_PARAMETER_ERROR;
    }
    do {
        pathLen = strlen(pathHead);
        if (CopyStrValue(&info->path, pathHead, pathLen) != IOTA_SUCCESS) {
            break;
        }
        portHead = strchr(siteHead, ':');
        if (portHead == NULL) {
            siteLen = pathHead - siteHead;
            portHead = TUNNEL_WSSCLIENT_DEFAULT_PORT;
            portLen = TUNNEL_WSSCLIENT_DEFAULT_PORT_LEN;
        } else {
            siteLen = portHead - siteHead;
            portHead++;
            portLen = pathHead - portHead;
        }

        if (CopyStrValue(&info->site, siteHead, siteLen) != IOTA_SUCCESS ||
            CopyStrValue(&info->port, portHead, portLen) != IOTA_SUCCESS) {
            break;
        }

        info->token = (char *)malloc(tokenLen);
        if (!info->token) {
            info->token = NULL;
            break;
        }
        if (snprintf_s(info->token, tokenLen, tokenLen - 1, "%s%s", header, token) == IOTA_FAILURE) {
            break;
        }
        return IOTA_SUCCESS;
    } while (0);

    MemFree(&info->path);
    MemFree(&info->port);
    MemFree(&info->site);
    MemFree(&info->token);
    return IOTA_FAILURE;
}

/**
 * @Description: send message via websocket
 * @param reqId: request id
 * @param opType:  operation type.
 * @param buff:  message content.
 * @param len:  message length.
 * @return: NULL
 */
void WssClientSendRsp(const char *reqId, const char *opType, char *buff, int len)
{
    cJSON *json2Send = NULL;
    int bytes;
    char *rspMsg = NULL;

    if (!g_Conn) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "WssClientSendRsp: WssClient connection is null, could't send message!\n");
        return;
    }
    json2Send = cJSON_CreateObject();
    cJSON_AddStringToObject(json2Send, TUNNEL_SSH_OPTYPE, opType);
    cJSON_AddStringToObject(json2Send, TUNNEL_SSH_SERTYPE, TUNNEL_SSH_SERTYPE_SSH);
    cJSON_AddStringToObject(json2Send, TUNNEL_SSH_REQID, reqId);
    cJSON_AddStringToObject(json2Send, TUNNEL_SSH_DATA, buff);
    rspMsg = cJSON_Print(json2Send);
    cJSON_Delete(json2Send);
    if (!rspMsg) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "WssClientSendRsp: Malloc failed\n");
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "WssClientSendRsp: len:%d\n", (int)strlen(rspMsg));
    bytes = nopoll_conn_send_text(g_Conn, rspMsg, (int)strlen(rspMsg));
    PrintfLog(EN_LOG_LEVEL_INFO, "WssClientSendRsp: bytes:%d\n", bytes);
    if (bytes != strlen(rspMsg)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "WssClientSendRsp: WssClient send message failed!\n");
    }
    MemFree(&rspMsg);
    return;
}

/**
 * @Description: send disconnect message
 * @param reqId: request id
 * @param code:  error code.
 * @param msg:  error message.
 * @return: NULL
 */
void WssClientSendDisConn(const char *reqId, const char *code, const char *msg)
{
    cJSON *data = cJSON_CreateObject();
    cJSON *root = cJSON_CreateObject();
    char* rspMsg = NULL;

    if (!data || !root) {
        cJSON_Delete(data);
        cJSON_Delete(root);
        return;
    }
    cJSON_AddStringToObject(data, TUNNEL_SSH_STATUS_CODE, code);
    cJSON_AddStringToObject(data, TUNNEL_SSH_STATUS_MSG, msg);
    cJSON_AddStringToObject(root, TUNNEL_SSH_OPTYPE, TUNNEL_SSH_OPTYPE_DISCONN);
    cJSON_AddStringToObject(root, TUNNEL_SSH_SERTYPE, TUNNEL_SSH_SERTYPE_SSH);
    cJSON_AddStringToObject(root, TUNNEL_SSH_REQID, reqId);
    cJSON_AddItemToObject(root, TUNNEL_SSH_DATA, data);
    rspMsg = cJSON_Print(root);
    cJSON_Delete(root);
    if (!rspMsg) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "WssClientSendDisConn: Malloc failed\n");
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "WssClientSendDisConn: Send DisConnect message\n");
    nopoll_conn_send_text(g_Conn, rspMsg, (int)strlen(rspMsg));
    MemFree(&rspMsg);
}