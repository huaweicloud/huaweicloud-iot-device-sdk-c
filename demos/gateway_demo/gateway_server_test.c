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
 * Device gateway sample code
 * Related Documents: https://support.huaweicloud.com/usermanual-iothub/iot_01_0052.html
 *
 * Includes basic examples:
 * 0. Report properties in batches.
 * 1. The platform notifies the gateway of adding a subdevice.
 * 2. The platform notifies the gateway of subdevice deletion.
 * 3. The gateway updates the subdevice status.
 * 4. Response to the request for updating the subdevice status
 * 5: Request for adding a subdevice
 * 6. Response to the gateway's request for adding a subdevice
 * 7: Request for deleting a subdevice.
 * 8. Response to the gateway's request for deleting a subdevice
 * 9. The platform delivers the command.
 * 10. Command delivery response from the platform
 * API description: https://support.huaweicloud.com/api-iothub/iot_06_v5_3019.html
 * Description: https://support.huaweicloud.com/usermanual-iothub/iot_01_0052.html
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
#include "generic_tcp_protocol.h"

char *gIoTPlatformIp = "XXXX"; // replace with the real access address
char *gIoTPlatformPort = "8883";
// deviceId，the mqtt protocol requires the user name to be filled in. Please fill in the deviceId
char *gUserName = "XXXX"; 
char *gPassWord = "XXXX";
// Please fill in the sub device product
char *gSubProductId = "XXXX"; 

int gClientSocket = -1;

void MyPrintLog(int level, char *format, va_list args)
{
    if(level != EN_LOG_LEVEL_DEBUG) vprintf(format, args);
    /*
     * if you want to printf log in system log files,you can do this:
     * vsyslog(level, format, args);
     *  */
}

// ----------------------------------Command Delivery Processing-----------------------------
static void HandleCommandRequest(EN_IOTA_COMMAND *command)
{
    if (command == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleCommandRequest(), command is NULL");
        return;
    }

    // You can get deviceId,serviceId,commandName from here
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleCommandRequest(), the deviceId is %s\n", command->object_device_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleCommandRequest(), the service_id is %s\n", command->service_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleCommandRequest(), the command_name is %s\n", command->command_name);

    char *payload = EncodeCommandParas(command);
    if (payload != NULL) {
        SendMessageToSubDevice(gClientSocket, payload);
        MemFree(&payload);
    }
    SendCommandRspToIoTPlatform(command->request_id);
}

// ----------- Sub-device processing -----------------
static void HandleSubDeviceManager(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message)
{
   HandleSubDeviceManagerEvents(message);
}

static void HandleOtaCallback(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message)
{
    HandleSubOTAEvents(message);
}

// --------------- Connection initialization --------------------
void mqttDeviceSecretInit(void) {
    IOTA_Init("."); 
    IOTA_SetPrintLogCallback(MyPrintLog); 
 
    // oad connection parameters
    IOTA_ConnectConfigSet(gIoTPlatformIp, gIoTPlatformPort, gUserName, gPassWord);
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);

    // Set the connection callback function.
    IOTA_DefaultCallbackInit();
    IOTA_SetCmdCallback(HandleCommandRequest);
    IOTA_SetEvenSubDeviceCallback(HandleSubDeviceManager);

#ifdef _SYS_LOG
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LOCAL_NUMBER, LOG_LOCAL7);
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LEVEL, LOG_INFO);
#endif
}

int main(int argc, char **argv)
{
#if defined(_DEBUG)
    (void)setvbuf(stdout, NULL, _IONBF, 0); // in order to make the console log printed immediately at debug mode
#endif
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: start test ===================>\n");
    mqttDeviceSecretInit();
    // Connect to HuaweiCloud IoT service
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
        
        if(gClientSocket) {
            if(strlen(getTcpDeviceId()) != 0) {
            UpdateSubDeviceStatus(getTcpDeviceId(), 1);
        }
        }
        gClientSocket = clientSocket;
        ProcessMessageFromClient(gClientSocket, getTcpDeviceId(), gSubProductId);
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
