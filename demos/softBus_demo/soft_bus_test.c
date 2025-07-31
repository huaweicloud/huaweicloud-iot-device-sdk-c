/*
 * Copyright (c) 2024-2025 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#include "soft_bus_datatrans.h"
#include "soft_bus_data_process.h"
#include "dconncaseone_interface.h"

/**
 * 软总线使用示例
 * 平台提供下发软总线关系网的功能，当使用openharmony的时候可以使用该功能，实现设备间的互相通信。
 * 当使用该功能时，请在Makefile中打开该注释：#SOFT_BUS_OPTION2 := 1
 */

// You can get the access address from IoT Console "Overview" -> "Access Information"
char *g_address = "XXXX";
char *g_port = "8883";

// deviceId, the mqtt protocol requires the user name to be filled in.
// Please fill in the deviceId
char *g_deviceId = "XXXX";
char *g_secret = "XXXX";

// 网络接口名称， 软总线将通过该网络接口进行近端通信。 可通过ifconfig查看，如"eth0"。
char *g_networkInterfaceName = "eth0";

// 本机为主还是从
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

void SendDataToDeviceCb(const char *deviceId, int result)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "Data sent to [%s] result: %d\n", deviceId, result);
}

void ReceiveDataFromDeviceCb(const char *deviceId, char *receiveData, int datelen)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "Data received from device [%s]. Datalen: %d, Data: %s\n", deviceId, datelen,
        receiveData);
}

// ---------------------------- secret authentication --------------------------------------
static void MqttDeviceSecretInit(char *address, char *port, char *deviceId, char *password) 
{
    IOTA_Init("."); // The certificate address is ./conf/rootcert.pem
    IOTA_SetPrintLogCallback(MyPrintLog); 
 
    // MQTT protocol when the connection parameter is 1883; MQTTS protocol when the connection parameter is 8883
    IOTA_ConnectConfigSet(address, port, deviceId, password);
    // Set authentication method to secret authentication
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);
    
    // Set connection callback function
    IOTA_DefaultCallbackInit();
}

// ---------------------- 软总线关系网下发回调，可以在这里进行数据处理 ---------------------
void HandleEventSoftBus(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), soft_bus_info: %s \n",
                message->soft_bus_paras->bus_infos);
#if defined(SOFT_BUS_OPTION2)
    // 当下发成功 -- 可以从getSoftBusTotal()获取解析后的数据 
    soft_bus_total *soft_bus_total_data = getSoftBusTotal();
    int total = soft_bus_total_data->count;

    // 清除所有软总线连接
    CleanAllSoftBus();

    int i;
    for (i = 0; i < total; i++) {
        // 初始化软总线
        SoftBusInit(i);
    }
#endif
}

// -------------------------- 上报设备信息 -------------------------------
// 其中ifname是网络接口名称如"eth0"，可通过ifconfig查看
static void TestReportDeviceInfo(char *ifname)
{
    ST_IOTA_DEVICE_INFO_REPORT deviceInfo;
    
    char ipv4[16] = {0};
    int ret = GetLocalNetworkIpv4(ifname, ipv4, 16);

    deviceInfo.device_sdk_version = SDK_VERSION;
    deviceInfo.sw_version = "v1.0";
    deviceInfo.fw_version = "v1.0";
    deviceInfo.event_time = NULL;
    deviceInfo.device_ip = ipv4;
    deviceInfo.object_device_id = NULL;

    int messageId = IOTA_ReportDeviceInfo(&deviceInfo, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: TestReportDeviceInfo() failed, messageId %d\n", messageId);
    }
}

static void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    (void)rsp;
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: handleConnectSuccess(), login success\n");
    // 连接成功后上报设备信息
    TestReportDeviceInfo(g_networkInterfaceName);
}
 
static void SetSoftBusInit(void)
{
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_SUCCESS, HandleConnectSuccess);
    IOTA_SetEvenSoftBusCallback(HandleEventSoftBus);
#if defined(SOFT_BUS_OPTION2)
    // 注册鸿蒙软总线
    CallbackParam param = {
        .sendDataResultCb = SendDataToDeviceCb,
        .onReceiveDataCb = ReceiveDataFromDeviceCb,
        .isValidIP = isValidIP,
        .isValidDeviceID = isValidDeviceID,
        .getAuthKey = GetSlaveAuthKey,
        .getDeviceID = getDeviceId,
    };
    RegisterCallback(&param);
#endif
}


int main(int argc, char **argv)
{
    // secret authentication initialization
    MqttDeviceSecretInit(g_address, g_port, g_deviceId, g_secret); 
    SetSoftBusInit();

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "message_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    TimeSleep(1500);

    while(!IOTA_IsConnected()) {
        TimeSleep(300);
    }

    while(1) {
        // 每隔1分钟发送一次数据
        char *data = "hello!";
        SendDataToAllInfoDevice(0, data, strlen(data));
        TimeSleep(60000);
    }

    IOTA_Destroy();
}
