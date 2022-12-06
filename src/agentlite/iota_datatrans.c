/*
 * Copyright (c) 2020-2022 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "string.h"
#include "securec.h"
#include "string_util.h"
#include "log_util.h"
#include "base.h"
#include "hw_type.h"
#include "data_trans.h"
#include "iota_datatrans.h"
#include "cJSON.h"
#include "iota_error_type.h"
#include "subscribe.h"
#include "limits.h"
#include "rule_trans.h"

char *IOTA_MessageReportPayload(HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content)
{
    cJSON *root;
    root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, object_device_id);
    cJSON_AddStringToObject(root, NAME, name);
    cJSON_AddStringToObject(root, ID, id);
    cJSON_AddStringToObject(root, CONTENT, content);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);

    return payload;
}

HW_API_FUNC HW_INT IOTA_MessageReport(HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content,
    HW_CHAR *topicParas, HW_INT compressFlag, void *context)
{
    if (content == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the content cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    int messageId = 0;
    char *payload = IOTA_MessageReportPayload(object_device_id, name, id, content);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDeviceData(payload, topicParas, compressFlag, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_MessageReport() with payload %s By topic %s==>\n", payload,
            topicParas);
        MemFree(&payload);
    }
    return messageId;
}

#if defined(MQTTV5)

HW_API_FUNC HW_INT IOTA_MessageReportV5(ST_IOTA_MESS_REP_INFO mass, HW_INT compressFlag, void *context,
    MQTTV5_DATA *mqttv5)
{
    if (mass.content == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the content cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char *payload = IOTA_MessageReportPayload(mass.object_device_id, mass.name, mass.id, mass.content);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDeviceData(payload, mass.topicParas, compressFlag, context, (void *)mqttv5);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_MessageReportV5() with payload %s By topic %s==>\n",
            payload, mass.topicParas);
        MemFree(&payload);
    }
    return messageId;
}
#endif

char *IOTA_PropertiesReportPayload(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum)
{
    if (serviceNum == 0 || pServiceData == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
        return NULL;
    }
    RuleMgr_CachePropertiesValue(pServiceData, serviceNum);
    RuleMgr_CheckAndExecuteNoTimers();
    
    cJSON *root, *serviceDatas;
    root = cJSON_CreateObject();
    serviceDatas = cJSON_CreateArray();
    int i;
    for (i = 0; i < serviceNum; i++) {
        cJSON *properties = cJSON_Parse(pServiceData[i].properties);
        if (properties == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
            cJSON_Delete(serviceDatas);
            cJSON_Delete(root);
            return NULL;
        }

        cJSON *tmp;
        tmp = cJSON_CreateObject();

        cJSON_AddStringToObject(tmp, SERVICE_ID, pServiceData[i].service_id);
        cJSON_AddStringToObject(tmp, EVENT_TIME, pServiceData[i].event_time);
        cJSON_AddItemToObject(tmp, PROPERTIES, properties);
        cJSON_AddItemToArray(serviceDatas, tmp);
    }

    cJSON_AddItemToObject(root, SERVICES, serviceDatas);
    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

HW_API_FUNC HW_INT IOTA_PropertiesReport(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum,
    HW_INT compressFlag, void *context)
{
    char *payload = IOTA_PropertiesReportPayload(pServiceData, serviceNum);
    int messageId = 0;

    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDeviceProperties(payload, compressFlag, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_PropertiesReport() with payload %s ==>\n", payload);
        MemFree(&payload);
    }
    return messageId;
}

#if defined(MQTTV5)

HW_API_FUNC HW_INT IOTA_PropertiesReportV5(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum,
    HW_INT compressFlag, void *context, MQTTV5_DATA *mqttv5)
{
    char *payload = IOTA_PropertiesReportPayload(pServiceData, serviceNum);
    int messageId = 0;

    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDeviceProperties(payload, compressFlag, context, (void *)mqttv5);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_PropertiesReportV5() with payload %s ==>\n", payload);
        MemFree(&payload);
    }
    return messageId;
}
#endif

char *IOTA_BatchPropertiesReportPayload(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum,
    HW_INT serviceLenList[])
{
    if (deviceNum == 0 || serviceLenList == NULL || pDeviceData == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
        return NULL;
    }

    if (deviceNum < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the deviceNum cannot be minus.\n");
        return NULL;
    }

    if (deviceNum > MaxServiceReportNum) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the deviceNum exceeds maximum.\n");
        return NULL;
    }

    cJSON *root, *deviceDatas;
    root = cJSON_CreateObject();
    deviceDatas = cJSON_CreateArray();
    int ii, jj;
    for (ii = 0; ii < deviceNum; ii++) {
        cJSON *deviceData;
        cJSON *services;
        deviceData = cJSON_CreateObject();
        services = cJSON_CreateArray();
        cJSON_AddStringToObject(deviceData, DEVICE_ID, pDeviceData[ii].device_id);

        for (jj = 0; jj < serviceLenList[ii]; jj++) {
            cJSON *properties = cJSON_Parse(pDeviceData[ii].services[jj].properties);
            if (properties == NULL) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
                cJSON_Delete(services);
                cJSON_Delete(deviceData);
                cJSON_Delete(deviceDatas);
                cJSON_Delete(root);
                return IOTA_PARSE_JSON_FAILED;
            }

            cJSON *tmp;
            tmp = cJSON_CreateObject();

            cJSON_AddStringToObject(tmp, SERVICE_ID, pDeviceData[ii].services[jj].service_id);
            cJSON_AddStringToObject(tmp, EVENT_TIME, pDeviceData[ii].services[jj].event_time);
            cJSON_AddItemToObject(tmp, PROPERTIES, properties);
            cJSON_AddItemToArray(services, tmp);
        }

        cJSON_AddItemToObject(deviceData, SERVICES, services);
        cJSON_AddItemToArray(deviceDatas, deviceData);
    }

    cJSON_AddItemToObject(root, DEVICES, deviceDatas);
    char *payload = cJSON_Print(root);

    cJSON_Delete(root);
    return payload;
}

HW_API_FUNC HW_INT IOTA_BatchPropertiesReport(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum,
    HW_INT serviceLenList[], HW_INT compressFlag, void *context)
{
    char *payload = IOTA_BatchPropertiesReportPayload(pDeviceData, deviceNum, serviceLenList);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        int messageId = ReportBatchDeviceProperties(payload, compressFlag, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_BatchPropertiesReport() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}
#if defined(MQTTV5)

HW_API_FUNC HW_INT IOTA_BatchPropertiesReportV5(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum,
    HW_INT serviceLenList[], HW_INT compressFlag, void *context, MQTTV5_DATA *mqttv5)
{
    char *payload = IOTA_BatchPropertiesReportPayload(pDeviceData, deviceNum, serviceLenList);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        int messageId = ReportBatchDeviceProperties(payload, compressFlag, context, (void *)mqttv5);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_BatchPropertiesReport() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}
#endif


char *IOTA_CommandResponsePayload(HW_INT result_code, HW_CHAR *response_name, HW_CHAR *pcCommandResponse)
{
    cJSON *root;
    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, RESULT_CODE, result_code);
    cJSON_AddStringToObject(root, RESPONSE_NAME, response_name);
    cJSON *commandResponse = cJSON_Parse(pcCommandResponse);
    if (commandResponse == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_AddItemToObject(root, PARAS, commandResponse);

    char *payload;
    payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

HW_API_FUNC HW_INT IOTA_CommandResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name,
    HW_CHAR *pcCommandResponse, void *context)
{
    if (pcCommandResponse == NULL || requestId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_CommandResponse:the requestId or commandResponse cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    int messageId = 0;
    char *payload = IOTA_CommandResponsePayload(result_code, response_name, pcCommandResponse);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportCommandReponse(requestId, payload, context, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_CommandResponse() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}
#if defined(MQTTV5)

HW_API_FUNC HW_INT IOTA_CommandResponseV5(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name,
    HW_CHAR *pcCommandResponse, void *context, MQTTV5_DATA *properties)
{
    if (pcCommandResponse == NULL || requestId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_CommandResponse:the requestId or commandResponse cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    int messageId = 0;
    char *payload = IOTA_CommandResponsePayload(result_code, response_name, pcCommandResponse);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportCommandReponse(requestId, payload, context, (void *)properties);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_CommandResponse() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}
#endif

HW_API_FUNC HW_INT IOTA_PropertiesSetResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *result_desc,
    void *context)
{
    if (requestId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_PropertiesSetResponse:the requestId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    cJSON *root;
    root = cJSON_CreateObject();
    char *payload;
    cJSON_AddNumberToObject(root, RESULT_CODE, result_code);

    if (result_desc != 0) {
        cJSON_AddStringToObject(root, RESULT_DESC, result_desc);
    }

    payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportPropSetReponse(requestId, payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_PropertiesSetResponse() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}


HW_API_FUNC HW_INT IOTA_PropertiesGetResponse(HW_CHAR *requestId, ST_IOTA_SERVICE_DATA_INFO serviceProp[],
    HW_INT serviceNum, void *context)
{
    if (serviceNum <= 0 || requestId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_PropertiesGetResponse:the requestId or serviceNum is wrong.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    cJSON *root, *services;
    root = cJSON_CreateObject();
    char *payload;
    services = cJSON_CreateArray();

    int i;

    if (serviceProp[0].service_id != 0 && serviceProp[0].properties != 0) {
        for (i = 0; i < serviceNum; i++) {
            if (serviceProp[i].service_id != 0 && serviceProp[i].properties != 0) {
                cJSON *properties = cJSON_Parse(serviceProp[i].properties);
                if (properties == NULL) {
                    PrintfLog(EN_LOG_LEVEL_ERROR, "parse JSON failed.\n");
                    cJSON_Delete(services);
                    cJSON_Delete(root);
                    return IOTA_PARSE_JSON_FAILED;
                }

                cJSON *tmp;
                tmp = cJSON_CreateObject();

                cJSON_AddStringToObject(tmp, SERVICE_ID, serviceProp[i].service_id);
                cJSON_AddStringToObject(tmp, EVENT_TIME, serviceProp[i].event_time);
                cJSON_AddItemToObject(tmp, PROPERTIES, properties);

                cJSON_AddItemToArray(services, tmp);
            } else {
                PrintfLog(EN_LOG_LEVEL_ERROR, "the payload is wrong.\n");
                cJSON_Delete(services);
                cJSON_Delete(root);
                return IOTA_PARAMETER_ERROR;
            }
        }
    }

    cJSON_AddItemToObject(root, SERVICES, services);
    payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportPropGetReponse(requestId, payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_PropertiesGetResponse() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_UpdateSubDeviceStatus(ST_IOTA_DEVICE_STATUSES *device_statuses, HW_INT deviceNum, void *context)
{
    if ((device_statuses == NULL) || (deviceNum < 0) || (deviceNum > MaxSubDeviceCount)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_UpdateSubDeviceStatus() error, the input is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }

    if (device_statuses->device_statuses[0].device_id == NULL || device_statuses->device_statuses[0].status == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_UpdateSubDeviceStatus() error, the input of "
                                      "device_statuses->device_statuses[0] is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }

    cJSON *root, *services, *device_statuses_json;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    device_statuses_json = cJSON_CreateArray();
    cJSON *service = cJSON_CreateObject();
    cJSON_AddStringToObject(service, SERVICE_ID, SUB_DEVICE_MANAGER);
    cJSON_AddStringToObject(service, EVENT_TYPE, SUB_DEVICE_UPDATE_STATUS);
    cJSON_AddStringToObject(service, EVENT_TIME, device_statuses->event_time);

    int i;
    for (i = 0; i < deviceNum; i++) {
        if (device_statuses->device_statuses[i].device_id != NULL &&
            device_statuses->device_statuses[i].status != NULL) {
            cJSON *tmp;
            tmp = cJSON_CreateObject();

            cJSON_AddStringToObject(tmp, DEVICE_ID, device_statuses->device_statuses[i].device_id);
            cJSON_AddStringToObject(tmp, STATUS, device_statuses->device_statuses[i].status);

            cJSON_AddItemToArray(device_statuses_json, tmp);
        } else {
            PrintfLog(EN_LOG_LEVEL_ERROR, "the payload is wrong.\n");
            cJSON_Delete(device_statuses_json);
            cJSON_Delete(service);
            cJSON_Delete(services);
            cJSON_Delete(root);
            return IOTA_PARAMETER_ERROR;
        }
    }

    cJSON *paras = cJSON_CreateObject();
    cJSON_AddItemToObject(paras, DEVICE_STATUS, device_statuses_json);

    cJSON_AddItemToObject(service, PARAS, paras);


    cJSON_AddItemToArray(services, service);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload;
    payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_UpdateSubDeviceStatus() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_AddSubDevice(ST_IOTA_SUB_DEVICE_INFO *subDevicesInfo, HW_INT deviceNum, void *context)
{
    if ((subDevicesInfo == NULL) || (deviceNum < 0) || (deviceNum > MaxAddedSubDevCount)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_AddSubDevice() error, the input is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }
    if (subDevicesInfo->deviceInfo == NULL || subDevicesInfo->deviceInfo[0].device_id == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "iota_datatrans: IOTA_AddSubDevice() error, the input of subDevicesInfo->deviceInfo is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }

    if (subDevicesInfo->deviceInfo[0].node_id == NULL || subDevicesInfo->deviceInfo[0].product_id == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "iota_datatrans: IOTA_AddSubDevice() error, the input of subDevicesInfo->deviceInfo[0] is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }

    cJSON *root, *services, *subDeviceInfo;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    subDeviceInfo = cJSON_CreateArray();

    cJSON *service = cJSON_CreateObject();
    cJSON_AddStringToObject(service, SERVICE_ID, SUB_DEVICE_MANAGER);
    cJSON_AddStringToObject(service, EVENT_TYPE, ADD_SUB_DEVICE_REQUEST);
    cJSON_AddStringToObject(service, EVENT_TIME, subDevicesInfo->event_time);
    cJSON_AddStringToObject(service, EVENT_ID, subDevicesInfo->event_id);

    int i;
    for (i = 0; i < deviceNum; i++) {
        if (subDevicesInfo->deviceInfo[i].node_id != NULL && subDevicesInfo->deviceInfo[i].product_id != NULL) {
            cJSON *tmp = cJSON_CreateObject();

            cJSON_AddStringToObject(tmp, PARENT_DEVICE_ID, subDevicesInfo->deviceInfo[i].parent_device_id);
            cJSON_AddStringToObject(tmp, NODE_ID, subDevicesInfo->deviceInfo[i].node_id);
            cJSON_AddStringToObject(tmp, DEVICE_ID, subDevicesInfo->deviceInfo[i].device_id);
            cJSON_AddStringToObject(tmp, NAME, subDevicesInfo->deviceInfo[i].name);
            cJSON_AddStringToObject(tmp, DESCRIPTION, subDevicesInfo->deviceInfo[i].description);
            cJSON_AddStringToObject(tmp, PRODUCT_ID, subDevicesInfo->deviceInfo[i].product_id);
            cJSON_AddStringToObject(tmp, EXTENSION_INFO, subDevicesInfo->deviceInfo[i].extension_info);

            cJSON_AddItemToArray(subDeviceInfo, tmp);
        } else {
            PrintfLog(EN_LOG_LEVEL_ERROR, "the payload is wrong.\n");
            cJSON_Delete(subDeviceInfo);
            cJSON_Delete(service);
            cJSON_Delete(services);
            cJSON_Delete(root);
            return IOTA_PARAMETER_ERROR;
        }
    }

    cJSON *paras = cJSON_CreateObject();
    cJSON_AddItemToObject(paras, DEVICES, subDeviceInfo);

    cJSON_AddItemToObject(service, PARAS, paras);

    cJSON_AddItemToArray(services, service);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload;
    payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_AddSubDevice() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_DelSubDevice(ST_IOTA_DEL_SUB_DEVICE *delSubDevices, HW_INT deviceNum, void *context)
{
    if ((delSubDevices == NULL) || (deviceNum < 0) || (deviceNum > MaxDelSubDevCount)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_DelSubDevice() error, the input is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }
    if (delSubDevices->delSubDevice[0] == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "iota_datatrans: IOTA_DelSubDevice() error, the input of delSubDevices->delSubDevice[0] is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }

    cJSON *root, *services, *delSubDevice;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();

    cJSON *service = cJSON_CreateObject();
    cJSON_AddStringToObject(service, SERVICE_ID, SUB_DEVICE_MANAGER);
    cJSON_AddStringToObject(service, EVENT_TYPE, DEL_SUB_DEVICE_REQUEST);
    cJSON_AddStringToObject(service, EVENT_TIME, delSubDevices->event_time);
    cJSON_AddStringToObject(service, EVENT_ID, delSubDevices->event_id);

    delSubDevice = cJSON_CreateStringArray(delSubDevices->delSubDevice, deviceNum);

    cJSON *paras = cJSON_CreateObject();
    cJSON_AddItemToObject(paras, DEVICES, delSubDevice);

    cJSON_AddItemToObject(service, PARAS, paras);

    cJSON_AddItemToArray(services, service);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload;
    payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_DelSubDevice() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_OTAVersionReport(ST_IOTA_OTA_VERSION_INFO otaVersionInfo, void *context)
{
    if (!(otaVersionInfo.fw_version != NULL || otaVersionInfo.sw_version != NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_OTAVersionReport:the input is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }

    cJSON *root, *paras, *services, *tmp;
    root = cJSON_CreateObject();
    tmp = cJSON_CreateObject();
    paras = cJSON_CreateObject();
    services = cJSON_CreateArray();
    char *payload;

    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, otaVersionInfo.object_device_id);
    cJSON_AddStringToObject(tmp, SERVICE_ID, OTA);
    cJSON_AddStringToObject(tmp, EVENT_TIME, otaVersionInfo.event_time);
    cJSON_AddStringToObject(tmp, EVENT_TYPE, VERSION_REPORT);
    cJSON_AddStringToObject(paras, SW_VERSION, otaVersionInfo.sw_version);
    cJSON_AddStringToObject(paras, FW_VERSION, otaVersionInfo.fw_version);
    cJSON_AddItemToObject(tmp, PARAS, paras);

    cJSON_AddItemToArray(services, tmp);

    cJSON_AddItemToObject(root, SERVICES, services);

    payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_OTAVersionReport() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_OTAStatusReport(ST_IOTA_UPGRADE_STATUS_INFO otaStatusInfo, void *context)
{
    if (otaStatusInfo.progress > 100 || otaStatusInfo.progress < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_OTAVersionReport:the progress is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }

    if (!((otaStatusInfo.result_code >= 0 && otaStatusInfo.result_code <= 10) || (otaStatusInfo.result_code == 255))) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_OTAVersionReport:the result_code is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }

    cJSON *root, *paras, *services, *tmp;
    root = cJSON_CreateObject();
    tmp = cJSON_CreateObject();
    paras = cJSON_CreateObject();
    services = cJSON_CreateArray();
    char *payload;

    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, otaStatusInfo.object_device_id);
    cJSON_AddStringToObject(tmp, SERVICE_ID, OTA);
    cJSON_AddStringToObject(tmp, EVENT_TIME, otaStatusInfo.event_time);
    cJSON_AddStringToObject(tmp, EVENT_TYPE, UPGRADE_PROGRESS_REPORT);
    cJSON_AddNumberToObject(paras, RESULT_CODE, otaStatusInfo.result_code);
    cJSON_AddNumberToObject(paras, PROGRESS, otaStatusInfo.progress);
    cJSON_AddStringToObject(paras, VERSION, otaStatusInfo.version);
    cJSON_AddStringToObject(paras, DESCRIPTION, otaStatusInfo.description);

    cJSON_AddItemToObject(tmp, PARAS, paras);

    cJSON_AddItemToArray(services, tmp);

    cJSON_AddItemToObject(root, SERVICES, services);

    payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_OTAStatusReport() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_SubscribeUserTopic(HW_CHAR *topicParas)
{
    return SubscribeUserTopic(topicParas);
}

HW_API_FUNC HW_INT IOTA_SubscribeTopic(HW_CHAR *topic, HW_INT qos)
{
    return SubsribeTopic(topic, qos);
}

char *IOTA_GetUrlData(HW_CHAR *url, uint8_t urlLen, char *ip)
{
    // Split Url data
    char *tmp = strstr(url, DOUBLE_OBLIQUE_LINE);
    if (tmp == NULL) {
        return NULL;
    }

    int len = strlen(tmp);
    char *tmpContainsColon = strstr(tmp, COLON);
    if (tmpContainsColon == NULL) {
        return NULL;
    }
    // the length of ipTmp is enougt to copy
    int ret = strncpy_s(ip, IP_LENGTH, tmp + strlen(DOUBLE_OBLIQUE_LINE),
        len - strlen(tmpContainsColon) - strlen(DOUBLE_OBLIQUE_LINE));
    if (ret != 0) {
        return NULL;
    }

    char *uri = strstr(tmpContainsColon, SINGLE_SLANT);
    return uri;
}

int IOTA_ConnectionDomainName(HW_CHAR *ip, HW_INT ipLen, HW_INT timeout)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_ConnectionDomainName() creat socket error.\n");
        return fd;
    }

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if ((setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval))) != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_ConnectionDomainName() setsockopt error.\n");
        close(fd);
        return IOTA_FAILURE;
    }

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(OTA_PORT);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_ConnectionDomainName() fail to connect server by tcp.\n");
        close(fd);
        return IOTA_FAILURE;
    }
    return fd;
}


SSL *IOTA_OpensslConnect(int fd, SSL_CTX *context)
{
    SSL *ssl = SSL_new(context);
    if (ssl == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_OpensslConnect() new ssl failed.\n");
        return NULL;
    }

    if (SSL_set_fd(ssl, fd) <= 0) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_OpensslConnect() SSL_set_fd fail.\n");
        return NULL;
    }

    if (SSL_connect(ssl) == -1) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_OpensslConnect() ssl connect failed.\n");
        return NULL;
    }
    return ssl;
}

HW_API_FUNC HW_INT IOTA_SendHttpHeader(char *str1, size_t str1len, char *uri, char *ip, char *token)
{
    int retSum = 0;

    retSum = memset_s(str1, str1len, 0, HTTP_HEADER_LENGTH);
    if (retSum != 0) {
        return IOTA_FAILURE;
    }

    strcat_s(str1, str1len, OTA_HTTP_GET);
    retSum = strcat_s(str1, str1len, uri);
    if (retSum != 0) {
        return IOTA_FAILURE;
    }
    strcat_s(str1, str1len, OTA_HTTP_VERSION);

    strcat_s(str1, str1len, OTA_HTTP_HOST);
    retSum = strcat_s(str1, str1len, ip);
    if (retSum != 0) {
        return IOTA_FAILURE;
    }
    strcat_s(str1, str1len, OTA_LINEFEED);

    strcat_s(str1, str1len, OTA_CONTENT_TYPE);
    strcat_s(str1, str1len, OTA_AUTH);
    retSum = strcat_s(str1, str1len, token);
    if (retSum) {
        return IOTA_FAILURE;
    }
    strcat_s(str1, str1len, OTA_CRLF);
    return 1;
}

HW_API_FUNC HW_INT IOTA_IotReadHeaderFlag(SSL *ssl, char *buf, int read_length, char *fileName, long *fileSize)
{
    // the length of rspStatusCode is enougt to copy
    char rspStatusCode[HTTP_STATUS_LENGTH + 1] = { "" };
    int result = 0;
    int ret =
        strncpy_s(rspStatusCode, sizeof(rspStatusCode), buf + strlen(OTA_HTTP_RESPONSE_VERSION), HTTP_STATUS_LENGTH);
    if (ret != 0) {
        return IOTA_FAILURE;
    }

    rspStatusCode[HTTP_STATUS_LENGTH] = '\0';
    if (strcmp(rspStatusCode, HTTP_OK)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() error: the statusCode is %s.\n",
            rspStatusCode);
        result = IOTA_FAILURE;
    }

    // get packageSize  from  Content-Length in the reponse Header
    int content_Length_index = GetSubStrIndex((const char *)buf, OTA_CONTENT_LENGTH);
    if (content_Length_index < IOTA_SUCCESS) {
        return IOTA_FAILURE;
    }
    int p = 0, k;
    char pkgSize[PKG_LENGTH];
    for (k = content_Length_index + strlen(OTA_CONTENT_LENGTH); k < read_length - 1; k++) {
        if (buf[k] == '\r' || buf[k] == '\n' || buf[k] == ';') {
            break;
        } else {
            pkgSize[p++] = buf[k];
        }
    }
    pkgSize[p] = '\0';

    char *end;
    *fileSize = strtol(pkgSize, &end, 10);
    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() the fileSize is %d.\n", *fileSize);

    // get filename  from  Content-Disposition in the reponse Header
    int filename_index = GetSubStrIndex((const char *)buf, FILE_NAME);
    p = 0;
    if (filename_index < IOTA_SUCCESS) {
        return IOTA_FAILURE;
    }
    for (k = filename_index + strlen(FILE_NAME); k < read_length - 1; k++) {
        if (buf[k] == '\r' || buf[k] == '\n' || buf[k] == ';') {
            break;
        } else {
            fileName[p++] = buf[k];
        }
    }
    fileName[p] = '\0';
    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() the fileName is %s \n", fileName);
    return result;
}

HW_API_FUNC HW_INT IOTA_IotReadHeaderData(char *buf, int read_length, long fileSize, long *write_size, FILE *fp)
{
    int write_length = 0;
    int bodyStart = GetSubStrIndex(buf, OTA_CRLF);
    if (bodyStart < IOTA_SUCCESS) {
        return IOTA_PARAMETER_EMPTY;
    }
    if ((*write_size + read_length) < fileSize) {
        write_length =
            fwrite(buf + bodyStart + strlen(OTA_CRLF), sizeof(char), read_length - bodyStart - strlen(OTA_CRLF), fp);
        if (write_length < read_length - bodyStart - (int)strlen(OTA_CRLF)) {
            return IOTA_FAILURE;
        }
        *write_size += write_length;
        return 1;
    } else if (((*write_size + read_length) >= fileSize) && (*write_size < fileSize)) {
        write_length = fwrite(buf + bodyStart + strlen(OTA_CRLF), sizeof(char),
            fileSize - *write_size - bodyStart - strlen(OTA_CRLF), fp);
        if (write_length < fileSize - *write_size - bodyStart - (int)strlen(OTA_CRLF)) {
            return IOTA_FAILURE;
        }
        *write_size += write_length;
        return IOTA_SUCCESS;
    } else {
        return IOTA_SUCCESS;
    }
    return 1;
}

HW_API_FUNC HW_INT IOTA_IotRead(SSL *ssl)
{
    int read_length = 0;
    int write_length = 0;
    long write_size = 0L;
    long fileSize = 0L;
    int ret = 0;
    int result = 0;
    char fileName[PKGNAME_MAX] = {0};

    int headerFlag = 0; // to judge if read the response header
    FILE *fp = NULL;

    char buf[BUFSIZE];
    do {
        memset_s(buf, sizeof(buf), 0, sizeof(buf));
        read_length = SSL_read(ssl, buf, sizeof(buf) - 1);

        if (headerFlag == 0) {
            headerFlag = 1;
            result = IOTA_IotReadHeaderFlag(ssl, buf, read_length, fileName, &fileSize);
            if (strlen(fileName) > PATH_MAX || NULL == fileName || result < 0) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_IotReadHeaderFlag() return = %d.\n", result);
                break;
            }
            char fileName2[PATH_MAX] = {0};
            realpath(fileName, fileName2);
            fp = fopen(fileName2, "wb");
            if (fp == NULL) {
                result = IOTA_FAILURE;
                PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_IotRead() fopen %s Failed\n", fileName);
                break;
            }
            ret = IOTA_IotReadHeaderData(buf, read_length, fileSize, &write_size, fp);
            if (ret == -1) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_IotRead() %s Write Failed!.\n", fileName);
            }
            if (ret <= 0) {
                break;
            }
        } else {
            if ((write_size + read_length) < fileSize && fp != NULL) {
                write_length = fwrite((void *)buf, sizeof(char), read_length, fp);
                if (write_length < read_length) {
                    PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_IotRead() %s Write Failed.\n", fileName);
                    break;
                }
                write_size += write_length;
            } else if (((write_size + read_length) >= fileSize) && (write_size < fileSize) && fp != NULL) {
                write_length = fwrite((void *)buf, sizeof(char), fileSize - write_size, fp);
                if (write_length < fileSize - write_size) {
                    PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_IotRead() %s Write Failed!.\n", fileName);
                    break;
                }
                write_size += write_length;
                break;
            } else {
                break;
            }
        }
        buf[read_length] = '\0';
    } while (read_length > 0);

    fflush(fp);
    if (fp != NULL) {
        fclose(fp);
    }
    return result;
}

HW_API_FUNC HW_INT IOTA_GetOTAPackages(HW_CHAR *url, HW_CHAR *token, HW_INT timeout)
{
    if (url == NULL || token == NULL ||
        timeout <= OTA_TIMEOUT_MIN_LENGTH) { // the timeout value must be greater than 300s
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_GetOTAPackages() the input is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }

    int result = 0;
    char ip[IP_LENGTH] = { "" };
    char *uri = IOTA_GetUrlData(url, strlen(url), ip);
    if (uri == NULL) {
        return IOTA_FAILURE;
    }

    int fd = IOTA_ConnectionDomainName(ip, IP_LENGTH, timeout);
    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() connect server success by tcp.\n");

    SSL_CTX *context = IOTA_ssl_init();
    SSL *ssl = IOTA_OpensslConnect(fd, context);
    if (ssl == NULL) {
        SSL_CTX_free(context);
        close(fd);
        return IOTA_FAILURE;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() connect to server.\n");


    char str1[HTTP_HEADER_LENGTH];
    result = IOTA_SendHttpHeader(str1, sizeof(str1), uri, ip, token);
    if (result < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "iota_datatrans: IOTA_GetOTAPackages() HTTP_HEADER_LENGTH Insufficient length \n");
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(context);
        close(fd);
        return IOTA_FAILURE;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() the request header is \n%s.\n", str1);

    int res = SSL_write(ssl, str1, strlen(str1));

    if (res < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "iota_datatrans: IOTA_GetOTAPackages() send https request failed, response is %d.\n", res);
        SSL_shutdown(ssl);
        close(fd);
        SSL_free(ssl);
        SSL_CTX_free(context);
        return IOTA_FAILURE;
    }

    result = IOTA_IotRead(ssl);
    SSL_shutdown(ssl);
    close(fd);
    SSL_free(ssl);
    SSL_CTX_free(context);
    if (result == 0) {
        PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() success.\n");
    }  
    usleep(1000 * 1000); // wait connection released
    return result;
}

HW_API_FUNC SSL_CTX *IOTA_ssl_init()
{
    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_ssl_init() start to init ssl.\n");
    static SSL_CTX *server_ctx;

    // init openssl
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    if (!RAND_poll()) {
        return NULL;
    }

    server_ctx = SSL_CTX_new(SSLv23_client_method());

    if (server_ctx == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_ssl_init() New SSL_CTX failed.\n");
        return NULL;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_ssl_init() end to init ssl.\n");
    return server_ctx;
}

HW_API_FUNC HW_INT IOTA_GetDeviceShadow(HW_CHAR *requestId, HW_CHAR *object_device_id, HW_CHAR *service_id,
    void *context)
{
    if (requestId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the requestId cannot be null\n");
        return IOTA_PARAMETER_EMPTY;
    }

    cJSON *root;
    root = cJSON_CreateObject();
    char *payload;

    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, object_device_id);
    cJSON_AddStringToObject(root, SERVICE_ID, service_id);

    payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetDeviceShadow() error,the payload is null.\n");
        return IOTA_FAILURE;
    } else {
        messageId = GetPropertiesRequest(requestId, payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_GetDeviceShadow() with requestId is %s, payload %s ==>\n",
            requestId, payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_Bootstrap()
{
    int messageId = 0;
    messageId = Bootstrap();

    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_Bootstrap()==>\n");

    return messageId;
}

HW_API_FUNC HW_INT IOTA_PropertiesReportV3(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum, void *context)
{
    if (serviceNum == 0 || pServiceData == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    cJSON *root, *data;
    root = cJSON_CreateObject();
    data = cJSON_CreateArray();
    int i;
    for (i = 0; i < serviceNum; i++) {
        cJSON *properties = cJSON_Parse(pServiceData[i].properties);
        if (properties == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
            cJSON_Delete(data);
            cJSON_Delete(root);
            return IOTA_PARSE_JSON_FAILED;
        }

        cJSON *serviceData;
        serviceData = cJSON_CreateObject();

        cJSON_AddStringToObject(serviceData, SERVICE_ID_V3, pServiceData[i].service_id);
        cJSON_AddStringToObject(serviceData, EVENT_TIME_V3, pServiceData[i].event_time);
        cJSON_AddItemToObject(serviceData, SERVICE_DATA_V3, properties);
        cJSON_AddItemToArray(data, serviceData);
    }

    cJSON_AddItemToObject(root, DATA_V3, data);
    cJSON_AddStringToObject(root, MSGTYPE, DEVIVE_REQ);

    char *payload;
    payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDevicePropertiesV3(payload, 0, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_PropertiesReportV3() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_BinaryReportV3(HW_CHAR *payload, void *context)
{
    if (payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    int messageId = ReportDevicePropertiesV3(payload, 1, context);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_BinaryReportV3() with payload %s ==>\n", payload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_CmdRspV3(ST_IOTA_COMMAND_RSP_V3 *cmdRspV3, void *context)
{
    if (cmdRspV3 == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    cJSON *root;
    root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, MID, cmdRspV3->mid);
    cJSON_AddNumberToObject(root, ERR_CODE, cmdRspV3->errcode);
    cJSON_AddStringToObject(root, MSGTYPE, DEVIVE_RSP);

    cJSON *body = cJSON_Parse(cmdRspV3->body);
    cJSON_AddItemToObject(root, BODY, body);

    char *payload;
    payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDevicePropertiesV3(payload, 0, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_CmdRspV3() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_SubscribeJsonCmdV3()
{
    return SubscribeJsonCmdV3();
}

HW_API_FUNC HW_INT IOTA_SubsrcibeBinaryCmdV3()
{
    return SubscribeBinaryCmdV3();
}

HW_API_FUNC HW_INT IOTA_SubscribeBoostrap()
{
    return SubscribeBootstrap();
}

HW_API_FUNC HW_INT IOTA_GetNTPTime(void *context)
{
    cJSON *root, *services;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    cJSON *serviceEvent;
    serviceEvent = cJSON_CreateObject();

    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, SDK_TIME);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, SDK_NTP_REQUEST);
    char *event_time = GetEventTimesStamp();
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, event_time);

    cJSON *paras;
    paras = cJSON_CreateObject();
    cJSON_AddNumberToObject(paras, DEVICE_SEND_TIME, getTime());

    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload;
    payload = cJSON_Print(root);
    MemFree(&event_time);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_GetNTPTime() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_ReportDeviceLog(HW_CHAR *type, HW_CHAR *content, HW_CHAR *timestamp, void *context)
{
    cJSON *root, *services, *serviceEvent;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    serviceEvent = cJSON_CreateObject();

    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, LOG);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, LOG_REPORT);

    cJSON *paras;
    paras = cJSON_CreateObject();
    cJSON_AddStringToObject(paras, CONTENT, content);
    cJSON_AddStringToObject(paras, TYPE, type);
    cJSON_AddStringToObject(paras, TIMESTAMP, timestamp);

    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload;
    payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_ReportDeviceLog() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}


/**
 *@Description: report device info to the iot platform
 *@param timestamp: ST_IOTA_DEVICE_INFO_REPORT structure
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or
 failure callback functions to provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_ReportDeviceInfo(ST_IOTA_DEVICE_INFO_REPORT *device_info_report, void *context)
{
    cJSON *root, *services, *serviceEvent;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    serviceEvent = cJSON_CreateObject();

    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, device_info_report->object_device_id);
    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, SDK_INFO);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, SDK_INFO_REPORT);
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, device_info_report->event_time);

    cJSON *paras;
    paras = cJSON_CreateObject();
    cJSON_AddStringToObject(paras, DEVICE_SDK_VERSION, device_info_report->device_sdk_version);
    cJSON_AddStringToObject(paras, SW_VERSION, device_info_report->sw_version);
    cJSON_AddStringToObject(paras, FW_VERSION, device_info_report->fw_version);

    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload;
    payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_ReportDeviceInfo() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

/**
 *@Description: get newest soft bus info from the iotda
 *@param bus_id: the bus id of the soft bus group.
 *@param event_id: if the parameter is not used, it is automatically generated by the iot platform, and the generation rule is a 36 bit random string composed of numbers, letters and middle dashes
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
                   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_GetLatestSoftBusInfo(HW_CHAR *bus_id, HW_CHAR *event_id, void *context) {
		//get the newest soft bus info
		cJSON *root_soft_bus;
		root_soft_bus = cJSON_CreateObject();

		cJSON *services_soft_bus;
		services_soft_bus = cJSON_CreateArray();

		cJSON *service_soft_bus;
		service_soft_bus = cJSON_CreateObject();

		cJSON *para_etmpy;
		para_etmpy = cJSON_CreateObject();

		cJSON_AddStringToObject(service_soft_bus, SERVICE_ID, SOFT_BUS_SERVICEID);
		cJSON_AddStringToObject(service_soft_bus, EVENT_TYPE, SOFT_BUS_EVENT_REQ);
		cJSON_AddStringToObject(service_soft_bus, EVENT_ID, event_id);

		cJSON_AddStringToObject(service_soft_bus, BUS_ID, bus_id);
		cJSON_AddItemToObject(service_soft_bus, PARAS, para_etmpy);

		cJSON_AddItemToArray(services_soft_bus, service_soft_bus);

		cJSON_AddItemToObject(root_soft_bus, SERVICES, services_soft_bus);
		char *payload;
		payload = cJSON_Print(root_soft_bus);

		cJSON_Delete(root_soft_bus);

		int messageId = 0;
		if (payload == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: get the newest soft bus info failed, the payload is null\n");
		} else {
			messageId = EventUp(payload, context);
			free(payload);
			if (messageId != 0) {
				PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: get the newest soft bus info failed, the result is %d\n", messageId);
			}
			return messageId;
		}
}

/**
 * ----------------------------deprecated below, do not use it-------------------------------------->
 */

