/*
 * Copyright (c) 2020-2024 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#include <stdbool.h>
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
#include "mqttv5_util.h"
#include "securec.h"
#include "subscribe.h"
#ifdef SSH_SWITCH
#include "wss_client.h"
#endif

#ifdef SOFT_BUS_OPTION2
#include "soft_bus_datatrans.h"
#include "dconncaseone_interface.h"
#endif
/*
 * if you want to use syslog,you should do this:
 * #include "syslog.h"
 * #define _SYS_LOG
 */

static char *g_workPath = ".";

// You can get the access address from IoT Console "Overview" -> "Access Information"
static char *g_serverIp = "接入地址";
static int g_port = 8883;

// deviceId, The mqtt protocol requires the user name to be filled in.
// Here we use deviceId as the username
static char *g_username = "设备id";
static char *g_password = "设备密钥";

// for batch properties report
static char *g_subDeviceId = "XXXX";

// for upload file, change to your file
static char *g_uploadFilePath = NULL;

/* 
 * If you use #define CUSTOM_RECONNECT_SWITCH , enable customized reconnection example
 * it demostrates how to use IOTA_IsConnected() to check conneciton state and reconnect outside
 * the callback of EN_IOTA_CALLBACK_CONNECT_SUCCESS and EN_IOTA_CALLBACK_CONNECT_FAILURE.
 * Note: the change is only a sample, for reference only
 */
#ifndef CUSTOM_RECONNECT_SWITCH
static int g_connectFailedTimes = 0;
#endif

static void Test_MessageReport(void);
static void Test_PropertiesReport(void);
static void Test_BatchPropertiesReport(char *deviceId);
static void Test_CommandResponse(char *requestId);
static void Test_PropSetResponse(char *requestId);
static void Test_PropGetResponse(char *requestId);
static void Test_ReportOTAVersion(EN_IOTA_EVENT *message);
static void Test_ReportUpgradeStatus(int i, char *version, char *object_device_id);
static void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
static void HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
static void HandleConnectionLost(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
static void HandleDisConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
static void HandleDisConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
static void HandleSubscribesuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
static void HandleSubscribeFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
static void HandlePublishSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
static void HandlePublishFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
static void HandleMessageDown(EN_IOTA_MESSAGE *rsp, void *mqttv5);
static void HandleRawMessageDown(EN_IOTA_RAW_MESSAGE *rsp, void *mqttv5);
static void HandleM2mMessageDown(EN_IOTA_M2M_MESSAGE *rsp);
static void HandleUserTopicMessageDown(EN_IOTA_USER_TOPIC_MESSAGE *rsp);
static void HandleUserTopicRawMessageDown(EN_IOTA_USER_TOPIC_RAW_MESSAGE *rsp);
static void HandleCommandRequest(EN_IOTA_COMMAND *command);
static void HandlePropertiesSet(EN_IOTA_PROPERTY_SET *rsp);
static void TimeSleep(int ms);
static void HandlePropertiesGet(EN_IOTA_PROPERTY_GET *rsp);
static void HandleDeviceShadowRsp(EN_IOTA_DEVICE_SHADOW *rsp);
static void HandleEventsDown(EN_IOTA_EVENT *message);
static void MyPrintLog(int level, char *format, va_list args);
static void SetAuthConfig(void);
static void SetMyCallbacks(void);
static void Test_ReportJson(void);
static void Test_ReportBinary(void);
static void Test_CmdRspV3(void);
static void Test_UpdateSubDeviceStatus(char *deviceId);
static void Test_GtwAddSubDevice(void);
static void Test_GtwDelSubDevice(void);
static void Test_ReportDeviceInfo(void);

#if defined(MQTTV5)
static void Test_CorreaytionData(void);
static void Test_ContentType(void);
static void Test_UserProperty(void);
static void Test_MessageReportV5(void);
static void Test_PropertiesReportV5(void);
#endif

void TimeSleep(int ms)
{
#if defined(WIN32) || defined(WIN64)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}
// ----------------- Test MQTT5.0 ------------------------
#if defined(MQTTV5)

// V5 correlation data and response topic
static void Test_CorreaytionData()
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    char *responseTopicParas = "test";
    ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "content", NULL};
    char *topic = CombineStrings(4, "$oc/devices/", g_username, "/user/", responseTopicParas);
    char num[5] = "1234";

    // MessageReport one, Topic = A, response_topic = B
    PrintfLog(EN_LOG_LEVEL_DEBUG, "Test_CorreaytionData()\n");
    massv5.response_topic = topic;
    massv5.correlation_data = num;
    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_CorreaytionData() failed, messageId %d\n", messageId);
    }

    // MessageReport two, Topic = B, response_topic = NULL
    massv5.response_topic = NULL;
    massv5.correlation_data = num;
    mass.topicParas = responseTopicParas;
    messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_CorreaytionData() failed, messageId %d\n", messageId);
    }
    MemFree(&topic);
}

// v5 contnt type
static void Test_ContentType()
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "content", NULL};

    PrintfLog(EN_LOG_LEVEL_DEBUG, "Test_ContentType()\n");
    massv5.contnt_type = "application/json";

    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ContentType() failed, messageId %d\n", messageId);
    }
}

// v5 User Property
static void Test_UserProperty()
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "content", NULL};

    MQTTV5_USER_PRO userProperty0;
    MQTTV5_USER_PRO userProperty1;

    PrintfLog(EN_LOG_LEVEL_DEBUG, "Test_UserProperty()\n");
    userProperty0.key = "region";
    userProperty0.Value = "A";
    userProperty0.nex = &userProperty1;

    userProperty1.key = "type";
    userProperty1.Value = "JSON";
    userProperty1.nex = NULL;

    massv5.properties = &userProperty0;
    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_UserProperty() failed, messageId %d\n", messageId);
    }
}

// v5 message report
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
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_MessageReportV5() failed, messageId %d\n", messageId);
    }
}

