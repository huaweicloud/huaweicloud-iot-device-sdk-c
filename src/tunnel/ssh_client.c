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
#include <pthread.h>
#include "libssh/libssh.h"
#include "securec.h"
#include "string_util.h"
#include "json_util.h"
#include "log_util.h"
#include "iota_error_type.h"
#include "callbacks.h"
#include "iota_datatrans.h"
#include "wss_client.h"
#include "ssh_client.h"

ssh_session g_SSHSession = NULL;
ssh_channel g_SSHChannel = NULL;
static char g_reqId[TUNNEL_SSH_REQID_LENGTH] = {0};
static pthread_mutex_t g_channelLocker = PTHREAD_MUTEX_INITIALIZER;
void SSHClientOnChannelClose(ssh_session session, ssh_channel channel, void *userdata);
struct ssh_channel_callbacks_struct g_callBackFunc = {.channel_close_function = SSHClientOnChannelClose};

void SSHClientSessionDestroy(void)
{
    pthread_mutex_lock(&g_channelLocker);
    if (g_SSHChannel) {
        ssh_channel_close(g_SSHChannel);
        ssh_channel_send_eof(g_SSHChannel);
        ssh_channel_free(g_SSHChannel);
        g_SSHChannel = NULL;
    }

    if (g_SSHSession) {
        ssh_disconnect(g_SSHSession);
        ssh_free(g_SSHSession);
        g_SSHSession = NULL;
    }
    pthread_mutex_unlock(&g_channelLocker);
}

int SSHClientVerifyKnownhost(ssh_session session)
{
    enum ssh_known_hosts_e state;
    unsigned char *hash = NULL;
    ssh_key srv_pubkey = NULL;
    size_t hlen;
    char *hexa;
    int ret;
 
    ret = ssh_get_server_publickey(session, &srv_pubkey);
    if (ret < 0) {
        return IOTA_FAILURE;
    }
 
    ret = ssh_get_publickey_hash(srv_pubkey, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);
    ssh_key_free(srv_pubkey);
    if (ret < 0) {
        return IOTA_FAILURE;
    }
 
    state = ssh_session_is_known_server(session);
    switch (state) {
        case SSH_KNOWN_HOSTS_OK:
            break;
        case SSH_KNOWN_HOSTS_CHANGED:
            PrintfLog(EN_LOG_LEVEL_INFO,
                "SSHClientVerifyKnownhost: Host key for server changed: it is now:\n");
            ssh_print_hexa("Public key hash", hash, hlen);
            PrintfLog(EN_LOG_LEVEL_INFO,
                "SSHClientVerifyKnownhost: For security reasons, connection will be stopped\n");
            ssh_clean_pubkey_hash(&hash);
            return IOTA_FAILURE;
        case SSH_KNOWN_HOSTS_OTHER:
            PrintfLog(EN_LOG_LEVEL_INFO,
                "SSHClientVerifyKnownhost: The host key for this server was not found but "
                "another type of key exists.\n");
            PrintfLog(EN_LOG_LEVEL_INFO,
                "SSHClientVerifyKnownhost: An attacker might change the default server key to confuse your client "
                "into thinking the key does not exist\n");
            ssh_clean_pubkey_hash(&hash);
            return IOTA_FAILURE;
        case SSH_KNOWN_HOSTS_NOT_FOUND:
            PrintfLog(EN_LOG_LEVEL_INFO, "Could not find known host file, the file will be automatically created.\n");
            /* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */
        case SSH_KNOWN_HOSTS_UNKNOWN:
            hexa = ssh_get_hexa(hash, hlen);
            ssh_string_free_char(hexa);
            ssh_clean_pubkey_hash(&hash);
            ret = ssh_session_update_known_hosts(session);
            if (ret < 0) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "Error %d\n", ret);
                return IOTA_FAILURE;
            }
            break;
        case SSH_KNOWN_HOSTS_ERROR:
            PrintfLog(EN_LOG_LEVEL_ERROR, "Error %s", ssh_get_error(session));
            ssh_clean_pubkey_hash(&hash);
            return IOTA_FAILURE;
        default:
            break;
    }
    ssh_clean_pubkey_hash(&hash);
    return 0;
}

void SSHClientOnChannelClose(ssh_session session, ssh_channel channel, void *userdata)
{
    WssClientSendDisConn("SSH_timeout_disconnect", "408", "SSH idles for too long, the session will be closed");
    SSHClientSessionDestroy();
}

void SSHClientRead(void)
{
    int nbytes = 0;
    char *rspBuffer = (char *)malloc(TUNNEL_SSH_RSPBUFF_LEN);
    if (!rspBuffer) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientRead: Malloc failed!\n");
        return;
    }
    (void)memset_s(rspBuffer, TUNNEL_SSH_RSPBUFF_LEN, 0, TUNNEL_SSH_RSPBUFF_LEN);
    do {
        pthread_mutex_lock(&g_channelLocker);
        nbytes = ssh_channel_read_nonblocking(g_SSHChannel, rspBuffer, TUNNEL_SSH_RSPBUFF_LEN, 0);
        pthread_mutex_unlock(&g_channelLocker);
        if (nbytes > 0) {
            PrintfLog(EN_LOG_LEVEL_INFO, "SSHClientRead: nbytes:%d\n", nbytes);
            WssClientSendRsp(g_reqId, TUNNEL_SSH_OPTYPE_CMDRSP, rspBuffer, nbytes);
            (void)memset_s(rspBuffer, TUNNEL_SSH_RSPBUFF_LEN, 0, TUNNEL_SSH_RSPBUFF_LEN);
        }
    } while (nbytes >= 0);
    MemFree(&rspBuffer);
}

