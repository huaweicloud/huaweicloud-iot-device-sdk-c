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
#include "string_util.h"
#include "iota_error_type.h"
#include "iota_login.h"
#include "iota_datatrans.h"
#include "cJSON.h"
#include "bridge_tcp_protocol.h"

char gBridgeDeviceId[128] = {0};
char gBridgePassword[128] = {0};

void TimeSleep(int ms)
{
#if defined(WIN32) || defined(WIN64)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

int CreateServerSocket(char *serverIp, int serverPort, int backlog)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: CreateServerSocket()\n");
    struct sockaddr_in addr = { 0 };
    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket <= 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: CreateServerSocket() create socket error.\n");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPort);
    addr.sin_addr.s_addr = inet_addr(serverIp);

    if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: CreateServerSocket() bind error.\n");
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, backlog) != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: CreateServerSocket() listen error.\n");
        close(server_socket);
        return -1;
    }

    return server_socket;
}

char *getTcpDeviceId(void)
{
    return gBridgeDeviceId;
}

int setTcpPassword(char *newSecret)
{
    if (newSecret) {
        memset_s(gBridgePassword, sizeof(gBridgePassword), 0, sizeof(gBridgePassword));
        strncpy_s(gBridgePassword, sizeof(gBridgePassword), newSecret, strlen(newSecret)); 
        return 0;  
    }
    return -1;
}

// Transform Json to Binary. Here is an example. You need to re-implement this function according to your encoding
char *EncodeCommandParas(EN_IOTA_COMMAND *command)
{
    /* The following is an example, please implement it by yourself */
    char *parasBinary = NULL;
    char *serviceid = command->service_id;
    char *commandname = command->command_name;
    char *parasJSON = command->paras; // the command paras from IoTPlatform is Json format

    if (parasJSON != NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: EncodeCommandParas(), para=%s\n", parasJSON);
        CopyStrValue(&parasBinary, parasJSON, strlen(parasJSON));
    }
    return parasBinary;
}

// Command response
void SendCommandRspToIoTPlatform(char *BridgeDeviceId, char *requestId)
{
    /* The following is an example, please implement it by yourself */
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: SendCommandRspToIoTPlatform()\n");
    int result_code = 0;
    char *commandResponse = "{\"Beep_State\": 123}"; // in service accumulator
    char *response_name = "Smoke_Control_Beep";

    int messageId = IOTA_BridgeDeviceCommandResponse(BridgeDeviceId, requestId, result_code, response_name, commandResponse, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: IOTA_BridgeDeviceCommandResponse() failed, messageId %d\n", messageId);
    }
}

// Sending data to TCP connected devices
void SendMessageToSubDevice(int clientSocket, char *payload)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: SendMessageToSubDevice()\n");
    if (send(clientSocket, payload, strlen(payload), 0) == -1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: SendMessageToSubDevice() send error.\n");
    }
}

// Transform Binary to Json. Here is an example. You need to re-implement this function according to your decoding
static void Test_propertiesReport(char *deviceId, int len, char *recvBuf)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: Test_BatchPropertiesReport() start, the payload is %s \n", recvBuf);

    /* The following is an example, please implement it by yourself */
    const int serviceNum = 1; // reported services' totol count
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];

    char service[100] = {0};
    int ret = sprintf_s(service, sizeof(service), "{\"Smoke_value\": %s}", recvBuf); 
    if (ret < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Test_propertiesReport() sprintf_s failed\n");
        return;
    }
    
    // if event_time is set to NULL, the time will be the iot-platform's time.
    services[0].event_time = GetEventTimesStamp(); 
    services[0].service_id = "Smoke";
    services[0].properties = service;

    int messageId = IOTA_BridgeDevicePropertiesReport(deviceId, services, serviceNum, 0, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "properties_test: Test_PropertiesReport() failed, messageId %d\n", messageId);
    }

    MemFree(&services[0].event_time);
    return;
}

static void Test_BridgeLogin(char *deviceId, char *password) 
{
    strncpy_s(gBridgeDeviceId, sizeof(gBridgeDeviceId), deviceId, strlen(deviceId));
    strncpy_s(gBridgePassword, sizeof(gBridgePassword), password, strlen(password));    
    PrintfLog(EN_LOG_LEVEL_INFO, "Test_BridgeLogin(), deviceId = %s\n", deviceId);

    int messageId = IOTA_BridgeDeviceLogin(deviceId, password, NULL, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_BridgeLogin() failed, messageId %d\n", messageId);
    }
}

static void Test_BridgeLogout(char *deviceId)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "Test_BridgeLogout(), deviceId = %s\n", deviceId);

    int messageId = IOTA_BridgeDeviceLogout(deviceId, NULL, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_BridgeLogout() failed, messageId %d\n", messageId);
    }
}

static void Test_BridgeResetSecret(char *deviceId, char *newSecret)
{
    char *oldSecret = NULL;
    if (strcmp(deviceId, gBridgeDeviceId) == 0) {
        oldSecret = gBridgePassword;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "Test_BridgeResetSecret(), deviceId = %s\n", deviceId);

    int messageId = IOTA_BridgeDeviceResetSecret(deviceId, oldSecret, newSecret, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_BridgeResetSecret() failed, messageId %d\n", messageId);
    }
}

void ProcessMessageFromClient(int clientSocket, char *subDeviceId)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: ProcessMessageFromClient()\n");
    char recvbuf[MAX_MESSAGE_BUF_LEN] = { 0 };

    int i = 0;
    while (i < 1000) { // In order to receive client data continuously
        memset_s(recvbuf, MAX_MESSAGE_BUF_LEN, '\0',sizeof(recvbuf));
        int recvLen = recv(clientSocket, recvbuf, MAX_MESSAGE_BUF_LEN - 1, 0);
        if (recvLen <= 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: ProcessMessageFromClient() recv error.\n");
            return;
        }

        recvbuf[recvLen] = 0;
        int len = strlen(recvbuf);
        if (len > strlen(BRIDGE_TCP_LOGIN) && StrInStr(recvbuf, BRIDGE_TCP_LOGIN) == recvbuf){
            char *sum = strchr(recvbuf, ';');
            int deviceIdlen = sum - recvbuf;
            if (sum < recvbuf) {
                return;
            }
            recvbuf[deviceIdlen] = 0;
            Test_BridgeLogin(recvbuf + strlen(BRIDGE_TCP_LOGIN), recvbuf + deviceIdlen + 1); // Add sub devices

        } else if (len == strlen(BRIDGE_TCP_LOGOUT) && StrInStr(recvbuf, BRIDGE_TCP_LOGOUT)== recvbuf) {
            Test_BridgeLogout(gBridgeDeviceId); // Delete sub devices

        } else if (len > strlen(BRIDGE_TCP_NEWSECRET) && StrInStr(recvbuf, BRIDGE_TCP_NEWSECRET) == recvbuf) {
            Test_BridgeResetSecret(gBridgeDeviceId, recvbuf + strlen(BRIDGE_TCP_NEWSECRET)); // Delete sub devices
        
        } else {
            Test_propertiesReport(gBridgeDeviceId, len, recvbuf); // Report sub device properties
        }

        SendMessageToSubDevice(clientSocket, "OK!");
        i++;
    }
    return;
}


