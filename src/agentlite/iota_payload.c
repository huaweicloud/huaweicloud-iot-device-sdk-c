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
#include "cJSON.h"
#include "iota_payload.h"
#include "log_util.h"
#include "base.h"
#include "string_util.h"
#include "rule_manager.h"
#include "libgen.h"

char *IOTA_MessageReportPayload(HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, object_device_id);
    cJSON_AddStringToObject(root, NAME, name);
    cJSON_AddStringToObject(root, ID, id);
    cJSON_AddStringToObject(root, CONTENT, content);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);

    return payload;
}

char *IOTA_PropertiesReportPayload(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum)
{
    if ((serviceNum == 0) || (pServiceData == NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
        return NULL;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *serviceDatas = cJSON_CreateArray();
    int i;
    for (i = 0; i < serviceNum; i++) {
        cJSON *properties = cJSON_Parse(pServiceData[i].properties);
        if (properties == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
            cJSON_Delete(serviceDatas);
            cJSON_Delete(root);
            return NULL;
        }

        cJSON *tmp = cJSON_CreateObject();

        cJSON_AddStringToObject(tmp, SERVICE_ID, pServiceData[i].service_id);
        cJSON_AddStringToObject(tmp, EVENT_TIME, pServiceData[i].event_time);
        cJSON_AddItemToObject(tmp, PROPERTIES, properties);
        cJSON_AddItemToArray(serviceDatas, tmp);
    }

    RuleMgr_CachePropertiesValue(serviceDatas);
    RuleMgr_CheckAndExecuteNoTimers();

    cJSON_AddItemToObject(root, SERVICES, serviceDatas);
    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_BatchPropertiesReportPayload(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum,
    HW_INT serviceLenList[])
{
    if ((deviceNum == 0) || (serviceLenList == NULL) || (pDeviceData == NULL)) {
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

    cJSON *root = cJSON_CreateObject();
    cJSON *deviceDatas = cJSON_CreateArray();
    int i, j;
    for (i = 0; i < deviceNum; i++) {
        cJSON *deviceData = cJSON_CreateObject();
        cJSON *services = cJSON_CreateArray();
        cJSON_AddStringToObject(deviceData, DEVICE_ID, pDeviceData[i].device_id);

        for (j = 0; j < serviceLenList[i]; j++) {
            cJSON *properties = cJSON_Parse(pDeviceData[i].services[j].properties);
            if (properties == NULL) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
                cJSON_Delete(services);
                cJSON_Delete(deviceData);
                cJSON_Delete(deviceDatas);
                cJSON_Delete(root);
                return NULL;
            }

            cJSON *tmp = cJSON_CreateObject();
            cJSON_AddStringToObject(tmp, SERVICE_ID, pDeviceData[i].services[j].service_id);
            cJSON_AddStringToObject(tmp, EVENT_TIME, pDeviceData[i].services[j].event_time);
            cJSON_AddItemToObject(tmp, PROPERTIES, properties);
            cJSON_AddItemToArray(services, tmp);
        }

        cJSON_AddItemToObject(deviceData, SERVICES, services);
        cJSON_AddItemToArray(deviceDatas, deviceData);
    }

    cJSON_AddItemToObject(root, DEVICES, deviceDatas);
    char *payload = cJSON_Print(root);
    char *payloadDynamic = CombineStrings(1, payload);
    cJSON_Delete(root);
    MemFree(&payload);
    return payloadDynamic;
}

char *IOTA_CommandResponsePayload(HW_INT result_code, HW_CHAR *response_name, HW_CHAR *pcCommandResponse)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, RESULT_CODE, result_code);
    cJSON_AddStringToObject(root, RESPONSE_NAME, response_name);
    if (pcCommandResponse) {
        cJSON *commandResponse = cJSON_Parse(pcCommandResponse);
        if (commandResponse == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
            cJSON_Delete(root);
            return NULL;
        }
        cJSON_AddItemToObject(root, PARAS, commandResponse);
    }
    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_PropertiesSetResponsePayload(HW_INT result_code, HW_CHAR *result_desc)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, RESULT_CODE, result_code);

    if (result_desc != 0) {
        cJSON_AddStringToObject(root, RESULT_DESC, result_desc);
    }
    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_PropertiesGetResponsePayload(ST_IOTA_SERVICE_DATA_INFO serviceProp[], HW_INT serviceNum)
{
    if (serviceNum <= 0) {
        return NULL;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();
    int i;

    if ((serviceProp[0].service_id != 0) && (serviceProp[0].properties != 0)) {
        for (i = 0; i < serviceNum; i++) {
            if ((serviceProp[i].service_id != 0) && (serviceProp[i].properties != 0)) {
                cJSON *properties = cJSON_Parse(serviceProp[i].properties);
                if (properties == NULL) {
                    PrintfLog(EN_LOG_LEVEL_ERROR, "parse JSON failed.\n");
                    cJSON_Delete(services);
                    cJSON_Delete(root);
                    return NULL;
                }

                cJSON *tmp = cJSON_CreateObject();

                cJSON_AddStringToObject(tmp, SERVICE_ID, serviceProp[i].service_id);
                cJSON_AddStringToObject(tmp, EVENT_TIME, serviceProp[i].event_time);
                cJSON_AddItemToObject(tmp, PROPERTIES, properties);

                cJSON_AddItemToArray(services, tmp);
            } else {
                PrintfLog(EN_LOG_LEVEL_ERROR, "the payload is wrong.\n");
                cJSON_Delete(services);
                cJSON_Delete(root);
                return NULL;
            }
        }
    }
    cJSON_AddItemToObject(root, SERVICES, services);
    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_GetDeviceShadowPayload(HW_CHAR *object_device_id, HW_CHAR *service_id)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, object_device_id);
    cJSON_AddStringToObject(root, SERVICE_ID, service_id);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_UpdateSubDeviceStatusPayload(ST_IOTA_DEVICE_STATUSES *device_statuses, HW_INT deviceNum)
{
    if ((device_statuses == NULL) || (deviceNum < 0) || (deviceNum > MaxSubDeviceCount)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_UpdateSubDeviceStatus() error, the input is invalid.\n");
        return NULL;
    }

    if ((device_statuses->device_statuses[0].device_id == NULL) || (device_statuses->device_statuses[0].status == NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_UpdateSubDeviceStatus() error, the input of "
            "device_statuses->device_statuses[0] is invalid.\n");
        return NULL;
    }

    
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();
    cJSON *device_statuses_json = cJSON_CreateArray();
    cJSON *service = cJSON_CreateObject();
    cJSON_AddStringToObject(service, SERVICE_ID, SUB_DEVICE_MANAGER);
    cJSON_AddStringToObject(service, EVENT_TYPE, SUB_DEVICE_UPDATE_STATUS);
    cJSON_AddStringToObject(service, EVENT_TIME, device_statuses->event_time);

    int i;
    for (i = 0; i < deviceNum; i++) {
        if ((device_statuses->device_statuses[i].device_id != NULL) &&
            (device_statuses->device_statuses[i].status != NULL)) {
            cJSON *tmp = cJSON_CreateObject();

            cJSON_AddStringToObject(tmp, DEVICE_ID, device_statuses->device_statuses[i].device_id);
            cJSON_AddStringToObject(tmp, STATUS, device_statuses->device_statuses[i].status);

            cJSON_AddItemToArray(device_statuses_json, tmp);
        } else {
            PrintfLog(EN_LOG_LEVEL_ERROR, "the payload is wrong.\n");
            cJSON_Delete(device_statuses_json);
            cJSON_Delete(service);
            cJSON_Delete(services);
            cJSON_Delete(root);
            return NULL;
        }
    }

    cJSON *paras = cJSON_CreateObject();
    cJSON_AddItemToObject(paras, DEVICE_STATUS, device_statuses_json);
    cJSON_AddItemToObject(service, PARAS, paras);
    cJSON_AddItemToArray(services, service);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_AddSubDevicePayload(ST_IOTA_SUB_DEVICE_INFO *subDevicesInfo, HW_INT deviceNum)
{
    if ((subDevicesInfo == NULL) || (deviceNum < 0) || (deviceNum > MaxAddedSubDevCount)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_AddSubDevice() error, the input is invalid.\n");
        return NULL;
    }
    if ((subDevicesInfo->deviceInfo == NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "iota_datatrans: IOTA_AddSubDevice() error, the input of subDevicesInfo->deviceInfo is invalid.\n");
        return NULL;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();
    cJSON *subDeviceInfo = cJSON_CreateArray();

    cJSON *service = cJSON_CreateObject();
    cJSON_AddStringToObject(service, SERVICE_ID, SUB_DEVICE_MANAGER);
    cJSON_AddStringToObject(service, EVENT_TYPE, ADD_SUB_DEVICE_REQUEST);
    cJSON_AddStringToObject(service, EVENT_TIME, subDevicesInfo->event_time);
    cJSON_AddStringToObject(service, EVENT_ID, subDevicesInfo->event_id);

    int i;
    for (i = 0; i < deviceNum; i++) {
        if ((subDevicesInfo->deviceInfo[i].node_id != NULL) && (subDevicesInfo->deviceInfo[i].product_id != NULL)) {
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
            return NULL;
        }
    }

    cJSON *paras = cJSON_CreateObject();
    cJSON_AddItemToObject(paras, DEVICES, subDeviceInfo);

    cJSON_AddItemToObject(service, PARAS, paras);

    cJSON_AddItemToArray(services, service);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_DelSubDevicePayload(ST_IOTA_DEL_SUB_DEVICE *delSubDevices, HW_INT deviceNum)
{
    if ((delSubDevices == NULL) || (deviceNum < 0) || (deviceNum > MaxDelSubDevCount)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_DelSubDevice() error, the input is invalid.\n");
        return NULL;
    }
    if (delSubDevices->delSubDevice[0] == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "iota_datatrans: IOTA_DelSubDevice() error, the input of delSubDevices->delSubDevice[0] is invalid.\n");
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();
    cJSON *service = cJSON_CreateObject();
    cJSON_AddStringToObject(service, SERVICE_ID, SUB_DEVICE_MANAGER);
    cJSON_AddStringToObject(service, EVENT_TYPE, DEL_SUB_DEVICE_REQUEST);
    cJSON_AddStringToObject(service, EVENT_TIME, delSubDevices->event_time);
    cJSON_AddStringToObject(service, EVENT_ID, delSubDevices->event_id);

    cJSON *delSubDevice = cJSON_CreateStringArray(delSubDevices->delSubDevice, deviceNum);
    cJSON *paras = cJSON_CreateObject();
    cJSON_AddItemToObject(paras, DEVICES, delSubDevice);

    cJSON_AddItemToObject(service, PARAS, paras);

    cJSON_AddItemToArray(services, service);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_OTAVersionReportPayload(ST_IOTA_OTA_VERSION_INFO otaVersionInfo)
{
    if (((otaVersionInfo.fw_version == NULL) && (otaVersionInfo.sw_version == NULL))) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_OTAVersionReport:the input is invalid.\n");
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *tmp = cJSON_CreateObject();
    cJSON *paras = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();

    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, otaVersionInfo.object_device_id);
    cJSON_AddStringToObject(tmp, SERVICE_ID, OTA);
    cJSON_AddStringToObject(tmp, EVENT_TIME, otaVersionInfo.event_time);
    cJSON_AddStringToObject(tmp, EVENT_TYPE, VERSION_REPORT);
    if (otaVersionInfo.sw_version != NULL) {
        cJSON_AddStringToObject(paras, SW_VERSION, otaVersionInfo.sw_version);
    }
    if (otaVersionInfo.fw_version != NULL) { 
        cJSON_AddStringToObject(paras, FW_VERSION, otaVersionInfo.fw_version);
    }
    cJSON_AddItemToObject(tmp, PARAS, paras);

    cJSON_AddItemToArray(services, tmp);

    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_OTAStatusReportPayload(ST_IOTA_UPGRADE_STATUS_INFO otaStatusInfo)
{
    if ((otaStatusInfo.progress > 100) || (otaStatusInfo.progress < 0)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_OTAVersionReport:the progress is invalid.\n");
        return NULL;
    }

    if (!((otaStatusInfo.result_code >= 0 && otaStatusInfo.result_code <= 10) || (otaStatusInfo.result_code == 255))) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_OTAVersionReport:the result_code is invalid.\n");
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *tmp = cJSON_CreateObject();
    cJSON *paras = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();

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

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_GetFileUrlPayload(const ST_IOTA_DOWNLOAD_FILE *file, int isup)
{
    if(!file || !file->file_name) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_GetFileUrl: download or filename must not be none!");
        return NULL;
    }
   
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_AddArrayToObject(root, SERVICES);
    cJSON *serviceEvent = cJSON_CreateObject();
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, FILE_MANAGER);
    if (isup) {
        cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, GET_UPLOAD_URL);
    } else {
        cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, GET_DOWNLOAD_URL);
        
    }
    char *event_time = GetEventTimesStamp();
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, event_time);
    cJSON *paras = cJSON_AddObjectToObject(serviceEvent, PARAS);
    if ((!root) || (!services) || (!serviceEvent) || (!paras) || (!event_time)) {
        MemFree(&event_time);
        cJSON_Delete(root);
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_GetFileUrl: memory alloc failed!");
        return NULL;
    }
    const char *filename = basename(file->file_name);
    const char *username = GetConfig(EN_BASE_CONFIG_USERNAME);
    char *obsFileName = CombineStrings(1, filename);
    cJSON_AddStringToObject(paras, "file_name", obsFileName);
    MemFree(&event_time);
    MemFree(&obsFileName);

    if (file->hash_code || file->size) {
        cJSON *attr = cJSON_AddObjectToObject(paras, "file_attributes");
        cJSON_AddNumberToObject(paras, "size", file->size);
        cJSON_AddStringToObject(paras, "hash_code", file->hash_code);
    }

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_GetNTPTimePayload(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();
    cJSON *serviceEvent = cJSON_CreateObject();

    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, SDK_TIME);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, SDK_NTP_REQUEST);
    char *event_time = GetEventTimesStamp();
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, event_time);

    cJSON *paras = cJSON_CreateObject();
    cJSON_AddNumberToObject(paras, DEVICE_SEND_TIME, getTime());

    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    MemFree(&event_time);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_ReportDeviceInfoPayload(ST_IOTA_DEVICE_INFO_REPORT *device_info_report)
{
    if (device_info_report == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_GetFileUrl: download or filename must not be none!");
        return NULL;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();
    cJSON *serviceEvent = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, device_info_report->object_device_id);
    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, SDK_INFO);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, SDK_INFO_REPORT);
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, device_info_report->event_time);

    cJSON *paras = cJSON_CreateObject();
    cJSON_AddStringToObject(paras, DEVICE_SDK_VERSION, device_info_report->device_sdk_version);
    cJSON_AddStringToObject(paras, SW_VERSION, device_info_report->sw_version);
    cJSON_AddStringToObject(paras, FW_VERSION, device_info_report->fw_version);
    if (device_info_report->device_ip != NULL) {
        cJSON_AddStringToObject(paras, DEVICE_IP, device_info_report->device_ip);
    }

    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}