/**
 * @Description: create interactive shell session
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
int SSHClientInteractiveShellSession(void)
{
    int ret;
    int nbytes = 0;
    pthread_t threadRead;
    
    g_SSHChannel = ssh_channel_new(g_SSHSession);
    if (g_SSHChannel == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientInteractiveShellSession: SSH channel created failed: %s\n",
            ssh_get_error(g_SSHSession));
        return IOTA_FAILURE;
    }

    ret = ssh_channel_open_session(g_SSHChannel);
    if (ret != SSH_OK) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientInteractiveShellSession: SSH channel open session failed: %s\n",
            ssh_get_error(g_SSHSession));
        return ret;
    }

    ret = ssh_channel_request_pty(g_SSHChannel);
    if (ret != SSH_OK) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientInteractiveShellSession: SSH channel request pty failed: %s\n",
            ssh_get_error(g_SSHSession));
        return ret;
    }
 
    ret = ssh_channel_change_pty_size(g_SSHChannel, TUNNEL_TERMINAL_WIDTH, TUNNEL_TERMINAL_HEIGHT);
    if (ret != SSH_OK) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientInteractiveShellSession: SSH channel change pty size failed: %s\n",
            ssh_get_error(g_SSHSession));
        return ret;
    }

    ret = ssh_channel_request_shell(g_SSHChannel);
    if (ret != SSH_OK) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientInteractiveShellSession: SSH channel request shell failed: %s\n",
            ssh_get_error(g_SSHSession));
        return ret;
    }

    ssh_callbacks_init(&g_callBackFunc);
    ssh_set_channel_callbacks(g_SSHChannel, &g_callBackFunc);
    pthread_create(&threadRead, NULL, SSHClientRead, NULL);
    pthread_detach(threadRead);
    return IOTA_SUCCESS;
}

int SSHClientSessionInit(const char *userName, const char *passWord)
{
    int verbosity = SSH_LOG_NOLOG;
    int port = 22;
    int ret;
    
    g_SSHSession = ssh_new();
    if (g_SSHSession == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientSessionInit: SSH session create Failed\n");
        return IOTA_FAILURE;
    }
    ssh_options_set(g_SSHSession, SSH_OPTIONS_HOST, "localhost");
    ssh_options_set(g_SSHSession, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ssh_options_set(g_SSHSession, SSH_OPTIONS_PORT, &port);
    ssh_options_set(g_SSHSession, SSH_OPTIONS_USER, userName);

    do {
        ret = ssh_connect(g_SSHSession);
        if (ret != SSH_OK) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientSessionInit: Connecting to localhost failed: %s\n",
                ssh_get_error(g_SSHSession));
            break;
        }

        ret = SSHClientVerifyKnownhost(g_SSHSession);
        if (ret < 0) {
            break;
        }

        ret = ssh_userauth_password(g_SSHSession, NULL, passWord);
        if (ret != SSH_AUTH_SUCCESS) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientSessionInit: Authenticating with password failed: %s\n",
                ssh_get_error(g_SSHSession));
            break;
        }

        ret = SSHClientInteractiveShellSession();
    } while (0);
    
    if (ret != 0) {
        SSHClientSessionDestroy();
    }
    return ret;
}

int SSHClientCreate(const JSON *root)
{
    JSON *data = JSON_GetObjectFromObject(root, TUNNEL_SSH_DATA);
    char *reqId = JSON_GetStringFromObject(root, TUNNEL_SSH_REQID, NULL);
    char *userName = NULL;
    char *passWord = NULL;

    if (!data || !reqId) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientCreate: Tunnel ssh data parse error!\n");
        return IOTA_PARSE_JSON_FAILED;
    }
    if (memcpy_s(g_reqId, TUNNEL_SSH_REQID_LENGTH, reqId, TUNNEL_SSH_REQID_LENGTH - 1) != EOK) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientCreate: Memcpy failed!\n");
        return IOTA_FAILURE;
    }
    // only one session can be built
    SSHClientSessionDestroy();

    userName = JSON_GetStringFromObject(data, TUNNEL_SSH_DATA_USERNAME, NULL);
    passWord = JSON_GetStringFromObject(data, TUNNEL_SSH_DATA_PASSWORD, NULL);
    if (!userName || !passWord) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientCreate: Tunnel ssh userName or passWord parse error!\n");
        return IOTA_PARSE_JSON_FAILED;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "UserName:%s\n", userName);
    int ret = SSHClientSessionInit(userName, passWord);
    (void)memset_s(passWord, strlen(passWord), 0, strlen(passWord));
    return ret;
}

void SSHClientRunCmd(JSON *root)
{
    int ret;
    int nbytes = 0;
    char *cmd = JSON_GetStringFromObject(root, TUNNEL_SSH_DATA, NULL);
    char *reqId = JSON_GetStringFromObject(root, TUNNEL_SSH_REQID, NULL);

    if (!cmd || !reqId) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientRunCmd: Tunnel ssh command parse error!\n");
        return;
    }
    if (memcpy_s(g_reqId, TUNNEL_SSH_REQID_LENGTH, reqId, TUNNEL_SSH_REQID_LENGTH - 1) != EOK) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientRunCmd: Memcpy failed!\n");
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "SSHClientRunCmd: Command:%10s\n", cmd);


    if (!g_SSHSession) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientRunCmd: SSH may not init yet, please check!\n");
        return;
    }

    ret = ssh_channel_write(g_SSHChannel, cmd, strlen(cmd));
    if (ret != strlen(cmd)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "SSHClientRunCmd: SSH channel command execute failed: %s\n", ssh_get_error(g_SSHSession));
        return;
    }
    return;
}