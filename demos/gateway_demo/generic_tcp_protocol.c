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

#include <sys/socket.h>
#include <netinet/in.h>
#include <log_util.h>
#include <json_util.h>
#include <string_util.h>
#include <stdio.h>
#include "signal.h"
#include "hw_type.h"
#include "iota_init.h"
#include "iota_cfg.h"
#include "errno.h"
#include "iota_error_type.h"
#include "iota_login.h"
#include "iota_datatrans.h"
#include "cJSON.h"
#include "generic_tcp_protocol.h"

char gSubDeviceId[128] = {0};
void TimeSleep(int ms)
{
#if defined(WIN32) || defined(WIN64)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

char *getTcpDeviceId(void)
{
    return gSubDeviceId;
}

int CreateServerSocket(char *serverIp, int serverPort, int backlog)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: CreateServerSocket()\n");
    struct sockaddr_in addr = { 0 };
    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket <= 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: CreateServerSocket() create socket error.\n");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPort);
    addr.sin_addr.s_addr = inet_addr(serverIp);

    if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: CreateServerSocket() bind error.\n");
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, backlog) != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: CreateServerSocket() listen error.\n");
        close(server_socket);
        return -1;
    }

    return server_socket;
}

// Report the subdevice status.
void UpdateSubDeviceStatus(char *deviceId, int isOnline)
{
    if (deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: UpdateSubDeviceStatus() failed, deviceId is NULL\n");
        return;
    }
    int deviceNum = 1;
    ST_IOTA_DEVICE_STATUSES device_statuses;
    device_statuses.event_time = GetEventTimesStamp();
    device_statuses.device_statuses[0].device_id = deviceId;
    device_statuses.device_statuses[0].status = isOnline ? ONLINE : OFFLINE;
    int messageId = IOTA_UpdateSubDeviceStatus(&device_statuses, deviceNum, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: UpdateSubDeviceStatus() failed, messageId %d\n", messageId);
    }
    MemFree(&device_statuses.event_time);
}

// Command response
void SendCommandRspToIoTPlatform(char *requestId)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: SendCommandRspToIoTPlatform()\n");
    char *commandResponse = "{\"SupWh\": \"aaa\"}"; // in service accumulator
    int result_code = 0;
    char *response_name = "cmdResponses";

    int messageId = IOTA_CommandResponse(requestId, result_code, response_name, commandResponse, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: SendCommandRspToIoTPlatform() failed, messageId %d\n", messageId);
    }
}

// You need to implement this function if you have this requirement
static void AddSubDevice(EN_IOTA_DEVICE_PARAS *paras)
{
    int j = 0;
    while (paras->devices_count > j) {
        EN_IOTA_DEVICE_INFO device = paras->devices[j];
        PrintfLog(EN_LOG_LEVEL_INFO, "%s: add sub device, device_id: %s, node_id %s, product_id %s\n",
                    __FUNCTION__, device.device_id, device.node_id, device.product_id);
            
        // To do......
        j++;
    }

}