// v5 Properties Report
static void Test_PropertiesReportV5()
{
    int serviceNum = 2; // reported services' totol count
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];
    MQTTV5_DATA massv5 = mqttv5_initializer;
    MQTTV5_USER_PRO userProperty0;
    MQTTV5_USER_PRO userProperty1;
    // --------------- the data of service1 -------------------------------
    char *service1 = "{\"Load\":\"6\",\"ImbA_strVal\":\"7\"}";

    services[0].event_time =
        GetEventTimesStamp(); // if event_time is set to NULL, the time will be the iot-platform's time.
    services[0].service_id = "parameter";
    services[0].properties = service1;

    // --------------- the data of service2 -------------------------------
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
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesReportV5() failed, messageId %d\n", messageId);
    }

    MemFree(&services[0].event_time);
}
#endif

// --------------------------- Test data report ------------------------------------
static void Test_MessageReport(void)
{
    // default topic
    ST_IOTA_MESS_REP_INFO mass = {NULL, "name", "id", "content", NULL};
    int messageId = IOTA_MessageDataReport(mass, NULL);
    /* // user topic
    * mass.topicParas = "devMsg";
    * int messageId = IOTA_MessageDatReport(mass, NULL);
    */
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_MessageReport() failed, messageId %d\n", messageId);
    }
}

// V3 report
static void Test_ReportJson(void)
{
    const int serviceNum = 2; // reported services' totol count
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];

    // --------------- the data of service1 -------------------------------
    char *service1 = "{\"Load\":\"6\",\"ImbA_strVal\":\"7\"}";

    services[0].event_time =
        GetEventTimesStamp(); // if event_time is set to NULL, the time will be the iot-platform's time.
    services[0].service_id = "LTE";
    services[0].properties = service1;

    // --------------- the data of service2 -------------------------------
    char *service2 = "{\"PhV_phsA\":\"9\",\"PhV_phsB\":\"8\"}";

    services[1].event_time = NULL;
    services[1].service_id = "CPU";
    services[1].properties = service2;

    int messageId = IOTA_PropertiesReportV3(services, serviceNum, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportJson() failed, messageId %d\n", messageId);
    }

    MemFree(&services[0].event_time);
}

// V3 report
static void Test_ReportBinary(void)
{
    int messageId = IOTA_BinaryReportV3("1234567890", NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportBinary() failed, messageId %d\n", messageId);
    }
}

// V3 report
static void Test_CmdRspV3(void)
{
    ST_IOTA_COMMAND_RSP_V3 *rsp = (ST_IOTA_COMMAND_RSP_V3 *)malloc(sizeof(ST_IOTA_COMMAND_RSP_V3));
    if (rsp == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_CmdRspV3() error, Memory allocation failure\n");
        return;
    }
    rsp->body = "{\"result\":\"0\"}";
    rsp->errcode = 0;
    rsp->mid = 1;

    int messageId = IOTA_CmdRspV3(rsp, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_CmdRspV3() failed, messageId %d\n", messageId);
    }
    MemFree(&rsp);
}

/*
 * If you cancel #define STORE_DATA_SWITCH comments , enable offline caching¡£
 * Note: the change is only a sample, for reference only
 * Currently, the container for storing exception messages is a dynamic two-dimensional array,
 * which can be selected by users according to their business logic
 * #define STORE_DATA_SWITCH
*/

/*
 * Open a space and store data, and report data at the same time
 * It is recommended to store the data of the sensor in the container
 * before reporting the data each time. Only after receiving
 * the callback of onpublishsuccess can the message be removed
 * from the container.
 */
#if defined(STORE_DATA_SWITCH)
#define CACHE_SPACE_MAX 100

typedef struct {
    ST_IOTA_SERVICE_DATA_INFO * services;
    int serviceNum;
}   ServiceCacheSpace;

static ServiceCacheSpace g_cacheSpace[CACHE_SPACE_MAX] = {0};
static int g_cacheSpaceLen = 0;
static int g_cachePosition = 0;

static void Test_PropertiesStoreData(void)
{
    int serviceNum = 2; // reported services' totol count
    int i = 0;

    ST_IOTA_SERVICE_DATA_INFO *services =
        (ST_IOTA_SERVICE_DATA_INFO *)malloc(sizeof(ST_IOTA_SERVICE_DATA_INFO) * serviceNum);
    if (services == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesStoreData()  malloc failed! ");
        return;
    }
    // --------------- the data of service1 -------------------------------
    char *service1 = "{\"Load\":\"6\",\"ImbA_strVal\":\"7\"}";

    services->event_time =
        GetEventTimesStamp(); // if event_time is set to NULL, the time will be the iot-platform's time.
    services->service_id = "parameter";
    services->properties = service1;

    // --------------- the data of service2 -------------------------------
    char *service2 = "{\"PhV_phsA\":\"9\",\"PhV_phsB\":\"8\"}";

    (services + 1)->event_time = NULL;
    (services + 1)->service_id = "analog";
    (services + 1)->properties = service2;

    // Message store
    if (g_cacheSpaceLen > CACHE_SPACE_MAX) {
        MemFree(&(g_cacheSpace[g_cachePosition].services));
        g_cacheSpace[g_cachePosition].services = services;
        g_cacheSpace[g_cachePosition].serviceNum = serviceNum;
        g_cachePosition++;
    } else {
        g_cachePosition = 0;
        for (i = 0; i < CACHE_SPACE_MAX; i++) {
            if (g_cacheSpace[i].services == NULL) {
                g_cacheSpace[i].services = services;
                g_cacheSpace[i].serviceNum = serviceNum;
                g_cacheSpaceLen++;
                break;
            }
        }
    }
    // Report data -- Try resending
    int messageId = IOTA_PropertiesReport(services, serviceNum, 0, (void *)services);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesReport() failed, messageId %d\n", messageId);
    }
}
#endif

static void Test_PropertiesReport(void)
{
    const int serviceNum = 2; // reported services' totol count
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];

    // --------------- the data of service1 -------------------------------
    char *service1 = "{\"Load\":\"6\",\"ImbA_strVal\":\"7\"}";

    services[0].event_time =
        GetEventTimesStamp(); // if event_time is set to NULL, the time will be the iot-platform's time.
    services[0].service_id = "parameter";
    services[0].properties = service1;

    // --------------- the data of service2 -------------------------------
    char *service2 = "{\"PhV_phsA\":\"9\",\"PhV_phsB\":\"8\"}";

    services[1].event_time = NULL;
    services[1].service_id = "analog";
    services[1].properties = service2;

    int messageId = IOTA_PropertiesReport(services, serviceNum, 0, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesReport() failed, messageId %d\n", messageId);
    }

    MemFree(&services[0].event_time);
}

