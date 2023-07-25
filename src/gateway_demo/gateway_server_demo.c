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
#include "stdio.h"
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

char *gIoTPlatformIp = "接入地址"; // replace with the real access address
int gIoTPlatformPort = 8883;
int gClientSocket = -1;
// deviceId，The mqtt protocol requires the user name to be filled in. Here we use deviceId as the username
char *gUserName = "XXXX"; 
char *gPassWord = "XXXX";
int gIoTPlatformStatus = DISCONNECTED;
char *gatewayId = NULL;

int connect_failed_times = 0;

void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleConnectionLost(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleDisconnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleDisconnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleSubscribesuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleSubscribeFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleReportSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleReportFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);

void HandleCommandRequest(EN_IOTA_COMMAND *command);
void HandleEventsDown(EN_IOTA_EVENT *message);

void MyPrintLog(int level, char *format, va_list args);
void SetAuthConfig(void);
void SetMyCallbacks(void);

// ----------------------------handle ack -------------------------------------------
void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleConnectSuccess(), connect success\n");
    gIoTPlatformStatus = CONNECTED;
}

void HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleConnectFailure(), messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);

    // Check the cause of connect failure, and do what you want, Such as retrying

    connect_failed_times++;
    if (connect_failed_times < 10) {
        TimeSleep(50);
    } else if (connect_failed_times < 50) {
        TimeSleep(2500);
    }

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleConnectFailure(), connect again failed, result %d\n", ret);
    }
}

void HandleConnectionLost(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleConnectionLost() , messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
    gIoTPlatformStatus = DISCONNECTED;

    // Check the cause of lost, and do what you want, Such as reconnect

    if (connect_failed_times < 10) {
        TimeSleep(50);
    } else if (connect_failed_times < 50) {
        TimeSleep(2500);
    }

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleConnectionLost(), connect again failed, result %d\n", ret);
    }
}

void HandleDisconnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleDisconnectSuccess, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
    gIoTPlatformStatus = DISCONNECTED;
}

void HandleDisconnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleDisconnectFailure(), messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);

    int ret = IOTA_DisConnect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleDisconnectFailure(), disconnect again failed, result %d\n",
            ret);
    }
}

void HandleReportSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleReportSuccess() messageId %d\n", rsp->mqtt_msg_info->messageId);
}

void HandleSubscribesuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleSubscribesuccess() messageId %d\n",
        rsp->mqtt_msg_info->messageId);
}

void HandleSubscribeFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "gateway_demo: HandleSubscribeFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

void HandleReportFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleReportFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
    // Check the cause of connect failure, and do what you want
    // To Do ......
}

// ----------------------------------handle command or event arrive-----------------------------
void HandleCommandRequest(EN_IOTA_COMMAND *command)
{
    if (command == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleCommandRequest(), command is NULL");
        return;
    }

    // You can get deviceId,serviceId,commandName from here
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleCommandRequest(), the deviceId is %s\n",
        command->object_device_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleCommandRequest(), the service_id is %s\n", command->service_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleCommandRequest(), the command_name is %s\n",
        command->command_name);

    char *payload = EncodeCommandParas(command);
    if (payload != NULL) {
        SendMessageToSubDevice(gClientSocket, payload);
        MemFree(&payload);
    }
    SendCommandRspToIoTPlatform(command->request_id);
}

