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
#include "iota_init.h"
#include "iota_datatrans.h"
#include "iota_defaultCallback.h"
#include "string_util.h"
#include "log_util.h"
#include "iota_cfg.h"
#include "subscribe.h"

/*
 * Basic examples
 * Including: 1. secret authentication
 *   2. Certificate authentication
 *   3. Reconnection
 *   4. Message reporting and sending
 *   5. Disconnection
 */

// You can get the access address from IoT Console "Overview" -> "Access Information"
char *g_address = "XXXX"; 
char *g_port = "8883";

// deviceId, the mqtt protocol requires the user name to be filled in.
// Here we use deviceId as the username
char *g_deviceId = "XXXX"; 
char *g_password = "XXXX";

void TimeSleep(int ms)
{
#if defined(WIN32) || defined(WIN64)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

static void MyPrintLog(int level, char *format, va_list args)
{
    vprintf(format, args);
    /*
     * if you want to printf log in system log files, you can do this:
     * vsyslog(level, format, args);
     */
}

// Message downstream sending callback
static void HandleMessageDown(EN_IOTA_MESSAGE *rsp, void *mqttv5)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "basic_test: %s(), content: %s\n", __FUNCTION__,  rsp->content);
    PrintfLog(EN_LOG_LEVEL_INFO, "basic_test: %s(), id: %s\n", __FUNCTION__, rsp->id);
    PrintfLog(EN_LOG_LEVEL_INFO, "basic_test: %s(), name: %s\n", __FUNCTION__,  rsp->name);
    PrintfLog(EN_LOG_LEVEL_INFO, "basic_test: %s(), objectDeviceId: %s\n", __FUNCTION__, rsp->object_device_id);
}


// Connection failure callback
static void HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    static int g_connectFailedTimes = 0;
    PrintfLog(EN_LOG_LEVEL_ERROR, "test_bootstrap: HandleConnectFailure() error, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);

    g_connectFailedTimes++;
    if (g_connectFailedTimes < 10) {
        TimeSleep(3000);
    } else {
        TimeSleep(20000);
    }

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "basic_test: HandleAuthFailure() error, login again failed, result %d\n", ret);
    }
}

static void SubscribeDeviceTpoic(void)
{
    SubscribeAllQos(1);
}

static void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    (void)rsp;
    PrintfLog(EN_LOG_LEVEL_INFO, "basic_test: HandleConnectSuccess(), login success\n");
    /* Connection successful, subscribe */
    SubscribeDeviceTpoic();
}

// ---------------------------- secret authentication --------------------------------------
static void mqttDeviceSecretInit(char *address, char *port, char *deviceId, char *password) {
    
    IOTA_Init("."); // The certificate address is set to: ./conf/rootcert.pem
    // IOTA_InitConf()
    IOTA_SetPrintLogCallback(MyPrintLog); // Set log printing method
    
    // MQTT protocol when the connection parameter is 1883; MQTTS protocol when the connection parameter is 8883
    IOTA_ConnectConfigSet(address, port, deviceId, password);
    // Set authentication method to secret authentication
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);

    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_SUCCESS, HandleConnectSuccess);
    // Set connection callback function
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_FAILURE, HandleConnectFailure);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECTION_LOST, HandleConnectFailure);
}

// ---------------------------- Certificate authentication --------------------------------------
void mqttDeviceCertInit(char *address, char *port, char *deviceId, char *deviceKyePassword) {

    // Change the device certificate/secret name to "./conf/deviceCert.pem" and "./conf/deviceCert.key" respectively
    IOTA_Init("."); 
    // Set log printing method
    IOTA_SetPrintLogCallback(MyPrintLog); 
    
    // Load connection parameter and the protocol is MQTTS
    IOTA_ConnectConfigSet(address, port, deviceId, NULL);
    // Set authentication method to certificate authentication
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_CERT);
    IOTA_ConfigSetUint(EN_IOTA_CFG_PRIVATE_KEY_PASSWORD, deviceKyePassword);

    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_SUCCESS, HandleConnectSuccess);
    // Set connection callback function
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_FAILURE, HandleConnectFailure);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECTION_LOST, HandleConnectFailure);
}


int main(int argc, char **argv) {

    // secret authentication initialization
    mqttDeviceSecretInit(g_address, g_port, g_deviceId, g_password);  

    // Certificate authentication initialization
    // char *deviceKyePassword = ""; // Please enter the device private key password
    // mqttDeviceCertInit(g_address, g_port, g_deviceId, deviceKyePassword); 

    // Message downstream sending callback
    IOTA_SetMessageCallback(HandleMessageDown);
    
    // Create connection
    int ret = IOTA_Connect();
     if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "basic_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    
    while(!IOTA_IsConnected()) {
        TimeSleep(300);
    }

    // Message reporting
    while(1) {
        // Report a message with ID of "id" to the device, with the message content "hello!"
        ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "hello", NULL};
        int messageId = IOTA_MessageDataReport(mass, NULL);
        if (messageId < 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "basic_test: Test_MessageReport() failed, messageId %d\n", messageId);
        }
        TimeSleep(3000);
    }
    // Disconnect
    IOTA_Destroy();
}
