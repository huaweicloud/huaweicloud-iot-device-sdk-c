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
#include <string.h>
#include "login.h"
#include "bridge_topic_data.h"
#include "string_util.h"
#include "hmac_sha256.h"
#include "securec.h"
#include "iota_error_type.h"
#include "iota_bridge.h"
#include "cJSON.h"
#include "iota_cfg.h"
#include "log_util.h"
#include "hw_type.h"
#include "iota_payload.h"

unsigned int gBridgeRequestId = 1;

static char *getBridgeRequestId(char *requestId, int requestIdLen)
{
    (void)snprintf_s(requestId, requestIdLen, requestIdLen - 1, "%u", gBridgeRequestId);
    gBridgeRequestId++;
    if (gBridgeRequestId > 65535) gBridgeRequestId = 0;
    return requestId;
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceLogin(HW_CHAR *deviceId, HW_CHAR *password, HW_CHAR *requestId, void *context)
{
    if (deviceId == NULL || password == NULL) {
        PrintfLog(
            EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDeviceLogin(), the deviceId or password cannot be null.\n");
    }
    
    char presetRequestId[6] = {0};
    if (requestId == NULL) {
        requestId = getBridgeRequestId(presetRequestId, sizeof(presetRequestId));
    }
    char *timestamp = GetClientTimesStamp();
    char *tempEncryptedPwd = NULL;
    StringMalloc(&tempEncryptedPwd, 65);
    int ret = EncryptWithHMac(password, &timestamp, 32, tempEncryptedPwd, 0);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, IOTA_SIGN_TYPE, 0);
    cJSON_AddStringToObject(root, IOTA_TIMESTAMP, timestamp);
    cJSON_AddStringToObject(root, IOTA_PASSWORD, tempEncryptedPwd);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    MemFree(&tempEncryptedPwd);
    MemFree(&timestamp);
    if (payload == NULL) {
        return IOTA_FAILURE;
    }
    int messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_LOGIN, deviceId, requestId, context, NULL);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: Bridge_ReportDeviceData() with payload ==> %s\n", payload);
    MemFree(&payload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceLogout(HW_CHAR *deviceId, HW_CHAR *requestId, void *context)
{
    char *payload = "";
    char presetRequestId[6] = {0};
    if (requestId == NULL) {
        requestId = getBridgeRequestId(presetRequestId, sizeof(presetRequestId));
    }
    return Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_LOGOUT, deviceId, requestId, context, NULL);
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceResetSecret(HW_CHAR *deviceId, HW_CHAR *oldSecret, HW_CHAR *newSecret, HW_CHAR *requestId, void *context)
{
    if (deviceId == NULL || oldSecret == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "iota_bridge: IOTA_BridgeDeviceResetSecret(), the deviceId or oldSecret cannot be null.\n");
        return IOTA_FAILURE;
    }
    char presetRequestId[6] = {0};
    if (requestId == NULL) {
        requestId = getBridgeRequestId(presetRequestId, sizeof(presetRequestId));
    }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, IOTA_OLD_SECRET, oldSecret);
    cJSON_AddStringToObject(root, IOTA_NEW_SECRET, newSecret);
    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    if (payload == NULL) {
        return IOTA_FAILURE;
    }

    int messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_RESET_SECRET, deviceId, requestId, context, NULL);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceResetSecret() with payload ==> %s\n", payload);
    MemFree(&payload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceMessagesReport(HW_CHAR *deviceId, HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content, void *context)
{
    if (deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDeviceMessagesReport(), the deviceId cannot be null.\n");
        return IOTA_FAILURE;
    }

    char *payload = IOTA_MessageReportPayload(object_device_id, name, id, content);
    if (payload == NULL) {
        return IOTA_FAILURE;
    }
    int messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_MESSAGES_UP, deviceId, NULL, context, NULL);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceMessagesReport() with payload %s ==>\n", payload);
    MemFree(&payload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_BridgeDevicePropertiesReport(HW_CHAR *deviceId, ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum, void *context)
{
    if (deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDevicePropertiesReport(), the deviceId cannot be null.\n");
        return IOTA_FAILURE;
    }
    char *payload = IOTA_PropertiesReportPayload(pServiceData, serviceNum);
    if (payload == NULL) {
        return IOTA_FAILURE;
    }
    int messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_PROPERTIES, deviceId, NULL, context, NULL);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceMessagesReport() with payload %s ==>\n", payload);
    MemFree(&payload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceBatchPropertiesReport(HW_CHAR *deviceId, ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum, HW_INT serviceLenList[], void *context)
{
    if (deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDeviceBatchPropertiesReport(), the deviceId cannot be null.\n");
        return IOTA_FAILURE;
    }

    char *payload = IOTA_BatchPropertiesReportPayload(pServiceData, serviceNum, serviceLenList);
    if (payload == NULL) {
        return IOTA_FAILURE;
    }
    int messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_GATEWAY_PROPERTIES, deviceId, NULL, context, NULL);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceBatchPropertiesReport() with payload %s ==>\n", payload);
    MemFree(&payload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_BridgeDevicePropertiesSetResponse(HW_CHAR *deviceId, HW_CHAR *requestId, HW_INT result_code, HW_CHAR *result_desc, void *context)
{
    if (requestId == NULL || deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDevicePropertiesSetResponse:the requestId or deviceId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    char *payload = IOTA_PropertiesSetResponsePayload(result_code, result_desc);
    if (payload == NULL) {
        return IOTA_FAILURE;
    }
    int messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_SET_PROPERTIES, deviceId, requestId, context, NULL);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDevicePropertiesSetResponse() with payload ==> %s\n", payload);
    MemFree(&payload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_BridgeDevicePropertiesGetResponse(HW_CHAR *deviceId, HW_CHAR *requestId, ST_IOTA_SERVICE_DATA_INFO serviceProp[],
    HW_INT serviceNum, void *context)
{
    if (serviceNum <= 0) return IOTA_FAILURE;
    if (requestId == NULL || deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDevicePropertiesGetResponse:the requestId or deviceId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    char *payload = IOTA_PropertiesGetResponsePayload(serviceProp, serviceNum);
    if (payload == NULL) {
        return IOTA_FAILURE;
    }
    int messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_GET_PROPERTIES, deviceId, requestId, context, NULL);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDevicePropertiesGetResponse() with payload ==> %s\n", payload);
    MemFree(&payload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceGetDeviceShadow(HW_CHAR *deviceId, HW_CHAR *requestId, HW_CHAR *object_device_id, HW_CHAR *service_id, void *context)
{
    if (deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDeviceGetDeviceShadow:the deviceId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char presetRequestId[6] = {0};
    if (requestId == NULL) {
        requestId = getBridgeRequestId(presetRequestId, sizeof(presetRequestId));
    }
    char *payload = IOTA_GetDeviceShadowPayload(object_device_id, service_id);
    if (payload == NULL) {
        return IOTA_FAILURE;
    }
    int messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_GET_SHADOW, deviceId, requestId, context, NULL);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceGetDeviceShadow() with payload ==> %s\n", payload);
    MemFree(&payload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceCommandResponse(HW_CHAR *deviceId, HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name, HW_CHAR *pcCommandResponse, void *context)
{
    if (deviceId == NULL || requestId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDeviceCommandResponse:the deviceId or requestId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char *payload = IOTA_CommandResponsePayload(result_code, response_name, pcCommandResponse);
    if (payload == NULL) {
        return IOTA_FAILURE;
    }
    int messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_COMMANDS_RESPONSE, deviceId, requestId, context, NULL);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceCommandResponse() with payload ==> %s\n", payload);
    MemFree(&payload);
    return messageId;
}

// --------- 事件相关 -----------
HW_API_FUNC HW_INT IOTA_BridgeDeviceUpdateSubDeviceStatus(HW_CHAR *deviceId, ST_IOTA_DEVICE_STATUSES *device_statuses, HW_INT deviceNum, void *context)
{
    if (deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDeviceUpdateSubDeviceStatus:the deviceId or requestId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char *payload = IOTA_UpdateSubDeviceStatusPayload(device_statuses, deviceNum);
    if (payload == NULL) {
        return IOTA_FAILURE;
    }
    int messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_EVENTS_UP, deviceId, NULL, context, NULL);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceUpdateSubDeviceStatus() with payload ==> %s\n", payload);
    MemFree(&payload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceAddSubDevice(HW_CHAR *deviceId, ST_IOTA_SUB_DEVICE_INFO *subDevicesInfo, HW_INT deviceNum, void *context)
{
    if (deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDeviceAddSubDevice:the deviceId or requestId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char *payload = IOTA_AddSubDevicePayload(subDevicesInfo, deviceNum);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_EVENTS_UP, deviceId, NULL, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceAddSubDevice() with payload ==> %s\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceDelSubDevice(HW_CHAR *deviceId, ST_IOTA_DEL_SUB_DEVICE *delSubDevices, HW_INT deviceNum, void *context)
{
    if (deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDeviceDelSubDevice:the deviceId or requestId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char *payload = IOTA_DelSubDevicePayload(delSubDevices, deviceNum);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_EVENTS_UP, deviceId, NULL, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceDelSubDevice() with payload ==> %s\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

// 文件上传下载
HW_API_FUNC HW_INT IOTA_BridgeDeviceGetUploadFileUrl(HW_CHAR *deviceId, const ST_IOTA_UPLOAD_FILE *upload, void *context)
{
    if (deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDeviceGetUploadFileUrl:the deviceId or requestId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char *payload = IOTA_GetFileUrlPayload(upload, 1);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_EVENTS_UP, deviceId, NULL, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceGetUploadFileUrl() with payload ==> %s\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceGetDownloadFileUrl(HW_CHAR *deviceId, const ST_IOTA_DOWNLOAD_FILE *download, void *context)
{
    if (deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_bridge: IOTA_BridgeDeviceGetDownloadFileUrl:the deviceId or requestId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char *payload = IOTA_GetFileUrlPayload(download, 0);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_EVENTS_UP, deviceId, NULL, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_bridge: IOTA_BridgeDeviceGetDownloadFileUrl() with payload ==> %s\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceGetNTPTime(HW_CHAR *deviceId, void *context)
{
    char *payload = IOTA_GetNTPTimePayload();
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_EVENTS_UP, deviceId, NULL, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_GetNTPTime() with payload ==> %s\n", payload);
        MemFree(&payload);
        return messageId;
    }
}


HW_API_FUNC HW_INT IOTA_BridgeDeviceReportDeviceInfo(HW_CHAR *deviceId, ST_IOTA_DEVICE_INFO_REPORT *device_info_report, void *context)
{
    char *payload = IOTA_ReportDeviceInfoPayload(device_info_report);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_EVENTS_UP, deviceId, NULL, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_ReportDeviceInfo() with payload ==> %s\n", payload);
        MemFree(&payload);
        return messageId;
    }
}


HW_API_FUNC HW_INT IOTA_BridgeDeviceGetLatestSoftBusInfo(HW_CHAR *deviceId, HW_CHAR *busId, HW_CHAR *eventId, void *context)
{
    char *payload = IOTA_GetLatestSoftBusInfoPayload(busId, eventId);
    int messageId = 0;
    if (payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: get the newest soft bus info failed, the payload is null\n");
        return IOTA_FAILURE;
    } else {
        messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_EVENTS_UP, deviceId, NULL, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_GetLatestSoftBusInfo() with payload ==> %s\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_BridgeDeviceReportDeviceLog(HW_CHAR *deviceId, HW_CHAR *type, HW_CHAR *content, HW_CHAR *timestamp, void *context)
{
    char *payload = IOTA_ReportDeviceLogPayload(type, content, timestamp);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = Bridge_ReportDeviceData(payload, BRIDGE_DEVICE_EVENTS_UP, deviceId, NULL, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_ReportDeviceLog() with payload ==> %s\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

// --------- 连接相关 ----------
int IOTA_BridgeConnect(void)
{
    IOTA_ConfigSetUint(EN_IOTA_CFG_BRIDGE_MODE, EN_IOTA_CFG_BRIDGE_ON);
    return CreateConnection();
}

int IOTA_BridgeIsConnected(void)
{
    return IsConnected();
}

int IOTA_BridgeDisConnect(void)
{
    IOTA_ConfigSetUint(EN_IOTA_CFG_BRIDGE_MODE, EN_IOTA_CFG_BRIDGE_OFF);
    return ReleaseConnection();
}
