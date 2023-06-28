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

#include <stdio.h>
#include <signal.h>

#if defined(WIN32) || defined(WIN64)
#include "windows.h"
#endif

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "hw_type.h"
#include "iota_init.h"
#include "iota_cfg.h"
#include "log_util.h"
#include "json_util.h"
#include "string_util.h"
#include "iota_login.h"
#include "iota_datatrans.h"
#include "cJSON.h"
#include "iota_error_type.h"

/* if you want to use syslog,you should do this:
 *
 * #include "syslog.h"
 * #define _SYS_LOG
 *
 * */

static char *g_workPath = ".";
static char *g_gatewayId = NULL;

static char *g_bootstrapAddress = "iot-bs.cn-north-4.myhuaweicloud.com";

static char *g_serverIp = NULL; // g_serverIp = "iot-mqtts.cn-north-4.myhuaweicloud.com";
static int g_port = 8883;

static char *g_password = "XXXX";

// deviceId，The mqtt protocol requires the user name to be filled in. Here we use deviceId as the username
static char *g_username = "XXXX";

static int g_disconnected = 0;


static int g_connectFailedTimes = 0;

void Test_PropertiesReport(void);

void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleConnectionLost(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleDisConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleDisConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleSubscribesuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleSubscribeFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandlePublishSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandlePublishFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void TimeSleep(int ms);
void MyPrintLog(int level, char *format, va_list args);
void SetAuthConfig(int bootstrapFlag);
void SetMyCallbacks(void);

void TimeSleep(int ms)
{
#if defined(WIN32) || defined(WIN64)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

// -------------------Test  data report---------------------------------

void Test_PropertiesReport(void)
{
    int serviceNum = 2; // reported services' totol count
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];

    // ---------------the data of service1-------------------------------
    char *service1 = "{\"Load\":\"6\",\"ImbA_strVal\":\"7\"}";

    services[0].event_time =
        GetEventTimesStamp(); // if event_time is set to NULL, the time will be the iot-platform's time.
    services[0].service_id = "parameter";
    services[0].properties = service1;

    // ---------------the data of service2--------------------------
    char *service2 = "{\"PhV_phsA\":\"9\",\"PhV_phsB\":\"8\"}";

    services[1].event_time = NULL;
    services[1].service_id = "analog";
    services[1].properties = service2;

    int messageId = IOTA_PropertiesReport(services, serviceNum, 0, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: Test_PropertiesReport() failed, messageId %d\n", messageId);
    }

    MemFree(&services[0].event_time);
}

void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "bootstrap_demo: handleConnectSuccess(), login success\n");
    g_disconnected = 0;
    g_connectFailedTimes = 0;
}

void HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: HandleConnectFailure() error, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
    // judge if the network is available etc. and login again
    // ...
    PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: HandleConnectFailure() login again\n");
    g_connectFailedTimes++;
    if (g_connectFailedTimes < 10) {
        TimeSleep(50);
    } else if (g_connectFailedTimes < 50) {
        TimeSleep(2500);
    }

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: HandleAuthFailure() error, login again failed, result %d\n",
            ret);
    }
}

void HandleConnectionLost(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: HandleConnectionLost() error, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
    // judge if the network is available etc. and login again
    // ...

    if (g_connectFailedTimes < 10) {
        TimeSleep(50);
    } else if (g_connectFailedTimes < 50) {
        TimeSleep(2500);
    }

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: HandleConnectionLost() error, login again failed, result %d\n",
            ret);
    }
}

void HandleDisConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    g_disconnected = 1;

    printf("bootstrap_demo: handleLogoutSuccess, login again\n");
    printf("bootstrap_demo: HandleDisConnectSuccess(), messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

void HandleDisConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "bootstrap_demo: HandleDisConnectFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

void HandleSubscribesuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "bootstrap_demo: HandleSubscribesuccess() messageId %d\n",
        rsp->mqtt_msg_info->messageId);
}

void HandleSubscribeFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "bootstrap_demo: HandleSubscribeFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

void HandlePublishSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "bootstrap_demo: HandlePublishSuccess() messageId %d\n",
        rsp->mqtt_msg_info->messageId);
}

void HandlePublishFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "bootstrap_demo: HandlePublishFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

// ------------handle  message arrived---------