// Reserved interface for transparent transmission
HW_API_FUNC HW_INT IOTA_ReportSubDeviceInfo(HW_CHAR *pcPayload, void *context)
{
    if (pcPayload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the pcPayload cannot be null\n");
        return -1;
    }

    HW_INT messageId = ReportSubDeviceInfo(pcPayload, context);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_ReportSubDeviceInfo() with payload %s ==>\n", pcPayload);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_SubDeviceVersionReport(HW_CHAR *version, void *context)
{
    if (version == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the version cannot be null\n");
        return -1;
    }
    cJSON *root;
    root = cJSON_CreateObject();
    char *payload;
    cJSON_AddStringToObject(root, MESSAGE_NAME, SUB_DEVICE_VERSION_REPORT);
    cJSON_AddStringToObject(root, VERSION, version);

    payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportSubDeviceInfo(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_SubDeviceVersionReport() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_SubDeviceProductGetReport(cJSON *product_id_list, void *context)
{
    if (product_id_list == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the product_id_list cannot be null\n");
        return -1;
    }
    cJSON *root;
    root = cJSON_CreateObject();
    char *payload;
    cJSON_AddStringToObject(root, MESSAGE_NAME, GET_PRODUCTS);
    cJSON_AddItemToObject(root, PRODUCTID_LIST, product_id_list);

    payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportSubDeviceInfo(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_SubDeviceProductGetReport() with payload %s ==>\n",
            payload);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_SubDeviceScanReport(cJSON *device_list, void *context)
{
    if (device_list == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the device_list cannot be null\n");
        return -1;
    }
    cJSON *root;
    root = cJSON_CreateObject();
    char *payload;
    cJSON_AddStringToObject(root, MESSAGE_NAME, SCAN_SUB_DEVICE_RESULT);
    cJSON_AddItemToObject(root, DEVICE_LIST, device_list);

    payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportSubDeviceInfo(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_SubDeviceScanReport() with payload %s ==>\n", payload);
        MemFree(&payload);
        return messageId;
    }
}