void HandleEventsDown(EN_IOTA_EVENT *message)
{
    if (message == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleEventsDown(), message is NULL");
        return;
    }

    // You can get deviceId from here
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleEventsDown(), the deviceId is %s\n", message->object_device_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleEventsDown(), the ServiceCount is %d\n", message->services_count);

    int serviceNum = 1;
    EN_IOTA_SERVICE_EVENT *servicesTmp = message->services;
    while (serviceNum <= message->services_count) {
        PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleEventsDown(), ServiceNum = %d\n", serviceNum);

        if (servicesTmp == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleEventsDown(), services is NULL, ServiceNum = %d\n",
                serviceNum);
        } else if (servicesTmp->servie_id == EN_IOTA_EVENT_SUB_DEVICE_MANAGER) {
            HandleSubDeviceManagerEvents(servicesTmp);
        } else if (servicesTmp->servie_id == EN_IOTA_EVENT_OTA) {
            HandleOTAEvents(servicesTmp);
        } else {
            PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleEventsDown(), unknown servie_id, ServiceNum = %d\n",
                serviceNum);
        }

        serviceNum++;
        servicesTmp++;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleEventsDown() end");
}

// ------------------------------------------------------------------------------------

void MyPrintLog(int level, char *format, va_list args)
{
    vprintf(format, args);
    /*
     * if you want to printf log in system log files,you can do this:
     * vsyslog(level, format, args);
     *  */
}

void SetAuthConfig(void)
{
    IOTA_ConfigSetStr(EN_IOTA_CFG_MQTT_ADDR, gIoTPlatformIp);
    IOTA_ConfigSetUint(EN_IOTA_CFG_MQTT_PORT, gIoTPlatformPort);
    IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICEID, gUserName);
    IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICESECRET, gPassWord);
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);

#ifdef _SYS_LOG
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LOCAL_NUMBER, LOG_LOCAL7);
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LEVEL, LOG_INFO);
#endif
}

void SetMyCallbacks(void)
{
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_SUCCESS, HandleConnectSuccess);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_FAILURE, HandleConnectFailure);

    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_DISCONNECT_SUCCESS, HandleDisconnectSuccess);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_DISCONNECT_FAILURE, HandleDisconnectFailure);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECTION_LOST, HandleConnectionLost);

    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_PUBLISH_SUCCESS, HandleReportSuccess);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_PUBLISH_FAILURE, HandleReportFailure);

    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_SUBSCRIBE_SUCCESS, HandleSubscribesuccess);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_SUBSCRIBE_FAILURE, HandleSubscribeFailure);

    IOTA_SetCmdCallback(HandleCommandRequest);
    IOTA_SetEventCallback(HandleEventsDown);
}

int main(int argc, char **argv)
{
#if defined(_DEBUG)
    (void)setvbuf(stdout, NULL, _IONBF, 0); // in order to make the console log printed immediately at debug mode
#endif

    IOTA_SetPrintLogCallback(MyPrintLog);

    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: start test ===================>\n");

    if (IOTA_Init(WORK_PATH) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: call IOTA_Init() error, init failed\n");
        return -1;
    }

    SetAuthConfig();
    SetMyCallbacks();

    // Connect to HuaweiCloud IoT service, do what you want in function HandleConnectSuccess
    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: connect to IoT platform error, result %d\n", ret);
        return -1;
    }

    TimeSleep(1500);

    // Create TCP Service listener
    int server_socket = CreateServerSocket(TCP_SERVER_IP, TCP_SERVER_PORT, BACK_LOG);
    if (server_socket == -1) {
        return -1;
    }

    // Accept client connection, and process the message reported by the client
    int count = 0;
    struct sockaddr_in addr_client = { 0 };
    socklen_t addrLen = sizeof(addr_client);
    while (count < 10000) {
        int clientSocket = accept(server_socket, (struct sockaddr *)&addr_client, &addrLen);
        if (clientSocket <= 0) {
            PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: accept client connect error.\n");
            continue;
        }

        if (gClientSocket != clientSocket) {
            PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: get a new client socket=%d\n", clientSocket);
            if (gClientSocket != -1) {
                close(gClientSocket);
            }
        }

        gClientSocket = clientSocket;
        ProcessMessageFromClient(gClientSocket);
        TimeSleep(1500);
        count++;
    }

    close(gClientSocket);
    gClientSocket = -1;
    close(server_socket);
    server_socket = -1;

    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: test has ended ===================>\n");

    return 0;
}