void HandleBootstrap(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "bootstrap_demo: HandleBootstrap(), address is %s\n", rsp->message);
    if (g_serverIp != NULL) {
        MemFree(&g_serverIp);
    }

    int address_length = GetSubStrIndex(rsp->message, ":") + 1;
    if (CopyStrValue(&g_serverIp, (const char *)rsp->message, address_length - 1) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "HandleBootstrap(): there is not enough memory here.\n");
    }
}

// ------------------------------------------------------

void SetAuthConfig(int bootstrapFlag)
{
    if (bootstrapFlag == 0) {
        IOTA_ConfigSetStr(EN_IOTA_CFG_MQTT_ADDR, g_serverIp);
    } else {
        IOTA_ConfigSetStr(EN_IOTA_CFG_MQTT_ADDR, g_bootstrapAddress);
    }
    IOTA_ConfigSetUint(EN_IOTA_CFG_MQTT_PORT, g_port);
    IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICEID, g_username);
    IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICESECRET, g_password);
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);
    /* *
     * Configuration is required in certificate mode:
     * IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_CERT);
     * IOTA_ConfigSetUint(EN_IOTA_CFG_PRIVATE_KEY_PASSWORD, "yourPassword");
     *
     */

    /* *
     * Configuration is required in bootstrap self register mode:
     * IOTA_ConfigSetUint(EN_IOTA_CFG_BS_MODE, EN_IOTA_CFG_BS_SELF_REG);
     * IOTA_ConfigSetStr(EN_IOTA_CFG_BS_SCOPE_ID, scopeId);
     *
     */

#ifdef _SYS_LOG
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LOCAL_NUMBER, LOG_LOCAL7);
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LEVEL, LOG_INFO);
#endif
}

void SetMyCallbacks(void)
{
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_SUCCESS, HandleConnectSuccess);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_FAILURE, HandleConnectFailure);

    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_DISCONNECT_SUCCESS, HandleDisConnectSuccess);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_DISCONNECT_FAILURE, HandleDisConnectFailure);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECTION_LOST, HandleConnectionLost);

    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_SUBSCRIBE_SUCCESS, HandleSubscribesuccess);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_SUBSCRIBE_FAILURE, HandleSubscribeFailure);

    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_PUBLISH_SUCCESS, HandlePublishSuccess);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_PUBLISH_FAILURE, HandlePublishFailure);

    IOTA_SetBootstrapCallback(HandleBootstrap);
}

void MyPrintLog(int level, char *format, va_list args)
{
    vprintf(format, args);
    /*
     * if you want to printf log in system log files,you can do this:
     * vsyslog(level, format, args);
     */
}

int main(int argc, char **argv)
{
#if defined(_DEBUG)
    // in order to make the console log printed immediately in debug mode
    (void)setvbuf(stdout, NULL, _IONBF, 0);
#endif

    IOTA_SetPrintLogCallback(MyPrintLog);
    PrintfLog(EN_LOG_LEVEL_INFO, "bootstrap_demo: start test ===================>\n");

    if (IOTA_Init(g_workPath) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: IOTA_Init() error, init failed\n");
        return 1;
    }

    SetAuthConfig(1);
    SetMyCallbacks();

    // see handleLoginSuccess and handleLoginFailure for login result
    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: IOTA_Connect() error, Auth failed, result %d\n", ret);
    }

    TimeSleep(1500);

    // subscribe boostrap topic
    IOTA_SubscribeBoostrap();

    // ensure the boostrap topic is subscribed, then start to IOTA_Bootstrap.
    TimeSleep(3000);

    if (IOTA_Bootstrap() < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: IOTA_Bootstrap() error \n");
    };

    printf("----------------------------------------------------------------\n");

    // ensure receiving the bootstrap response successfully
    TimeSleep(5500);

    IOTA_Destroy();

    if (IOTA_Init(g_workPath) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: IOTA_Init() error, init failed\n");
        return 1;
    }

    SetAuthConfig(0);
    IOTA_ConfigSetUint(EN_IOTA_CFG_BS_MODE, EN_IOTA_CFG_BS_REG);
    SetMyCallbacks();

    // see handleLoginSuccess and handleLoginFailure for login result
    int ret2 = IOTA_Connect();
    if (ret2 != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: IOTA_Connect() error, Auth failed, result %d\n", ret2);
    }

    TimeSleep(2500);

    Test_PropertiesReport();

    return 0;
}