static void Test_BatchPropertiesReport(char *deviceId)
{
    const int deviceNum = 1;                     // the number of sub devices
    ST_IOTA_DEVICE_DATA_INFO devices[deviceNum]; // Array of structures to be reported by sub devices
    int serviceList[deviceNum];       // Corresponding to the number of services to be reported for each sub device
    serviceList[0] = 2;               // device 1 reports two services
    // serviceList[1] = 1;            // device 2 reports one service

    char *device1_service1 = "{\"Load\":\"1\",\"ImbA_strVal\":\"3\"}"; // must be json

    char *device1_service2 = "{\"PhV_phsA\":\"2\",\"PhV_phsB\":\"4\"}"; // must be json

    devices[0].device_id = deviceId;
    devices[0].services[0].event_time = GetEventTimesStamp();
    devices[0].services[0].service_id = "parameter";
    devices[0].services[0].properties = device1_service1;

    devices[0].services[1].event_time = GetEventTimesStamp();
    devices[0].services[1].service_id = "analog";
    devices[0].services[1].properties = device1_service2;
    /* the demo serivce 1
     * char *device2_service1 = "{\"AA\":\"2\",\"BB\":\"4\"}";
     * devices[1].device_id = "subDevices22222";
     * devices[1].services[0].event_time = "d2s1";
     * devices[1].services[0].service_id = "device2_service11111111";
     * devices[1].services[0].properties = device2_service1;
    */
    int messageId = IOTA_BatchPropertiesReport(devices, deviceNum, serviceList, 0, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_BatchPropertiesReport() failed, messageId %d\n", messageId);
    }

    MemFree(&devices[0].services[0].event_time);
    MemFree(&devices[0].services[1].event_time);
}

static void Test_UpdateSubDeviceStatus(char *deviceId)
{
    int deviceNum = 1;
    ST_IOTA_DEVICE_STATUSES device_statuses;
    device_statuses.event_time = GetEventTimesStamp();
    device_statuses.device_statuses[0].device_id = deviceId;
    device_statuses.device_statuses[0].status = ONLINE;
    int messageId = IOTA_UpdateSubDeviceStatus(&device_statuses, deviceNum, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_UpdateSubDeviceStatus() failed, messageId %d\n", messageId);
    }
    MemFree(&device_statuses.event_time);
}

static void Test_CommandResponse(char *requestId)
{
    char *commandResponse = "{\"SupWh\": \"aaa\"}"; // in service accumulator

    int result_code = 0;
    char *response_name = "cmdResponses";

    int messageId = IOTA_CommandResponse(requestId, result_code, response_name, commandResponse, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_CommandResponse() failed, messageId %d\n", messageId);
    }
}

static void Test_PropSetResponse(char *requestId)
{
    int messageId = IOTA_PropertiesSetResponse(requestId, 0, "success", NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropSetResponse() failed, messageId %d\n", messageId);
    }
}

static void Test_PropGetResponse(char *requestId)
{
    const int serviceNum = 2;
    ST_IOTA_SERVICE_DATA_INFO serviceProp[serviceNum];

    char *property = "{\"Load\":\"5\",\"ImbA_strVal\":\"6\"}";

    serviceProp[0].event_time = GetEventTimesStamp();
    serviceProp[0].service_id = "parameter";
    serviceProp[0].properties = property;

    char *property2 = "{\"PhV_phsA\":\"2\",\"PhV_phsB\":\"4\"}";

    serviceProp[1].event_time = GetEventTimesStamp();
    serviceProp[1].service_id = "analog";
    serviceProp[1].properties = property2;

    int messageId = IOTA_PropertiesGetResponse(requestId, serviceProp, serviceNum, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropGetResponse() failed, messageId %d\n", messageId);
    }

    MemFree(&serviceProp[0].event_time);
    MemFree(&serviceProp[1].event_time);
}

static void Test_ReportOTAVersion(EN_IOTA_EVENT *message)
{
    ST_IOTA_OTA_VERSION_INFO otaVersion;

    otaVersion.event_time = NULL;
    otaVersion.sw_version = "v1.0";
    otaVersion.fw_version = "v1.0";
    otaVersion.object_device_id = message->object_device_id;

    int messageId = IOTA_OTAVersionReport(otaVersion, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportOTAVersion() failed, messageId %d\n", messageId);
    }
}

static void Test_ReportUpgradeStatus(int i, char *version, char *object_device_id)
{
    ST_IOTA_UPGRADE_STATUS_INFO statusInfo;
    if (i == 0) {
        statusInfo.description = "success";
        statusInfo.progress = 100;
        statusInfo.result_code = 0;
        statusInfo.version = version;
    } else {
        statusInfo.description = "failed";
        statusInfo.result_code = 1;
        statusInfo.progress = 0;
        statusInfo.version = version;
    }

    statusInfo.event_time = NULL;
    statusInfo.object_device_id = object_device_id;

    int messageId = IOTA_OTAStatusReport(statusInfo, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportUpgradeStatus() failed, messageId %d\n", messageId);
    }
}

void Test_GetUploadFileUrl(void)
{
    if (g_uploadFilePath) {
        ST_IOTA_UPLOAD_FILE uploadFile = {0};
        uploadFile.file_name = g_uploadFilePath;
        IOTA_GetUploadFileUrl(&uploadFile, NULL);
    }
}

