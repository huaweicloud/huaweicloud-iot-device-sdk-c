/*
 * Copyright (c) 2022-2024 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#include <math.h>
#include "callback_func.h"
#include "log_util.h"
#include "iota_datatrans.h"
#include "iota_defaultCallback.h"
#include "string_util.h"

#ifdef SSH_SWITCH
#include "wss_client.h"
#endif

static void TimeSleep(int ms)
{
#if defined(WIN32) || defined(WIN64)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

// Connection failure callback function
static void IOTA_HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    static long retryTimes = 0;
    PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_defaultCallback: IOTA_HandleConnectFailure() error, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);

    /* Set up a disconnection and reconnection mechanism */
    int lowBound = (int) (IOTA_DEFAULE_BACKOFF * 0.8);
    int highBound = (int) (IOTA_DEFAULE_BACKOFF * 1.0);
    unsigned long randomBackOff = rand() % (highBound - lowBound) + lowBound;
    int power = retryTimes >= 32 ? 31 : retryTimes;
    unsigned long powParameter = (unsigned long) (pow(2.0, (double) power));
    long backOffWithJitter = powParameter * (randomBackOff + lowBound);
    long waitTimeUntilNextRetry = IOTA_MIN_BACKOFF + backOffWithJitter > IOTA_MAX_BACKOFF ? IOTA_MAX_BACKOFF : IOTA_MIN_BACKOFF + backOffWithJitter;
    
    TimeSleep(waitTimeUntilNextRetry);
    retryTimes++;
    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_defaultCallback: HandleAuthFailure() error, login again failed, result %d\n", ret);
    }
}

static void IOTA_HandleDisConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    (void)rsp;
    PrintfLog(EN_LOG_LEVEL_INFO, "IOTA_defaultCallback: IOTA_HandleDisConnectSuccess(), disConnect success\n");
}

static void IOTA_HandleDisConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "IOTA_defaultCallback: HandleDisConnectFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

static void IOTA_HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    (void)rsp;
    PrintfLog(EN_LOG_LEVEL_INFO, "IOTA_defaultCallback: IOTA_HandleConnectSuccess(), login success\n");
}

void IOTA_HandleSubscribeFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "IOTA_HandleSubscribeFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

void IOTA_HandleReportFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_HandleReportFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

// ------------------------ OTA --------------------------------
static void ReportOTAVersion(EN_IOTA_EVENT *message)
{
    ST_IOTA_OTA_VERSION_INFO otaVersion;

    otaVersion.event_time = NULL;
    otaVersion.sw_version = "v1.0";
    otaVersion.fw_version = "v1.0";
    otaVersion.object_device_id = message->object_device_id;

    int messageId = IOTA_OTAVersionReport(otaVersion, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_defaultCallback: ReportOTAVersion() failed, messageId %d\n", messageId);
    }
}

static void ReportUpgradeStatus(int i, char *version, char *object_device_id)
{
    ST_IOTA_UPGRADE_STATUS_INFO statusInfo;
    if (i == 0) {
        statusInfo.description = "success";
        statusInfo.progress = 100;
        statusInfo.result_code = 0;
        statusInfo.version = version;
    } else {
        statusInfo.description = "failed";
        statusInfo.result_code = 1;
        statusInfo.progress = 0;
        statusInfo.version = version;
    }

    statusInfo.event_time = NULL;
    statusInfo.object_device_id = object_device_id;

    int messageId = IOTA_OTAStatusReport(statusInfo, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_defaultCallback: ReportUpgradeStatus() failed, messageId %d\n", messageId);
    }
}

static void HandleEvenOtaVersion(char *objectDeviceId)
{
    
    // report OTA version
    // When the platform issues a version upgrade notification, it returns firmware version information
    EN_IOTA_EVENT event;
    event.object_device_id = objectDeviceId;
    ReportOTAVersion(&event);
}
    
static void HandleEvenOtaUrlResponse(char *objectDeviceId, int event_type, EN_IOTA_OTA_PARAS *ota_paras)
{
    char filename[PKGNAME_MAX + 1];
    // start to receive packages and firmware_upgrade or software_upgrade
    // 把文件包存储在 ./${filename}中
    if (IOTA_GetOTAPackages_Ext(ota_paras->url, ota_paras->access_token,
            1000, ".", filename) == 0) {

        usleep(3000 * 1000);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the filename is %s\n", filename);
        // report successful upgrade status
        ReportUpgradeStatus(0, ota_paras->version, objectDeviceId);
    } else {
        // report failed status
        ReportUpgradeStatus(-1, ota_paras->version, objectDeviceId);
    }
}

static void HandleEventFile(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message)
{
    if (message == NULL) {
        return;
    }
    if (message->event_type == EN_IOTA_EVENT_GET_UPLOAD_URL_RESPONSE) {
        IOTA_UploadFile(NULL, message->file_mgr_paras->url, NULL);
    } else {
        IOTA_DownloadFile(NULL, message->file_mgr_paras->url, NULL);
    }
}

// ------------ Remote login ---------------
#ifdef SSH_SWITCH
static void HandleTunnelMgr(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message)
{
    URL_INFO info = {NULL, NULL, NULL, NULL};
    if (message->event_type != EN_IOTA_EVENT_TUNNEL_NOTIFY)
        return;

    int ret = WssClientSplitUrl(&info, message->tunnel_mgr_paras->tunnel_url, 
        message->tunnel_mgr_paras->tunnel_access_token);
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "HandleTunnelMgr: Url parse failed.%d\n", ret);
        return;
    }
    WssClientCreate(&info);
    MemFree(&info.path);
    MemFree(&info.port);
    MemFree(&info.site);
    MemFree(&info.token);
}
#endif

HW_API_FUNC HW_VOID IOTA_DefaultCallbackInit() 
{
    // Connection initialization
    SetProtocolCallback(EN_CALLBACK_CONNECT_SUCCESS, IOTA_HandleConnectSuccess);
    SetProtocolCallback(EN_CALLBACK_CONNECT_FAILURE, IOTA_HandleConnectFailure);
    SetProtocolCallback(EN_CALLBACK_CONNECTION_LOST, IOTA_HandleConnectFailure);
    SetProtocolCallback(EN_CALLBACK_DISCONNECT_SUCCESS, IOTA_HandleDisConnectSuccess);
    SetProtocolCallback(EN_CALLBACK_DISCONNECT_FAILURE, IOTA_HandleDisConnectFailure);
    SetProtocolCallback(EN_CALLBACK_SUBSCRIBE_FAILURE, IOTA_HandleSubscribeFailure);
    SetProtocolCallback(EN_CALLBACK_PUBLISH_FAILURE, IOTA_HandleReportFailure);
    
    // Event initialization    
    IOTA_SetEvenOtaVersionUpCallback(HandleEvenOtaVersion);
    IOTA_SetEvenOtaUrlResponseCallback(HandleEvenOtaUrlResponse);
    IOTA_SetEvenFileManagerCallback(HandleEventFile);
   
#ifdef SSH_SWITCH
    IOTA_SetEvenTunnelManagerCallback(HandleTunnelMgr);
#endif
}
