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
#include "string_util.h"
#include "log_util.h"
#include "iota_cfg.h"
#include "mqttv5_util.h"
#include "subscribe.h"

/*
 * Example of message reporting and downstream sending
 * 1、System topic reporting and downstream sending
 * 2、Topics defined in the product are prefixed with $oc/devices/{device_id}/user/
 * 3、Topics that do not start with $oc. for example, /aircondition/data/up.
 * About customizing topic, see https://support.huaweicloud.com/usermanual-iothub/iot_02_9997.html
 */

// You can get the access address from IoT Console "Overview" -> "Access Information"
char *g_address = "XXXX"; 
char *g_port = "8883";

// deviceId, the mqtt protocol requires the user name to be filled in.
// Please fill in the deviceId
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
     * if you want to printf log in system log files,you can do this:
     * vsyslog(level, format, args);
     */
}

static void HandleSubscribesuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    // You can write the actions that need to be taken after a successful subscription here
}

static void HandleSubscribeFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "message_test: HandleSubscribeFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

static void SubscribeDeviceTpoic(void)
{
    SubscribeMessageDownQos(0);
    // Subscription topic is "message/test", Qos is 1
    IOTA_SubscribeTopic("message/test", 1);
}

static void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "message_test: handleConnectSuccess(), login success\n");
    /* You can subscribe to the required topics here */
    SubscribeDeviceTpoic();
}

// System topic message downstream sending callback function
static void HandleMessageDown(EN_IOTA_MESSAGE *rsp, void *mqttv5)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "message_test: %s(), content: %s\n", __FUNCTION__,  rsp->content);
    PrintfLog(EN_LOG_LEVEL_INFO, "message_test: %s(), id: %s\n", __FUNCTION__, rsp->id);
    PrintfLog(EN_LOG_LEVEL_INFO, "message_test: %s(), name: %s\n", __FUNCTION__,  rsp->name);
    PrintfLog(EN_LOG_LEVEL_INFO, "message_test: %s(), objectDeviceId: %s\n", __FUNCTION__, rsp->object_device_id);
}

// Custom topic message downstream sending callback
static void HandleRawMessageDown(EN_IOTA_RAW_MESSAGE *rsp, void *mqttv5)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "message_test: %s(), content: %s\n", __FUNCTION__,  rsp->payload);
}

// Topic callback function that does not start with $oc
static void HandletUndefinedMessageDown(EN_IOTA_UNDEFINED_MESSAGE *rsp)
{
   if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "message_test: %s(), content: %s\n", __FUNCTION__,  rsp->payload);
}

// --------------------- System topic message -----------------------------
void basicMessage(void) 
{
    // Subscribe system topic
    SubscribeAll(); 
    //  The message content is "hello!"
    ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "hello!", NULL};
    int messageId = IOTA_MessageDataReport(mass, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "message_test: basicMessage() failed, messageId %d\n", messageId);
    }
    TimeSleep(3000);
}

// ------------------ Custom topic message -----------------------------
void customMessage(void) 
{
    // Subscribed topic is "$oc/devices/{device_id}/user/test"
    IOTA_SubscribeUserTopic("test");
    // The message content is "hello!"
    ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "hello!", "test"};
    int messageId = IOTA_MessageDataReport(mass, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "message_test: customMessage() failed, messageId %d\n", messageId);
    }
}

// ----------------- topics that do not start with $oc ------------------------
void allCustomMessage(void) 
{
    // Subscribed topic is "message/test", Qos is 1
    int messageId = IOTA_RawTopicMessageReport("message/test", "hello!", 1, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "message_test: allcustomMessage() failed, messageId %d\n", messageId);
    }
    TimeSleep(3000);
}

// ---------------------------- secret authentication --------------------------------------
static void mqttDeviceSecretInit(char *address, char *port, char *deviceId, char *password) 
{
    IOTA_Init("."); // The certificate address is ./conf/rootcert.pem
    IOTA_SetPrintLogCallback(MyPrintLog); 
 
 
    // MQTT protocol when the connection parameter is 1883; MQTTS protocol when the connection parameter is 8883
    IOTA_ConnectConfigSet(address, port, deviceId, password);
    // Set authentication method to secret authentication
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);
    
    // Set connection callback function
    IOTA_DefaultCallbackInit();
    // Subscription callback function
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_SUBSCRIBE_SUCCESS, HandleSubscribesuccess);
	IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_SUBSCRIBE_FAILURE, HandleSubscribeFailure);

    // connect success callback
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_SUCCESS, HandleConnectSuccess);
}

int main(int argc, char **argv) 
{

    // secret authentication initialization
    mqttDeviceSecretInit(g_address, g_port, g_deviceId, g_password); 

    // message down callback function
    IOTA_SetMessageCallback(HandleMessageDown);
    // Custom topic message down callback function
    IOTA_SetUserTopicMsgCallback(HandleRawMessageDown);
    // topics that do not start with $oc, message down callback function
    IOTA_SetUndefinedMessageCallback(HandletUndefinedMessageDown);

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "message_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    TimeSleep(1500);

    while(!IOTA_IsConnected()) {
        TimeSleep(300);
    }

    // massage reporting
    basicMessage();
    TimeSleep(3000);
    
    // Custom topic message reporting and downstream sending
    customMessage();
    TimeSleep(3000);

    // Send data to the topic "massge/test" and subscribe to the topic
    allCustomMessage();
    TimeSleep(5000);

    IOTA_Destroy();
}