static void Test_GtwAddSubDevice(void)
{
    ST_IOTA_SUB_DEVICE_INFO subDeviceInfos;
    int deviceNum = 2;

    subDeviceInfos.deviceInfo[0].description = "description";
    subDeviceInfos.deviceInfo[0].device_id = "device_id123";
    subDeviceInfos.deviceInfo[0].extension_info = NULL;
    subDeviceInfos.deviceInfo[0].name = "sub_device111";
    subDeviceInfos.deviceInfo[0].node_id = "node_id123";
    subDeviceInfos.deviceInfo[0].parent_device_id = NULL;
    subDeviceInfos.deviceInfo[0].product_id = "your_product_id"; // Please change the product ID of the sub device

    subDeviceInfos.deviceInfo[1].description = "description";
    subDeviceInfos.deviceInfo[1].device_id = "device_id1234";
    subDeviceInfos.deviceInfo[1].extension_info = NULL;
    subDeviceInfos.deviceInfo[1].name = "sub_device222";
    subDeviceInfos.deviceInfo[1].node_id = "node_id123";
    subDeviceInfos.deviceInfo[1].parent_device_id = NULL;
    subDeviceInfos.deviceInfo[1].product_id = "your_product_id"; // Please change the product ID of the sub device

    subDeviceInfos.event_id = "123123";
    subDeviceInfos.event_time = NULL;

    int messageId = IOTA_AddSubDevice(&subDeviceInfos, deviceNum, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_GtwAddSubDevice() failed, messageId %d\n", messageId);
    }
}

static void Test_GtwDelSubDevice(void)
{
    ST_IOTA_DEL_SUB_DEVICE delSubDevices;
    int deviceNum = 3;

    delSubDevices.event_id = NULL;
    delSubDevices.event_time = NULL;
    delSubDevices.delSubDevice[0] = "device_id123";
    delSubDevices.delSubDevice[1] = "device_id1234";
    delSubDevices.delSubDevice[2] = "device_id12345";

    int messageId = IOTA_DelSubDevice(&delSubDevices, deviceNum, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_GtwDelSubDevice() failed, messageId %d\n", messageId);
    }
}

static void Test_ReportDeviceInfo(void)
{
    ST_IOTA_DEVICE_INFO_REPORT deviceInfo;

    deviceInfo.device_sdk_version = SDK_VERSION;
    deviceInfo.sw_version = "v1.0";
    deviceInfo.fw_version = "v1.0";
    deviceInfo.event_time = NULL;
    deviceInfo.device_ip = NULL;
    deviceInfo.object_device_id = NULL;

    int messageId = IOTA_ReportDeviceInfo(&deviceInfo, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportDeviceInfo() failed, messageId %d\n", messageId);
    }
}

static void Test_M2MSendMsg(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "device_demo: this is m2m demo\n");
    char *to = "deviceA";
    char *from = g_username;
    char *content = "hello deviceB";
    char *requestId = "demoIdToDeviceB";
    int messageId = IOTA_M2MSendMsg(to, from, content, requestId, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_M2MSendMsg() failed, messageId %d\n", messageId);
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "device_demo: Test_M2MSendMsg() ok, messageId %d\n", messageId);
    }
}

// ---------------------------------------------------------------------------------------------
static void SubscribeDeviceTpoic(void)
{
    /* After successful connection, please subscribe */
    SubscribeAllQos(1);
}

static void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    (void)rsp;
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: handleConnectSuccess(), login success\n");
    SubscribeDeviceTpoic();
#if defined(STORE_DATA_SWITCH)
    int i = 0;
    if (g_cacheSpaceLen <= 0) {
        return;
    }
    // Reconnect -- Send the stored data
    for (i = 0; i < CACHE_SPACE_MAX && g_cacheSpaceLen > 0; i++) {
        if (g_cacheSpace[i].services != NULL) {
            int serviceNum = g_cacheSpace[i].serviceNum;
            int messageId = IOTA_PropertiesReport(g_cacheSpace[i].services, serviceNum, 0, (void *)g_cacheSpace[i].services);
            if (messageId != 0) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesReport() failed, messageId %d\n", messageId);
            }
            TimeSleep(100);
        }
    }
#endif
}

static void HandleConnectionBroken(const char *handleName, const EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: %s() error, messageId %d, code %d, messsage %s\n", handleName,
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);

#ifndef CUSTOM_RECONNECT_SWITCH
    // judge if the network is available etc. and login again
    // ...
    PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: %s() login again\n", handleName);

    g_connectFailedTimes++;
    if (g_connectFailedTimes < 10) {
        TimeSleep(5000);
    } else {
        TimeSleep(25000);
    }

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: HandleAuthFailure() error, login again failed, result %d\n", ret);
    }
#endif
}

static void HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    HandleConnectionBroken("HandleConnectFailure", rsp);
}

static void HandleConnectionLost(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    HandleConnectionBroken("HandleConnectionLost", rsp);
}

static void HandleDisConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    (void)rsp;
}

static void HandleDisConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "device_demo: HandleDisConnectFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

static void HandleSubscribesuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubscribesuccess() messageId %d\n", rsp->mqtt_msg_info->messageId);
}

static void HandleSubscribeFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "device_demo: HandleSubscribeFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

static void HandlePublishSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePublishSuccess() messageId %d\n", rsp->mqtt_msg_info->messageId);

#if defined(STORE_DATA_SWITCH)
    int i = 0;
    if (rsp->mqtt_msg_info->context == NULL) {
        return;
    }
    for (i = 0; i < CACHE_SPACE_MAX && g_cacheSpaceLen > 0; i++) {
        if (g_cacheSpace[i].services == rsp->mqtt_msg_info->context) {
            MemFree(&(g_cacheSpace[i].services));
            g_cacheSpace[i].services = NULL;
            g_cacheSpace[i].serviceNum = 0;
            g_cacheSpaceLen--;
            break;
        } else if (g_cacheSpace[i].services != NULL) { // 重传
            int serviceNum = g_cacheSpace[i].serviceNum;
            int messageId = IOTA_PropertiesReport(g_cacheSpace[i].services, serviceNum, 0, (void *)g_cacheSpace[i].services);
            if (messageId != 0) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesReport() failed, messageId %d\n", messageId);
            }
            TimeSleep(100);
        }
    }
#endif
}

static void HandlePublishFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING, "device_demo: HandlePublishFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

