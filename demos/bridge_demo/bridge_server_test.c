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

/*
 * Device gateway example
 * Related documents: https://support.huaweicloud.com/usermanual-iothub/iot_01_0052.html
 * 
 * Including basic examples:
 * 0. Gateway device reporting properties
 * 1. Gateway device logout
 * 2. Gateway device login
 * 3. Gateway changes device secret
 * 4. Gateway disconnects device connection
 * 5. Send commands to the gateway and then to the sub device
 * 6. Send messages to the gateway and then to the sub device
 * Document Description: https://support.huaweicloud.com/bestpractice-iothub/iot_bp_0009.html
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <log_util.h>
#include <string_util.h>
#include "stdio.h"
#include "signal.h"
#include "iota_init.h"
#include "iota_cfg.h"
#include "errno.h"
#include "iota_error_type.h"
#include "iota_login.h"
#include "iota_datatrans.h"
#include "bridge_tcp_protocol.h"

char *gAddress = "XXXX"; // replace with the real access address
char *gPort = "8883";
// deviceId，the mqtt protocol requires the user name to be filled in. 
char *gUserName = "XXXXX"; 
char *gPassWord = "XXXX";

int gClientSocket = -1;

void MyPrintLog(int level, char *format, va_list args)
{
    if(level != EN_LOG_LEVEL_DEBUG) vprintf(format, args);
    /*
     * if you want to printf log in system log files,you can do this:
     * vsyslog(level, format, args);
     *  */
}

// ----------------------------------handle command or event arrive-----------------------------
static void HandleCommandRequest(EN_IOTA_COMMAND *command)
{
    if (command == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: HandleCommandRequest(), command is NULL");
        return;
    }

    // You can get deviceId, serviceId, commandName from here
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: HandleCommandRequest(), bridgeDeviceId = %s, service_id = %s, command_name = %s\n",
        command->object_device_id, command->service_id, command->command_name);

    int result_code = 1;
    char *payload = EncodeCommandParas(command);
    if (payload != NULL) {
        result_code = 0;
        SendMessageToSubDevice(gClientSocket, payload);
        MemFree(&payload);
    }
    SendCommandRspToIoTPlatform(command->bridge_device_id, command->request_id);
}

static void HandleMessageDown(EN_IOTA_MESSAGE *rsp, void *mqttv5)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: HandleMessageDown(), bridgeDeviceId = %s, id = %s, content = %s\n",
        rsp->object_device_id, rsp->id, rsp->content);
    
    /* The following is an example, please implement it by yourself */
}

static void HandleLoginCommandRequest(EN_IOTA_BRIDGES_LOGIN *message)
{
    if (message == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: HandleLoginCommandRequest(), command is NULL");
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: HandleLoginCommandRequest(), the deviceId = %s, resultCode = %d\n", message->bridge_device_id,
        message->result_code);
}

static void HandleLogoutCommandRequest(EN_IOTA_BRIDGES_LOGOUT *message)
{
    if (message == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: HandleLogoutCommandRequest(), command = NULL");
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: HandleLogoutCommandRequest(), the deviceId = %s, resultCode = %d\n", message->bridge_device_id,
        message->result_code);
}

static void HandleResetSecretCommandRequest(EN_IOTA_BRIDGES_RESET_SECRET *message)
{
    if (message == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: HandleResetSecretCommandRequest(), command is NULL");
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: HandleResetSecretCommandRequest(), the deviceId = %s, requestId = %s, resultCode = %d, paras = %s\n", 
        message->bridge_device_id, message->request_id, message->result_code, message->paras);
    
    setTcpPassword(message->new_secret);
}

static void HandleDeviceDisConnCommandRequest(EN_IOTA_BRIDGES_PALLET_DISCONNECT *message)
{
    if (message == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bridge_demo: HandleDeviceDisConnCommandRequest(), command is NULL");
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: HandleDeviceDisConnCommandRequest(), the deviceId = %s\n", 
        message->bridge_device_id);
}

// ------------------------- Connection initialization --------------------------------
void mqttDeviceSecretInit(void) {
    IOTA_Init("."); 
    IOTA_SetPrintLogCallback(MyPrintLog); 
 
    // oad connection parameters
    IOTA_ConnectConfigSet(gAddress, gPort, gUserName, gPassWord);
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);

    // Set connection callback function
    IOTA_DefaultCallbackInit();
    IOTA_SetBridgesDeviceLoginCallback(HandleLoginCommandRequest); 
    IOTA_SetBridgesDeviceLogoutCallback(HandleLogoutCommandRequest); 
    IOTA_SetBridgesDeviceResetSecretCallback(HandleResetSecretCommandRequest); 
    IOTA_SetBridgesDeviceDisConnCallback(HandleDeviceDisConnCommandRequest); 
    IOTA_SetCmdCallback(HandleCommandRequest); 
    IOTA_SetMessageCallback(HandleMessageDown);


#ifdef _SYS_LOG
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LOCAL_NUMBER, LOG_LOCAL7);
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LEVEL, LOG_INFO);
#endif
}

int main(int argc, char **argv)
{
    mqttDeviceSecretInit();
    // Connect to HuaweiCloud IoT service
    int ret = IOTA_BridgeConnect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: connect to IoT platform error, result %d\n", ret);
        return -1;
    }

    TimeSleep(1500);

    // Create TCP Service listener
    int server_socket = CreateServerSocket(TCP_SERVER_IP, TCP_SERVER_PORT, BACK_LOG);
    if (server_socket == -1) {
        PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: server_socket error, server_socket %d\n", server_socket);
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
        ProcessMessageFromClient(gClientSocket, getTcpDeviceId());
        TimeSleep(1500);
        count++;
    }

    close(gClientSocket);
    gClientSocket = -1;
    close(server_socket);
    server_socket = -1;

    PrintfLog(EN_LOG_LEVEL_INFO, "bridge_demo: test has ended ===================>\n");
    while(1);
    return 0;
}
