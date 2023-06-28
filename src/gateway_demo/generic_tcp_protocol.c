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

void TimeSleep(int ms)
{
#if defined(WIN32) || defined(WIN64)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void ProcessMessageFromClient(int clientSocket)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessMessageFromClient()\n");
    char recvbuf[MAX_MESSAGE_BUF_LEN] = { 0 };

    int i = 0;
    while (i < 1000) { // In order to receive client data continuously
        int recvLen = recv(clientSocket, recvbuf, MAX_MESSAGE_BUF_LEN - 1, 0);
        if (recvLen <= 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: ProcessMessageFromClient() recv error.\n");
            return;
        }
        recvbuf[recvLen] = "\0";

        SendReportToIoTPlatform(recvbuf);
        i++;
    }

    return;
}

void SendReportToIoTPlatform(char *recvBuf)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: SendReportToIoTPlatform()\n");

    ST_IOTA_SERVICE_DATA_INFO *payload = DecodeServiceProperty(recvBuf);

    if (payload == NULL) {
        return;
    }

    int messageId = IOTA_PropertiesReport(payload, 1, 0, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "geteway_demo: SendReportToIoTPlatform(), report properties failed, messageId %d\n", messageId);
    }

    MemFree(&payload->event_time);
    MemFree(&payload->properties);
    MemFree(&payload->service_id);
    MemFree(&payload);
}

void SendMessageToSubDevice(int clientSocket, char *payload)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: SendMessageToSubDevice()\n");
    if (send(clientSocket, payload, strlen(payload), 0) == -1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: SendMessageToSubDevice() send error.\n");
    }
}

void SendCommandRspToIoTPlatform(char *requestId)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: SendCommandRspToIoTPlatform()\n");
    char *commandResponse = "{\"SupWh\": \"aaa\"}"; // in service accumulator
    int result_code = 0;
    char *response_name = "cmdResponses";

    int messageId = IOTA_CommandResponse(requestId, result_code, response_name, commandResponse, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: SendCommandRspToIoTPlatform() failed, messageId %d\n", messageId);
    }
}

void HandleSubDeviceManagerEvents(EN_IOTA_SERVICE_EVENT *subDeviceServices)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleSubDeviceManagerEvents() start\n");

    if (subDeviceServices->event_type == EN_IOTA_EVENT_ADD_SUB_DEVICE_NOTIFY) {
        AddSubDevice(subDeviceServices->paras);
    } else if (subDeviceServices->event_type == EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY) {
        DeleteSubDevice(subDeviceServices->paras);
    } else {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "gateway_demo: HandleSubDeviceManagerEvents(), Para is invalid, unknown event_type\n");
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleSubDeviceManagerEvents() end\n");
}

void HandleOTAEvents(EN_IOTA_SERVICE_EVENT *otaServices)
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


// You need to implement this function if you have this requirement
void AddSubDevice(EN_IOTA_DEVICE_PARAS *paras)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: AddSubDevice() start\n");

    // To do......
}

// You need to implement this function if you have this requirement
void DeleteSubDevice(EN_IOTA_DEVICE_PARAS *paras)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: DeleteSubDevice() start\n");

    // To do......
}

void QueryVersion()
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: QueryVersion() start\n");

    // To do......
}

// You need to implement this function if you have this requirement
void UpgradeFirmware(EN_IOTA_OTA_PARAS *paras)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: UpgradeFirmware() start\n");

    // To do......
}

// You need to implement this function if you have this requirement
void UpgradeSoftware(EN_IOTA_OTA_PARAS *paras)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: UpgradeSoftware() start");

    // To do......
}


// Transform Json to Binary, Just a sample, You need to reimplement this function using your encode protocol
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

// Transform Binary to Json, Just a sample, You need to reimplement this function using your decode protocol
ST_IOTA_SERVICE_DATA_INFO *DecodeServiceProperty(char *recvBuf)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: DecodeServiceProperty() start, the payload is %s \n", recvBuf);

    char *jsonKeyOfServiceId[4] = {"parameter", "analog", "discrete", "accumulator"};
    char *jsonKeyOfProperties[4][2] = {{"Load",     "ImbA_strVal"},
                                       {"PhV_phsA", "PhV_phsB"},
                                       {"Ind1",     "Ind2"},
                                       {"SupWh",    "SupVarh"}};

    // Decode the payload,service1/service2
    if (recvBuf == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, payload is NULL\n");
        return NULL;
    }

    // Demo protocol definition:
    // First byte indicate service type, the value ranges from 1 to 4;
    // Second byte indicate the value of properties1, the value ranges from 0 to 9;
    // Third byte indicate the value of properties2, the value ranges from 0 to 9;
    unsigned int serviceIndex = 0;
    char serviceId[2] = {0};
    char firstPropertie[2]= {0};
    char secondPropertie[2]= {0};
    serviceId[0] = *recvBuf;
    serviceId[1] = '\0';
    firstPropertie[0] = *(recvBuf + 1);
    firstPropertie[1] = '\0';
    secondPropertie[0] = *(recvBuf + 2);
    secondPropertie[1] = '\0';
    serviceIndex = atoi(serviceId);
    if (serviceIndex > 4 || serviceIndex < 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, service id = %d, is invalid\n",
            serviceIndex);
        return NULL;
    }

    // Combine the Service id
    char *jsonsServiceId = NULL;
    if (CopyStrValue(&jsonsServiceId, jsonKeyOfServiceId[serviceIndex - 1],
        StringLength(jsonKeyOfServiceId[serviceIndex - 1])) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, copy string failed\n");
        return NULL;
    }

    // Combine the event time
    char *eventTime = GetEventTimesStamp();
    if (eventTime == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, get system time failed\n");
        MemFree(&jsonsServiceId);
        return NULL;
    }

    // Combine the properties in Json String, Sample:{"Load":"1","ImbA_strVal":"2"}
    char *serviceProperties = CombineStrings(9, "{\"", jsonKeyOfProperties[serviceIndex - 1][0], "\":\"",
        firstPropertie, "\",\"", jsonKeyOfProperties[serviceIndex - 1][1], "\":\"", secondPropertie, "\"}");
    if (serviceProperties == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, Combine Properties failed\n");
        MemFree(&jsonsServiceId);
        MemFree(&eventTime);
        return NULL;
    }

    ST_IOTA_SERVICE_DATA_INFO *serviceDataInfo = (ST_IOTA_SERVICE_DATA_INFO *)malloc(sizeof(ST_IOTA_SERVICE_DATA_INFO));
    if (serviceDataInfo == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, malloc mem failed\n");
        MemFree(&jsonsServiceId);
        MemFree(&eventTime);
        MemFree(&serviceProperties);
        return NULL;
    }

    serviceDataInfo->service_id = jsonsServiceId;
    serviceDataInfo->event_time = eventTime;
    serviceDataInfo->properties = serviceProperties;

    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: DecodeServiceProperty() finished\n");

    return serviceDataInfo;
}