// ------------------- handle  message arrived -------------------------------
static bool GetMessageInSystemFormat(cJSON *root, char **objectDeviceId, char **name, char **id, char **content)
{
    if (!cJSON_IsObject(root)) {
        return false;
    }

    // check if any key is not belong to that in system format
    cJSON *kvInMessage = root->child;
    while (kvInMessage) {
        if ((strcmp(kvInMessage->string, CONTENT) != 0) && (strcmp(kvInMessage->string, OBJECT_DEVICE_ID) != 0) &&
            (strcmp(kvInMessage->string, NAME) != 0) && (strcmp(kvInMessage->string, ID) != 0)) {
            return false;
        }
        kvInMessage = kvInMessage->next;
    }

    // check if value is complying to system format
    cJSON *contentObject = cJSON_GetObjectItem(root, CONTENT);
    cJSON *objectDeviceIdObject = cJSON_GetObjectItem(root, OBJECT_DEVICE_ID);
    cJSON *nameObject = cJSON_GetObjectItem(root, NAME);
    cJSON *idObject = cJSON_GetObjectItem(root, ID);
    if ((objectDeviceIdObject && !cJSON_IsNull(objectDeviceIdObject) && !cJSON_IsString(objectDeviceIdObject)) ||
        (nameObject && !cJSON_IsNull(nameObject) && !cJSON_IsString(nameObject)) ||
        (idObject && !cJSON_IsNull(idObject) && !cJSON_IsString(idObject)) ||
        (contentObject && !cJSON_IsNull(contentObject) && !cJSON_IsString(contentObject))) {
        return false; // is not system format
    }

    *objectDeviceId = cJSON_GetStringValue(objectDeviceIdObject);
    *name = cJSON_GetStringValue(nameObject);
    *id = cJSON_GetStringValue(idObject);
    *content = cJSON_GetStringValue(contentObject);
    return true;
}

static void ProcessMQTT5Data(const char *functionName, void *mqttv5)
{
#if defined(MQTTV5)
    MQTTV5_DATA *mqtt = (MQTTV5_DATA *)mqttv5;
    MQTTV5_USER_PRO *user_pro = mqtt->properties;
    if (mqtt->contnt_type != NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(),contnt_type: %s\n", functionName, mqtt->contnt_type);
    }
    if (user_pro != NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(),properties is:\n", functionName);
        while (user_pro != NULL) {
            PrintfLog(EN_LOG_LEVEL_INFO, "key = %s, value = %s\n", user_pro->key, user_pro->Value);
            user_pro = (MQTTV5_USER_PRO *)user_pro->nex;
        }
    }
    if (mqtt->correlation_data != NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(),correlation_data: %s\n", mqtt->correlation_data);
    }
    if (mqtt->response_topic != NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(),response_topic: %s\n", functionName, mqtt->response_topic);
    }
    mqttV5_listFree(mqtt->properties);
#else
    (void)functionName;
    (void)mqttv5;
#endif
}

static void ProcessMessageData(const char *functionName,
    const char *content, const char *id, const char *name, const char *objectDeviceId)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(), content: %s\n", functionName, content);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(), id: %s\n", functionName, id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(), name: %s\n", functionName, name);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(), objectDeviceId: %s\n", functionName, objectDeviceId);
}

static void ProcessRawMessageData(const char *functionName, const char *payload, int payloadLength)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(), payload length: %d\n", functionName, payloadLength);
    cJSON *root = cJSON_Parse(payload);
    char *objectDeviceId = NULL;
    char *name = NULL;
    char *id = NULL;
    char *content = NULL;
    if (GetMessageInSystemFormat(root, &objectDeviceId, &name, &id, &content)) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(), message is system format\n", functionName);
        ProcessMessageData(functionName, content, id, name, objectDeviceId);
    }
    cJSON_Delete(root);
}

static void HandleMessageDown(EN_IOTA_MESSAGE *rsp, void *mqttv5)
{
    if (rsp == NULL) {
        return;
    }
    ProcessMQTT5Data(__FUNCTION__, mqttv5);
    ProcessMessageData(__FUNCTION__, rsp->content, rsp->id, rsp->name, rsp->object_device_id);
}

static void HandleRawMessageDown(EN_IOTA_RAW_MESSAGE *rsp, void *mqttv5)
{
    if (rsp == NULL) {
        return;
    }
    ProcessMQTT5Data(__FUNCTION__, mqttv5);
    ProcessRawMessageData(__FUNCTION__, rsp->payload, rsp->payloadLength);
}

static void HandleM2mMessageDown(EN_IOTA_M2M_MESSAGE *rsp)
{
    if (rsp == NULL) {
        return;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleM2mMessageDown(), requestId: %s\n", rsp->request_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleM2mMessageDown(), to:        %s\n", rsp->to);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleM2mMessageDown(), from:      %s\n", rsp->from);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleM2mMessageDown(), content:   %s\n", rsp->content);

    // do sth
}

static void HandlePropertiesSet(EN_IOTA_PROPERTY_SET *rsp)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), messageId %d \n", rsp->mqtt_msg_info->messageId);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), request_id %s \n", rsp->request_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), object_device_id %s \n", rsp->object_device_id);

    int i = 0;
    while (rsp->services_count > i) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), service_id %s \n",
            rsp->services[i].service_id);
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), properties %s \n",
            rsp->services[i].properties);
        i++;
    }

    Test_PropSetResponse(rsp->request_id); // response
}

static void HandlePropertiesGet(EN_IOTA_PROPERTY_GET *rsp)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), messageId %d \n", rsp->mqtt_msg_info->messageId);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), request_id %s \n", rsp->request_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), object_device_id %s \n", rsp->object_device_id);

    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), service_id %s \n", rsp->service_id);

    Test_PropGetResponse(rsp->request_id); // response
}

static void HandleDeviceShadowRsp(EN_IOTA_DEVICE_SHADOW *rsp)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), messageId %d \n",
        rsp->mqtt_msg_info->messageId);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), request_id %s \n", rsp->request_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), object_device_id %s \n", rsp->object_device_id);

    int i = 0;
    while (rsp->shadow_data_count > i) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), service_id %s \n",
            rsp->shadow[i].service_id);
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), desired properties %s \n",
            rsp->shadow[i].desired_properties);
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), reported properties %s \n",
            rsp->shadow[i].reported_properties);
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), version    %d \n", rsp->shadow[i].version);
        i++;
    }
}