// You need to implement this function if you have this requirement
static void DeleteSubDevice(EN_IOTA_DEVICE_PARAS *paras)
{
    int j = 0;
    while (paras->devices_count > j) {
        EN_IOTA_DEVICE_INFO device = paras->devices[j];
        PrintfLog(EN_LOG_LEVEL_INFO, "%s: add sub device, device_id: %s, node_id %s, parent_device_id %s\n",
                    __FUNCTION__, device.device_id, device.node_id, device.parent_device_id);
            
        // To do......
        j++;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: DeleteSubDevice() start\n");
}

// You need to implement this function if you have this requirement
static void UpdateSubDeviceResponse(EN_IOTA_GTW_UODATE_STATUS_PARAS *status)
{
    int j = 0;
    while (status->successful_devices_count > j) {
        PrintfLog(EN_LOG_LEVEL_INFO, "%s: update status sub device success, device_id: %s, status %s\n", 
            __FUNCTION__, status->successful_devices[j].device_id, status->successful_devices[j].status);
        j++;
    }
    j = 0;
    while (status->failed_devices_count > j) {
        PrintfLog(EN_LOG_LEVEL_INFO, "%s: update status sub device failed, device_id: %s, error_code: %s, error_msg: %s\n",
           __FUNCTION__, status->failed_devices[j].device_id, status->failed_devices[j].error_code, status->failed_devices[j].error_msg);
        j++;
    }
}

// You need to implement this function if you have this requirement
static void AddSubDeviceResponse(EN_IOTA_GTW_ADD_DEVICE_PARAS *add_device)
{
    int j = 0;
    while (add_device->successful_devices_count > j) {
        PrintfLog(EN_LOG_LEVEL_INFO, "%s: add device sub device success, device_id: %s, node_id: %s, status %s\n", 
            __FUNCTION__, add_device->successful_devices[j].device_id, add_device->successful_devices[j].node_id, add_device->successful_devices[j].status);
        strncpy_s(gSubDeviceId, sizeof(gSubDeviceId), add_device->successful_devices[j].device_id, strlen(add_device->successful_devices[j].device_id));
        UpdateSubDeviceStatus(gSubDeviceId, 1);
        j++;
    }
    j = 0;
    while (add_device->failed_devices_count > j) {
        EN_IOTA_ADD_DEVICE_FAILED_REASON device_failed = add_device->failed_devices[j];
        PrintfLog(EN_LOG_LEVEL_INFO, "%s: add device sub device failed, node_id: %s, product_id: %s, error_code: %s, error_msg: %s\n", __FUNCTION__, 
            device_failed.node_id, device_failed.product_id, device_failed.error_code, device_failed.error_msg);
        j++;
    }
}

// You need to implement this function if you have this requirement
static void DelSubDeviceResponse(EN_IOTA_GTW_DEL_DEVICE_PARAS *del_device)
{
    int j = 0;
    while (del_device->successful_devices_count > j) {
        PrintfLog(EN_LOG_LEVEL_INFO, "%s: delete sub device success, device_id: %s\n", 
            __FUNCTION__, del_device->successful_devices[j]);
        j++;
    }
    j = 0;
    while (del_device->failed_devices_count > j) {
        PrintfLog(EN_LOG_LEVEL_INFO, "%s: delete sub device failed, device_id: %s, error_code: %s, error_msg: %s\n",
            __FUNCTION__, del_device->failed_devices[j].device_id, del_device->failed_devices[j].error_code, del_device->failed_devices[j].error_msg);
        j++;
    }
}

void HandleSubDeviceManagerEvents(EN_IOTA_SERVICE_EVENT *subDeviceServices)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleSubDeviceManagerEvents() start\n");
    switch(subDeviceServices->event_type) {
        // add a sub device
        case EN_IOTA_EVENT_ADD_SUB_DEVICE_NOTIFY :
            AddSubDevice(subDeviceServices->paras);
            break;

        case EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY :
            DeleteSubDevice(subDeviceServices->paras);
            break;

        case EN_IOTA_EVENT_UPDATE_SUB_DEVICE_RESPONSE: 
            UpdateSubDeviceResponse(subDeviceServices->gtw_update_status_paras);
            break;

        case EN_IOTA_EVENT_ADD_SUB_DEVICE_RESPONSE :
            AddSubDeviceResponse(subDeviceServices->gtw_add_device_paras);
            break;

        case EN_IOTA_EVENT_DEL_SUB_DEVICE_RESPONSE :
            DelSubDeviceResponse(subDeviceServices->gtw_del_device_paras);
            break;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleSubDeviceManagerEvents() end\n");
}

// OTA related
static void QueryVersion(void)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: QueryVersion() start\n");

    // To do......
}

// You need to implement this function if you have this requirement
static void UpgradeFirmware(EN_IOTA_OTA_PARAS *paras)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: UpgradeFirmware() start\n");

    // To do......
}

// You need to implement this function if you have this requirement
static void UpgradeSoftware(EN_IOTA_OTA_PARAS *paras)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: UpgradeSoftware() start");

    // To do......
}

void HandleSubOTAEvents(EN_IOTA_SERVICE_EVENT *otaServices)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleOTAEvents() start\n");

    if (otaServices->event_type == EN_IOTA_EVENT_VERSION_QUERY) {
        QueryVersion();
    } else if (otaServices->event_type == EN_IOTA_EVENT_FIRMWARE_UPGRADE) {
        UpgradeFirmware(otaServices->ota_paras);
    } else if (otaServices->event_type == EN_IOTA_EVENT_SOFTWARE_UPGRADE) {
        UpgradeSoftware(otaServices->ota_paras);
    } else {
        PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleOTAEvents(), Para is invalid, unknown event_type\n");
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleOTAEvents() end\n");
}

// Transform Json to Binary. Here is just an example. You need to re-implement this function according to your own encoding.
char *EncodeCommandParas(EN_IOTA_COMMAND *command)
{
    char *parasBinary = NULL;
    char *serviceid = command->service_id;
    char *commandname = command->command_name;
    char *parasJSON = command->paras; // the command paras from IoTPlatform is Json format

    if (parasJSON != NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: EncodeCommandParas(), para=%s\n", parasJSON);
        CopyStrValue(&parasBinary, parasJSON, strlen(parasJSON));
    }
    return parasBinary;
}

// Sending Data to a subdevice
void SendMessageToSubDevice(int clientSocket, char *payload)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: SendMessageToSubDevice()\n");
    if (send(clientSocket, payload, strlen(payload), 0) == -1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: SendMessageToSubDevice() send error.\n");
    }
}