char *IOTA_GetLatestSoftBusInfoPayload(HW_CHAR *busId, HW_CHAR *eventId)
{
    cJSON *root_soft_bus;
    root_soft_bus = cJSON_CreateObject();

    cJSON *services_soft_bus;
    services_soft_bus = cJSON_CreateArray();

    cJSON *service_soft_bus;
    service_soft_bus = cJSON_CreateObject();

    cJSON *paras = cJSON_CreateObject();

    cJSON_AddStringToObject(service_soft_bus, SERVICE_ID, SOFT_BUS_SERVICEID);
    cJSON_AddStringToObject(service_soft_bus, EVENT_TYPE, SOFT_BUS_EVENT_REQ);

    if (eventId != NULL) {
        cJSON_AddStringToObject(service_soft_bus, EVENT_ID, eventId);
    }
    if (busId != NULL) {
        cJSON_AddStringToObject(paras, BUS_ID, busId);
    }

    cJSON_AddItemToObject(service_soft_bus, PARAS, paras);
    cJSON_AddItemToArray(services_soft_bus, service_soft_bus);
    cJSON_AddItemToObject(root_soft_bus, SERVICES, services_soft_bus);
    char *payload = cJSON_Print(root_soft_bus);
    cJSON_Delete(root_soft_bus);
    return payload;
}

char *IOTA_ReportDeviceLogPayload(HW_CHAR *type, HW_CHAR *content, HW_CHAR *timestamp)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();
    cJSON *serviceEvent = cJSON_CreateObject();

    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, LOG);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, LOG_REPORT);

    cJSON *paras = cJSON_CreateObject();
    cJSON_AddStringToObject(paras, CONTENT, content);
    cJSON_AddStringToObject(paras, TYPE, type);
    cJSON_AddStringToObject(paras, TIMESTAMP, timestamp);

    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    return payload;
}