static void HandleUserTopicMessageDown(EN_IOTA_USER_TOPIC_MESSAGE *rsp)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(), topic_para: %s\n", __FUNCTION__, rsp->topic_para);
    ProcessMessageData(__FUNCTION__, rsp->content, rsp->id, rsp->name, rsp->object_device_id);
}

static void HandletUndefinedMessageDown(EN_IOTA_UNDEFINED_MESSAGE *rsp)
{
   if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "message_test: %s(), content: %s\n", __FUNCTION__,  rsp->payload);
}

static void HandleUserTopicRawMessageDown(EN_IOTA_USER_TOPIC_RAW_MESSAGE *rsp)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: %s(), topic_para: %s\n", __FUNCTION__, rsp->topicPara);
    ProcessRawMessageData(__FUNCTION__, rsp->payload, rsp->payloadLength);
}

static void HandleCommandRequest(EN_IOTA_COMMAND *command)
{
    if (command == NULL) {
        return;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), messageId %d\n",
        command->mqtt_msg_info->messageId);

    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), object_device_id %s\n",
        command->object_device_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), service_id %s\n", command->service_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), command_name %s\n", command->command_name);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), paras %s\n", command->paras);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), request_id %s\n", command->request_id);

    Test_CommandResponse(command->request_id); // response command
}

static void HandleSubDeviceManager(EN_IOTA_EVENT *message, int i)
{
    // if it is the platform inform the gateway to add or delete the sub device
    if ((message->services[i].event_type == EN_IOTA_EVENT_ADD_SUB_DEVICE_NOTIFY) ||
        (message->services[i].event_type == EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY)) {
        int j = 0;
        while (message->services[i].paras->devices_count > 0) {
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), parent_device_id: %s \n",
                message->services[i].paras->devices[j].parent_device_id);
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), device_id: %s \n",
                message->services[i].paras->devices[j].device_id);
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), node_id: %s \n",
                message->services[i].paras->devices[j].node_id);

            // add a sub device
            if (message->services[i].event_type == EN_IOTA_EVENT_ADD_SUB_DEVICE_NOTIFY) {
                PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), name: %s \n",
                    message->services[i].paras->devices[j].name);
                PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), manufacturer_id: %s \n",
                    message->services[i].paras->devices[j].manufacturer_id);
                PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), product_id: %s \n",
                    message->services[i].paras->devices[j].product_id);

                // report status of the sub device
                Test_UpdateSubDeviceStatus(message->services[i].paras->devices[j].device_id);
                // report data of the sub device
                Test_BatchPropertiesReport(message->services[i].paras->devices[j].device_id);
            } else if (message->services[i].event_type == EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY) {
                // delete a sub device
                PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), the sub device is deleted: %s\n",
                    message->services[i].paras->devices[j].device_id);
            }

            j++;
            message->services[i].paras->devices_count--;
        }
    } else if (message->services[i].event_type == EN_IOTA_EVENT_ADD_SUB_DEVICE_RESPONSE) {
        int j = 0;
        while (message->services[i].gtw_add_device_paras->successful_devices_count > 0) {
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), device_id: %s \n",
                message->services[i].gtw_add_device_paras->successful_devices[j].device_id);
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), name: %s \n",
                message->services[i].gtw_add_device_paras->successful_devices[j].name);
            j++;
            message->services[i].gtw_add_device_paras->successful_devices_count--;
        }
        j = 0;
        while (message->services[i].gtw_add_device_paras->failed_devices_count > 0) {
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), error_code: %s \n",
                message->services[i].gtw_add_device_paras->failed_devices[j].error_code);
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), error_msg: %s \n",
                message->services[i].gtw_add_device_paras->failed_devices[j].error_msg);
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), node_id: %s \n",
                message->services[i].gtw_add_device_paras->failed_devices[j].node_id);
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), product_id: %s \n",
                message->services[i].gtw_add_device_paras->failed_devices[j].product_id);

            j++;
            message->services[i].gtw_add_device_paras->failed_devices_count--;
        }
    } else if (message->services[i].event_type == EN_IOTA_EVENT_DEL_SUB_DEVICE_RESPONSE) {
        int j = 0;
        while (message->services[i].gtw_del_device_paras->successful_devices_count > 0) {
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), device_id: %s \n",
                message->services[i].gtw_del_device_paras->successful_devices[j]);
            j++;
            message->services[i].gtw_del_device_paras->successful_devices_count--;
        }
        j = 0;
        while (message->services[i].gtw_del_device_paras->failed_devices_count > 0) {
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), error_code: %s \n",
                message->services[i].gtw_del_device_paras->failed_devices[j].error_code);
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), error_msg: %s \n",
                message->services[i].gtw_del_device_paras->failed_devices[j].error_msg);
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceManager(), device_id: %s \n",
                message->services[i].gtw_del_device_paras->failed_devices[j].device_id);

            j++;
            message->services[i].gtw_del_device_paras->failed_devices_count--;
        }
    }
}

static void HandleEventOta(EN_IOTA_EVENT *message, int i)
{
    if (message->services[i].event_type == EN_IOTA_EVENT_VERSION_QUERY) {
        // report OTA version
        Test_ReportOTAVersion(message);
    }

    if ((message->services[i].event_type == EN_IOTA_EVENT_FIRMWARE_UPGRADE) ||
        (message->services[i].event_type == EN_IOTA_EVENT_SOFTWARE_UPGRADE) ||
        (message->services[i].event_type == EN_IOTA_EVENT_FIRMWARE_UPGRADE_V2) ||
        (message->services[i].event_type == EN_IOTA_EVENT_SOFTWARE_UPGRADE_V2)) {
        // check sha256
        const char *pkg_sha256 = "your package sha256"; // the sha256 value of your ota package
        if (message->services[i].ota_paras->sign != NULL) {
            if (strcmp(pkg_sha256, message->services[i].ota_paras->sign)) { // V1 only
                // report failed status
                Test_ReportUpgradeStatus(-1, message->services[i].ota_paras->version, message->object_device_id);
            }
        }

        char filename[PKGNAME_MAX + 1];
        // start to receive packages and firmware_upgrade or software_upgrade
        if (IOTA_GetOTAPackages_Ext(message->services[i].ota_paras->url, message->services[i].ota_paras->access_token,
            1000, ".", filename) == 0) {
            usleep(3000 * 1000);
            PrintfLog(EN_LOG_LEVEL_DEBUG, "the filename is %s\n", filename);
            // report successful upgrade status
            Test_ReportUpgradeStatus(0, message->services[i].ota_paras->version, message->object_device_id);
        } else {
            // report failed status
            Test_ReportUpgradeStatus(-1, message->services[i].ota_paras->version, message->object_device_id);
        }
    }
}