// Transform Json to Binary. Here is just an example. You need to re-implement this function according to your own decoding.
static void Test_BatchPropertiesReport(char *subDeviceId, int len, char *recvBuf)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: Test_BatchPropertiesReport() start, the payload is %s \n", recvBuf);
    
    /* The following is an example, please implement it yourself */
    if (recvBuf == NULL || strlen(subDeviceId) == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: Test_BatchPropertiesReport() error, payload or subDeviceId is NULL\n");
        return;
    }
    
    int deviceNum = 1;      // Number of subdevices to be reported.
    ST_IOTA_DEVICE_DATA_INFO devices[deviceNum]; // Number of structure array to be reported by a subdevice.
    int serviceList[deviceNum]; // Setting the Number of Services Reported by a Child Device
    serviceList[0] = 1;

    devices[0].device_id = subDeviceId;
    devices[0].services[0].event_time = GetEventTimesStamp();
    devices[0].services[0].service_id = "Smoke";
    devices[0].services[0].properties = CombineStrings(3, "{\"Smoke_value\":", recvBuf, "}");

    int messageId = IOTA_BatchPropertiesReport(devices, deviceNum, serviceList, 0, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "geteway_demo: SendReportToIoTPlatform(), report properties failed, messageId %d\n", messageId);
    }

    MemFree(&devices[0].services[0].event_time);

    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: DecodeServiceProperty() finished\n");
    return;
}

static void Test_GtwAddSubDevice(char *node_id, char *product_id) 
{
    ST_IOTA_SUB_DEVICE_INFO subDeviceInfos;
    int deviceNum = 1;

    subDeviceInfos.deviceInfo[0].description = "description";
    subDeviceInfos.deviceInfo[0].name = NULL;
    subDeviceInfos.deviceInfo[0].device_id = NULL;
    subDeviceInfos.deviceInfo[0].extension_info = NULL;
    subDeviceInfos.deviceInfo[0].node_id = node_id;
    subDeviceInfos.deviceInfo[0].parent_device_id = NULL;
    subDeviceInfos.deviceInfo[0].product_id = product_id; 
    subDeviceInfos.event_id = NULL;
    subDeviceInfos.event_time = NULL;

    int messageId = IOTA_AddSubDevice(&subDeviceInfos, deviceNum, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_GtwAddSubDevice() failed, messageId %d\n", messageId);
    }
}

static void Test_GtwDelSubDevice(char *subDeviceId)
{
    if (subDeviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: Test_GtwDelSubDevice() error, subDeviceId is NULL\n");
        return;
    }
    ST_IOTA_DEL_SUB_DEVICE delSubDevices;
    int deviceNum = 1;

    delSubDevices.event_id = NULL;
    delSubDevices.event_time = NULL;
    delSubDevices.delSubDevice[0] = subDeviceId;

    int messageId = IOTA_DelSubDevice(&delSubDevices, deviceNum, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_GtwDelSubDevice() failed, messageId %d\n", messageId);
    }
}

void ProcessMessageFromClient(int clientSocket, char *subDeviceId, char *product_id)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessMessageFromClient()\n");
    char recvbuf[MAX_MESSAGE_BUF_LEN] = { 0 };

    int i = 0;
    while (i < 1000) { // In order to receive client data continuously
        memset_s(recvbuf,MAX_MESSAGE_BUF_LEN, '\0',sizeof(recvbuf));
        int recvLen = recv(clientSocket, recvbuf, MAX_MESSAGE_BUF_LEN - 1, 0);
        if (recvLen <= 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: ProcessMessageFromClient() recv error.\n");
            return;
        }

        recvbuf[recvLen] = 0;
        int len = strlen(recvbuf);
        if (len > 6 && recvbuf[0] == 'I' && recvbuf[1] == 'D' && recvbuf[2] == ':'){
            Test_GtwAddSubDevice(recvbuf + 3, product_id); // Add sub devices

        } else if (len == 4 && recvbuf[0] == 'e' && recvbuf[1] == 'x' && recvbuf[2] == 'i' && recvbuf[3] == 't') {
            UpdateSubDeviceStatus(gSubDeviceId, 0); // Update sub device status

        } else if (len == 3 &&  recvbuf[0] == 'd' && recvbuf[1] == 'e' && recvbuf[2] == 'l') {
            Test_GtwDelSubDevice(gSubDeviceId); // Delete sub devices

        } else if (len > 0) {
            Test_BatchPropertiesReport(subDeviceId, len, recvbuf); // Report sub device properties
        }

        SendMessageToSubDevice(clientSocket, "OK!");
        i++;
    }
    return;
}


