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

/*
 * MQTT5.0 access example
 * To use the MQTT5.0 protocol (default to MQTT3.1.1)
 * Need to set MQTTV5 := 1 in Makefile
 */
 
// You can get the access address from IoT Console "Overview" -> "Access Information"
char *g_address = "XXXX"; 
char *g_port = "8883";

// deviceId, The mqtt protocol requires the user name to be filled in.
// Please fill in the deviceId
char *g_deviceId = "XXXX"; 
char *g_password = "XXXX";
#if defined(MQTTV5)
// v5 contnt type && message up
static void Test_ContentType()
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "content", NULL};

    massv5.contnt_type = "application/json";
    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "mqttV5_test: Test_ContentType() failed, messageId %d\n", messageId);
    }
}

// v5 topic alias
static void Test_TopicAlias()
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    massv5.topic_alias = 1;

    ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "content", NULL};
    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "mqttV5_test: Test_TopicAlias() failed, messageId %d\n", messageId);
    }
    TimeSleep(1500);
    
    messageId = IOTA_RawTopicMessageReportV5("", "content", 1, NULL, &massv5);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "mqttV5_test: Test_TopicAlias() failed, messageId %d\n", messageId);
    }
}

// v5 User Property && message up
static void Test_UserProperty()
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "content", NULL};

    MQTTV5_USER_PRO userProperty1;
    MQTTV5_USER_PRO userProperty0;

    userProperty0.key = "region";
    userProperty0.Value = "A";
    userProperty0.nex = &userProperty1;

    userProperty1.key = "type";
    userProperty1.Value = "JSON";
    userProperty1.nex = NULL;

    massv5.properties = &userProperty0;
    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "mqttV5_test: Test_UserProperty() failed, messageId %d\n", messageId);
    }
}

// v5 message report example
static void Test_MessageReportV5()
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "content", NULL};

    // MQTTV5 publish
    MQTTV5_USER_PRO userProperty0;
    MQTTV5_USER_PRO userProperty1;
    userProperty0.key = "name";
    userProperty0.Value = "B";
    userProperty0.nex = &userProperty1;

    userProperty1.key = "type";
    userProperty1.Value = "JSON";
    userProperty1.nex = NULL;

    massv5.properties = &userProperty0;
    massv5.response_topic = "responseTopic";
    massv5.correlation_data = "4321";
    massv5.contnt_type = "application/json";

    // default topic
    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    // user topic
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "mqttV5_test: Test_MessageReportV5() failed, messageId %d\n", messageId);
    }
}

// v5 Properties Report Example
static void Test_PropertiesReportV5()
{
    int serviceNum = 2; // reported services' total count
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];
    MQTTV5_DATA massv5 = mqttv5_initializer;
    MQTTV5_USER_PRO userProperty0;
    MQTTV5_USER_PRO userProperty1;
    // ---------------the data of service1-------------------------------
    char *service1 = "{\"Load\":\"6\",\"ImbA_strVal\":\"7\"}";

    services[0].event_time =
        GetEventTimesStamp(); // if event_time is set to NULL, the time will be the iot-platform's time.
    services[0].service_id = "parameter";
    services[0].properties = service1;

    // ---------------the data of service2-------------------------------
    char *service2 = "{\"PhV_phsA\":\"9\",\"PhV_phsB\":\"8\"}";

    services[1].event_time = NULL;
    services[1].service_id = "analog";
    services[1].properties = service2;

    // -------------- MQTTV5 publish -----------------------------------
    userProperty0.key = "region";
    userProperty0.Value = "A";
    userProperty0.nex = &userProperty1;

    userProperty1.key = "type";
    userProperty1.Value = "JSON";
    userProperty1.nex = NULL;

    massv5.properties = &userProperty0;
    massv5.response_topic = "responseTopic";
    massv5.correlation_data = "1";
    massv5.contnt_type = "application/json";

    int messageId = IOTA_PropertiesReportV5(services, serviceNum, 0, NULL, &massv5);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "mqttV5_test: Test_PropertiesReportV5() failed, messageId %d\n", messageId);
    }

    MemFree(&services[0].event_time);
}
#endif

static void HandleMessageDown(EN_IOTA_MESSAGE *rsp, void *mqttv5)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "mqttV5_test: %s(), content: %s\n", __FUNCTION__,  rsp->content);
    PrintfLog(EN_LOG_LEVEL_INFO, "mqttV5_test: %s(), id: %s\n", __FUNCTION__, rsp->id);
    PrintfLog(EN_LOG_LEVEL_INFO, "mqttV5_test: %s(), name: %s\n", __FUNCTION__,  rsp->name);
    PrintfLog(EN_LOG_LEVEL_INFO, "mqttV5_test: %s(), objectDeviceId: %s\n", __FUNCTION__, rsp->object_device_id);
}

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

static void mqttDeviceSecretInit(char *address, char *port, char *deviceId, char *password) {

    IOTA_Init("."); // The certificate address is ./conf/rootcert.pem
    IOTA_SetPrintLogCallback(MyPrintLog);
 
    // MQTT protocol when the connection parameter is 1883; MQTTS protocol when the connection parameter is 8883
    IOTA_ConnectConfigSet(address, port, deviceId, password);
    // Set authentication method to secret authentication
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);
    
    // Set connection callback function
    IOTA_DefaultCallbackInit();
}

int main(int argc, char **argv) {

    // secret authentication initialization
    mqttDeviceSecretInit(g_address, g_port, g_deviceId, g_password); 

    // message down callback function
    IOTA_SetMessageCallback(HandleMessageDown);
    
    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "mqttV5_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    TimeSleep(5000);

#if defined(MQTTV5)
    Test_ContentType(); // MQTT5.0 
    TimeSleep(1500);

    Test_UserProperty();
    TimeSleep(1500);

    Test_MessageReportV5();
    TimeSleep(1500);

    Test_PropertiesReportV5();
    TimeSleep(1500);

    Test_TopicAlias();
    TimeSleep(1500);
#endif
    
    TimeSleep(5000);

    IOTA_Destroy();
}