static void HandleTimeSync(EN_IOTA_EVENT *message, int i)
{
    if (message->services[i].event_type == EN_IOTA_EVENT_GET_TIME_SYNC_RESPONSE) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleTimeSync(), device_real_time: %lld \n",
            message->services[i].ntp_paras->device_real_time);
    }
}

static void HandleDeviceLog(EN_IOTA_EVENT *message, int i)
{
    if (message->services[i].event_type == EN_IOTA_EVENT_LOG_CONFIG) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceLog(), log_switch: %s \n",
            message->services[i].device_log_paras->log_switch);
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceLog(), end_time: %s \n",
            message->services[i].device_log_paras->end_time);
    }
}

#ifdef SSH_SWITCH
static void HandleTunnelMgr(EN_IOTA_EVENT *message, int i)
{
    URL_INFO info = {NULL, NULL, NULL, NULL};
    int ret = 0;

    if (message->services[i].event_type != EN_IOTA_EVENT_TUNNEL_NOTIFY)
        return;

    ret = WssClientSplitUrl(&info, message->services[i].tunnel_mgr_paras->tunnel_url,
        message->services[i].tunnel_mgr_paras->tunnel_access_token);
    if (ret != IOTA_SUCCESS) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "HandleTunnelMgr: Url parse failed.%d\n", ret);
        return;
    }
    WssClientCreate(&info);
    MemFree(&info.path);
    MemFree(&info.port);
    MemFree(&info.site);
    MemFree(&info.token);
}
#endif


static void HandleFileMgr(EN_IOTA_SERVICE_EVENT *service) {
    IOTA_UploadFile(g_uploadFilePath, service->file_mgr_paras->url, NULL);
}

static void HandleEventsDown(EN_IOTA_EVENT *message)
{
    if (message == NULL) {
        return;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), messageId %d\n", message->mqtt_msg_info->messageId);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), services_count %d\n", message->services_count);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), object_device_id %s\n", message->object_device_id);
    int i = 0;
    while (message->services_count > i) {
        // sub device manager
        if (message->services[i].servie_id == EN_IOTA_EVENT_SUB_DEVICE_MANAGER) {
            HandleSubDeviceManager(message, i);
        } else if (message->services[i].servie_id == EN_IOTA_EVENT_OTA) {
            HandleEventOta(message, i);
        } else if (message->services[i].servie_id == EN_IOTA_EVENT_TIME_SYNC) {
            HandleTimeSync(message, i);
        } else if (message->services[i].servie_id == EN_IOTA_EVENT_DEVICE_LOG) {
            HandleDeviceLog(message, i);
        } else if (message->services[i].servie_id == EN_IOTA_EVENT_TUNNEL_MANAGER) {
#ifdef SSH_SWITCH
            HandleTunnelMgr(message, i);
#endif
        } else if (message->services[i].servie_id == EN_IOTA_EVENT_SOFT_BUS) {
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), event_id: %s \n",
                message->services[i].event_id);
            PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), soft_bus_info: %s \n",
                message->services[i].soft_bus_paras->bus_infos);
        } else if (message->services[i].servie_id == EN_IOTA_EVENT_FILE_MANAGER) {
            HandleFileMgr(&(message->services[i]));
        }
        i++;
    }
}

static int HandleDeviceRuleSendMsg(char *deviceId, char *message)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceRuleSendMsg(), deviceId is %s, the message is %s",
        deviceId, message);
    return 0;
}

static int HandleDeviceConfig(JSON *cfg, char *description)
{
    char *cfgstr = cJSON_Print(cfg);
    if (cfgstr) {
        PrintfLog(EN_LOG_LEVEL_INFO, "HandleDeviceConfig config content: %s\n", cfgstr);
    } else {
        PrintfLog(EN_LOG_LEVEL_WARNING, "HandleDeviceConfig config content is null\n");
    }
    (void)strcpy_s(description, MaxDescriptionLen, "update config success");
    MemFree(&cfgstr);
    return 0;
}

static void SetAuthConfig(void)
{
    IOTA_ConfigSetStr(EN_IOTA_CFG_MQTT_ADDR, g_serverIp);
    IOTA_ConfigSetUint(EN_IOTA_CFG_MQTT_PORT, g_port);
    IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICEID, g_username);
    IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICESECRET, g_password);
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);
    IOTA_ConfigSetUint(EN_IOTA_CFG_CHECK_STAMP_METHOD, EN_IOTA_CFG_CHECK_STAMP_OFF);
    /* *
     * Configuration is required in certificate mode:
     *
     * IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_CERT);
     * IOTA_ConfigSetUint(EN_IOTA_CFG_PRIVATE_KEY_PASSWORD, "yourPassword");
     *
     */

#ifdef _SYS_LOG
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LOCAL_NUMBER, LOG_LOCAL7);
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LEVEL, LOG_INFO);
#endif
}
#if defined(SOFT_BUS_OPTION2)
void SendDataToDeviceCb(const char *deviceId, int result)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "Data sent to [%s] result: %d\n", deviceId, result);
}

void ReceiveDataFromDeviceCb(const char *deviceId, char *receiveData, int datelen)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "Data received from device [%s]. Datalen: %d, Data: %s\n", deviceId, datelen,
        receiveData);
}

void Test_PrintfSoftBusInfo()
{
    soft_bus_total *g_soft_bus_total = getSoftBusTotal();
    int total = g_soft_bus_total->count;
    PrintfLog(EN_LOG_LEVEL_INFO, "总共有下发的bus信息个数为 : %d\n", total);
    int i;
    for (i = 0; i < total; i++) {
        PrintfLog(EN_LOG_LEVEL_INFO, "第 %d 个bus信息为: ---------- \n", i);
        char *bus_key = g_soft_bus_total->g_soft_bus_info[i].bus_key;
        PrintfLog(EN_LOG_LEVEL_INFO, "bus_key : %s \n", bus_key);

        int count = g_soft_bus_total->g_soft_bus_info[i].count;
        PrintfLog(EN_LOG_LEVEL_INFO, "设备数count为 : %d \n", count);

        int j;
        for (j = 0; j < count; j++) {
            PrintfLog(EN_LOG_LEVEL_INFO, "第 %d 个设备信息为:---------- \n", j);
            char *device_id = g_soft_bus_total->g_soft_bus_info[i].g_device_soft_bus_info[j].device_id;
            PrintfLog(EN_LOG_LEVEL_INFO, "device_id : %s \n", device_id);
            char *device_ip = g_soft_bus_total->g_soft_bus_info[i].g_device_soft_bus_info[j].device_ip;
            PrintfLog(EN_LOG_LEVEL_INFO, "device_ip : %s \n", device_ip);
        }
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "---------------------------------------------- \n");
}
#endif

static void SetMyCallbacks(void)
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

    IOTA_SetMessageCallback(HandleMessageDown);
    // 推荐使用此API，可以处理自定格式的消息
    IOTA_SetRawMessageCallback(HandleRawMessageDown);
    IOTA_SetM2mCallback(HandleM2mMessageDown);
    IOTA_SetUserTopicMsgCallback(HandleUserTopicMessageDown);
    IOTA_SetUndefinedMessageCallback(HandletUndefinedMessageDown);
    // 推荐使用此API，可以处理自定格式的消息
    IOTA_SetUserTopicRawMsgCallback(HandleUserTopicRawMessageDown);
    IOTA_SetCmdCallback(HandleCommandRequest);
    IOTA_SetPropSetCallback(HandlePropertiesSet);
    IOTA_SetPropGetCallback(HandlePropertiesGet);
    IOTA_SetEventCallback(HandleEventsDown);
    IOTA_SetShadowGetCallback(HandleDeviceShadowRsp);
    IOTA_SetDeviceRuleSendMsgCallback(HandleDeviceRuleSendMsg);
    IOTA_SetDeviceConfigCallback(HandleDeviceConfig);

#if defined(SOFT_BUS_OPTION2)
    // 注册鸿蒙软总线
    CallbackParam param = {
        .sendDataResultCb = SendDataToDeviceCb,
        .onReceiveDataCb = ReceiveDataFromDeviceCb,
        .isValidIP = isValidIP,
        .isValidDeviceID = isValidDeviceID,
        .getAuthKey = getAuthKey,
        .getDeviceID = getDeviceId,
    };
    RegisterCallback(&param);
#endif
}

static void MyPrintLog(int level, char *format, va_list args)
{
    vprintf(format, args);
    /*
     * if you want to printf log in system log files,you can do this:
     * vsyslog(level, format, args);
     **/
}

#ifdef CUSTOM_RECONNECT_SWITCH
static pthread_t g_reconnectionThread;

static void *ReconnectRoutine(void *args)
{
    (void)args;
    while (1) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "device_demo: need to reconnect: %s \n", IOTA_IsConnected() ? "no" : "yes");
        if (!IOTA_IsConnected()) {
            int ret = IOTA_Connect();
            if (ret != 0) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: IOTA_Connect() error, result %d\n", ret);
            }
        }
        TimeSleep(30 * 1000); // 30s
    }
    return NULL;
}

static void ReconnectDemo()
{
    if (pthread_create(&g_reconnectionThread, NULL, ReconnectRoutine, NULL) != NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: create ReconnectionThread failed! ===================>\n");
    }
}
#endif

int main(int argc, char **argv)
{
#if defined(_DEBUG)
    (void)setvbuf(stdout, NULL, _IONBF, 0); // in order to make the console log printed immediately at debug mode
#endif

    IOTA_SetPrintLogCallback(MyPrintLog);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: start test ===================>\n");

    if (IOTA_Init(g_workPath) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: IOTA_Init() error, init failed\n");
        return 1;
    }

    SetAuthConfig();
    SetMyCallbacks();
    // add your fileName to store device rule
    IOTA_EnableDeviceRuleStorage(DEVICE_RULE_FILE_PATH);

#ifndef CUSTOM_RECONNECT_SWITCH
    // see handleLoginSuccess and handleLoginFailure for login result
    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: IOTA_Auth() error, Auth failed, result %d\n", ret);
    }
    while(!IOTA_IsConnected()) {
        TimeSleep(300);
    }

#else
    ReconnectDemo();
    while (!IOTA_IsConnected()) {
        int errorCode = pthread_yield();
        if (errorCode != 0) {
            PrintfLog(EN_LOG_LEVEL_WARNING, "device_demo: pthread_yield() failed, error code: %d", errorCode);
        }
    }
#endif
    Test_GetUploadFileUrl();

    Test_ReportDeviceInfo();
    TimeSleep(1500);

    int count = 0;
    while (count < 10000) {
        
        // NTP
        IOTA_GetNTPTime(NULL);
        TimeSleep(1500);

        // report device log
        unsigned long long timestamp = getTime();
        char timeStampStr[14];
        (void)sprintf_s(timeStampStr, sizeof(timeStampStr), "%llu", timestamp);
        char *log = "device log";
        IOTA_ReportDeviceLog("DEVICE_STATUS", log, timeStampStr, NULL);

        // message up
        Test_MessageReport();
        TimeSleep(1500);

        // properties report
        Test_PropertiesReport();
        TimeSleep(1500);

        // batchProperties report
        Test_BatchPropertiesReport(g_subDeviceId);
        TimeSleep(1500);

        // m2m send msg
        Test_M2MSendMsg();
        count++;
    }
    count = 0;
    while (count < 100) {
        count++;
        TimeSleep(11050);
    }
    return 0;
}
