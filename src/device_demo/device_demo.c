/*
 * Copyright (c) 2020-2022 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#include "hw_type.h"
#include "iota_init.h"
#include "iota_cfg.h"
#include "log_util.h"
#include "json_util.h"
#include "string_util.h"
#include "iota_login.h"
#include "iota_datatrans.h"
#include "string.h"
#include "cJSON.h"
#include "iota_error_type.h"
#include "mqttv5_util.h"
#ifdef SSH_SWITCH
#include "wss_client.h"
#endif
/*
 * if you want to use syslog,you should do this:
 * #include "syslog.h"
 * #define _SYS_LOG
 *
 *   */

char *workPath = ".";
char *gatewayId = NULL;

char *serverIp_ = "iot-mqtts.cn-north-4.myhuaweicloud.com";
int port_ = 8883;

 // deviceId, The mqtt protocol requires the user name to be filled in.
 // Here we use deviceId as the username
char *username_ = "XXXXX";
char *password_ = "XXXXX";

int disconnected_ = 0;

int connect_failed_times = 0;

char *ota_version = NULL;
char *subDeviceId = "XXXX";
void Test_MessageReport(void);
void Test_PropertiesReport(void);
void Test_BatchPropertiesReport(char *deviceId);
void Test_CommandResponse(char *requestId);
void Test_PropSetResponse(char *requestId);
void Test_PropGetResponse(char *requestId);
void Test_ReportOTAVersion(void);
void Test_ReportUpgradeStatus(int i, char *version);
void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleConnectionLost(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleDisConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleDisConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleSubscribesuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleSubscribeFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandlePublishSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandlePublishFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp);
void HandleMessageDown(EN_IOTA_MESSAGE *rsp, void *mqttv5);
void HandleUserTopicMessageDown(EN_IOTA_USER_TOPIC_MESSAGE *rsp);
void HandleCommandRequest(EN_IOTA_COMMAND *command);
void HandlePropertiesSet(EN_IOTA_PROPERTY_SET *rsp);
void TimeSleep(int ms);
void HandlePropertiesGet(EN_IOTA_PROPERTY_GET *rsp);
void HandleDeviceShadowRsp(EN_IOTA_DEVICE_SHADOW *rsp);
void HandleEventsDown(EN_IOTA_EVENT *message);
void MyPrintLog(int level, char *format, va_list args);
void SetAuthConfig(void);
void SetMyCallbacks(void);
void Test_ReportJson();
void Test_ReportBinary();
void Test_CmdRspV3();
void Test_UpdateSubDeviceStatus(char *deviceId);
void Test_GtwAddSubDevice();
void Test_GtwDelSubDevice();
void Test_ReportDeviceInfo();

#if defined(MQTTV5)
void Test_CorreaytionData(char *response_topic);
void Test_PayloadFormatIndicator();
void Test_ContentType();
void Test_UserProperty();
void Test_MessageReportV5();
void Test_PropertiesReportV5();
#endif

void TimeSleep(int ms)
{
#if defined(WIN32) || defined(WIN64)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}
// -----------------Test  MQTT5.0 ------------------------
#if defined(MQTTV5)

// V5 correlation data and response topic
void Test_CorreaytionData(char *response_topicParas)
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    ST_IOTA_MESS_REP_INFO mass = { NULL, "data123", "123", "hello123123123123", NULL };
    char *topic = CombineStrings(4, "$oc/devices/", username_, "/user/", response_topicParas);
    char num[5] = "1234";

    // MessageReport one, Topic = A, response_topic = B
    PrintfLog(EN_LOG_LEVEL_DEBUG, "Test_CorreaytionData()\n");
    massv5.response_topic = topic;
    massv5.correlation_data = num;
    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_MessageReport() failed, messageId %d\n", messageId);
    }

    // MessageReport two, Topic = B, response_topic = NULL
    massv5.response_topic = NULL;
    massv5.correlation_data = num;
    mass.topicParas = response_topicParas;
    messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_MessageReport() failed, messageId %d\n", messageId);
    }
}

// v5 contnt type
void Test_ContentType()
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    ST_IOTA_MESS_REP_INFO mass = { NULL, "data123", "123", "hello123123123123", NULL };

    PrintfLog(EN_LOG_LEVEL_DEBUG, "Test_ContentType()\n");
    massv5.contnt_type = "application/json";

    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_MessageReport() failed, messageId %d\n", messageId);
    }
}

// v5 User Property
void Test_UserProperty()
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    ST_IOTA_MESS_REP_INFO mass = { NULL, "data123", "123", "hello123123123123", NULL };

    MQTTV5_USER_PRO user_0;
    MQTTV5_USER_PRO user_1;

    PrintfLog(EN_LOG_LEVEL_DEBUG, "Test_UserProperty()\n");
    user_0.key = "region";
    user_0.Value = "A";
    user_0.nex = &user_1;

    user_1.key = "type";
    user_1.Value = "JSON";
    user_1.nex = NULL;

    massv5.properties = &user_0;
    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_MessageReport() failed, messageId %d\n", messageId);
    }
}

// v5 message report
void Test_MessageReportV5()
{
    MQTTV5_DATA massv5 = mqttv5_initializer;
    ST_IOTA_MESS_REP_INFO mass = { NULL, "data123", "123", "hello123123123123", NULL };

    // MQTTV5 publish
    MQTTV5_USER_PRO user_0;
    MQTTV5_USER_PRO user_1;
    user_0.key = "name";
    user_0.Value = "B";
    user_0.nex = &user_1;

    user_1.key = "type";
    user_1.Value = "JSON";
    user_1.nex = NULL;

    massv5.properties = &user_0;
    massv5.response_topic = "responseTopic";
    massv5.correlation_data = "4321";
    massv5.contnt_type = "application/json";

    // default topic
    int messageId = IOTA_MessageReportV5(mass, 0, NULL, &massv5);
    // user topic
    // 	mass.topicParas = "devMsg";
    // 	int messageId = IOTA_MessageReportV5(mass, 0, NULL , &massv5);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_MessageReport() failed, messageId %d\n", messageId);
    }
}

// v5 Properties Report
void Test_PropertiesReport5()
{
    int serviceNum = 2; // reported services' totol count
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];
    MQTTV5_DATA massv5 = mqttv5_initializer;
    MQTTV5_USER_PRO user_0;
    MQTTV5_USER_PRO user_1;
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
    user_0.key = "region";
    user_0.Value = "A";
    user_0.nex = &user_1;

    user_1.key = "type";
    user_1.Value = "JSON";
    user_1.nex = NULL;

    massv5.properties = &user_0;
    massv5.response_topic = "responseTopic";
    massv5.correlation_data = "1";
    massv5.contnt_type = "application/json";

    int messageId = IOTA_PropertiesReportV5(services, serviceNum, 0, NULL, &massv5);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesReport() failed, messageId %d\n", messageId);
    }

    MemFree(&services[0].event_time);
}
#endif
// ---------------------------Test  data report------------------------------------

void Test_MessageReport()
{
    // default topic
    int messageId = IOTA_MessageReport(NULL, "data123", "123", "hello123123123123", NULL, 0, NULL);

    // user topic
    // 	int messageId = IOTA_MessageReport(NULL, "data123", "123", "hello", "devMsg", 0);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_MessageReport() failed, messageId %d\n", messageId);
    }
}

// V3 report
void Test_ReportJson()
{
    const int serviceNum = 2; // reported services' totol count
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];

    // ---------------the data of service1-------------------------------
    char *service1 = "{\"Load\":\"6\",\"ImbA_strVal\":\"7\"}";

    services[0].event_time =
        GetEventTimesStamp(); // if event_time is set to NULL, the time will be the iot-platform's time.
    services[0].service_id = "LTE";
    services[0].properties = service1;

    // ---------------the data of service2-------------------------------
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
void Test_ReportBinary()
{
    int messageId = IOTA_BinaryReportV3("1234567890", NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportBinary() failed, messageId %d\n", messageId);
    }
}

// V3 report
void Test_CmdRspV3()
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

// If you cancel #define STORE_DATA_SWITCH comments , enable offline caching¡£
// Note: the change is only a sample, for reference only
// Currently, the container for storing exception messages is a dynamic two-dimensional array,
// which can be selected by users according to their business logic
// #define STORE_DATA_SWITCH

/*
 * Open a space and store data, and report data at the same time
 * It is recommended to store the data of the sensor in the container
 * before reporting the data each time. Only after receiving
 * the callback of onpublishsuccess can the message be removed
 * from the container.
 */
#if defined(STORE_DATA_SWITCH)
char **cacheSpace = NULL;
int cacheSpaceLen = 0;
int cache_p = 0;
#define CACHE_SPACE_MAX 100

void Test_PropertiesStoreData()
{
    int serviceNum = 2; // reported services' totol count
    int i = 0;
    if (cacheSpace == NULL) {
        cacheSpace = (char **)malloc(CACHE_SPACE_MAX * sizeof(char *));
        if (cacheSpace == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_CmdRspV3()  malloc failed! ");
            return; // Failed to apply for space
        }
    }
    ST_IOTA_SERVICE_DATA_INFO *services =
        (ST_IOTA_SERVICE_DATA_INFO *)malloc(sizeof(ST_IOTA_SERVICE_DATA_INFO) * serviceNum);
    // ---------------the data of service1-------------------------------
    char *service1 = "{\"Load\":\"6\",\"ImbA_strVal\":\"7\"}";

    services->event_time =
        GetEventTimesStamp(); // if event_time is set to NULL, the time will be the iot-platform's time.
    services->service_id = "parameter";
    services->properties = service1;

    // ---------------the data of service2-------------------------------
    char *service2 = "{\"PhV_phsA\":\"9\",\"PhV_phsB\":\"8\"}";

    (services + 1)->event_time = NULL;
    (services + 1)->service_id = "analog";
    (services + 1)->properties = service2;

    // Message store
    if (cacheSpaceLen > CACHE_SPACE_MAX) {
        MemFree(cacheSpace[cache_p]);
        cacheSpace[cache_p] = services;
        cache_p++;
    } else {
        cache_p = 0;
        for (i = 0; i < CACHE_SPACE_MAX; i++) {
            if (cacheSpace[i] == NULL) {
                cacheSpace[i] = services;
                cacheSpaceLen++;
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

void Test_PropertiesReport()
{
    const int serviceNum = 2; // reported services' totol count
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];

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

    int messageId = IOTA_PropertiesReport(services, serviceNum, 0, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesReport() failed, messageId %d\n", messageId);
    }

    MemFree(&services[0].event_time);
}

void Test_BatchPropertiesReport(char *deviceId)
{
    const int deviceNum = 1;                     // the number of sub devices
    ST_IOTA_DEVICE_DATA_INFO devices[deviceNum]; // Array of structures to be reported by sub devices
    int serviceList[deviceNum]; // Corresponding to the number of services to be reported for each sub device
    serviceList[0] = 2;         // device 1 reports two services
                                // 	serviceList[1] = 1;		  // device 2 reports one service

    char *device1_service1 = "{\"Load\":\"1\",\"ImbA_strVal\":\"3\"}"; // must be json

    char *device1_service2 = "{\"PhV_phsA\":\"2\",\"PhV_phsB\":\"4\"}"; // must be json

    devices[0].device_id = deviceId;
    devices[0].services[0].event_time = GetEventTimesStamp();
    devices[0].services[0].service_id = "parameter";
    devices[0].services[0].properties = device1_service1;

    devices[0].services[1].event_time = GetEventTimesStamp();
    devices[0].services[1].service_id = "analog";
    devices[0].services[1].properties = device1_service2;

    // 	char *device2_service1 = "{\"AA\":\"2\",\"BB\":\"4\"}";
    // 	devices[1].device_id = "subDevices22222";
    // 	devices[1].services[0].event_time = "d2s1";
    // 	devices[1].services[0].service_id = "device2_service11111111";
    // 	devices[1].services[0].properties = device2_service1;

    int messageId = IOTA_BatchPropertiesReport(devices, deviceNum, serviceList, 0, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_BatchPropertiesReport() failed, messageId %d\n", messageId);
    }

    MemFree(&devices[0].services[0].event_time);
    MemFree(&devices[0].services[1].event_time);
}

void Test_UpdateSubDeviceStatus(char *deviceId)
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

void Test_CommandResponse(char *requestId)
{
    char *pcCommandRespense = "{\"SupWh\": \"aaa\"}"; // in service accumulator

    int result_code = 0;
    char *response_name = "cmdResponses";

    int messageId = IOTA_CommandResponse(requestId, result_code, response_name, pcCommandRespense, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_CommandResponse() failed, messageId %d\n", messageId);
    }
}

void Test_PropSetResponse(char *requestId)
{
    int messageId = IOTA_PropertiesSetResponse(requestId, 0, "success", NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropSetResponse() failed, messageId %d\n", messageId);
    }
}

void Test_PropGetResponse(char *requestId)
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

void Test_ReportOTAVersion()
{
    ST_IOTA_OTA_VERSION_INFO otaVersion;

    otaVersion.event_time = NULL;
    otaVersion.sw_version = "v1.0";
    otaVersion.fw_version = "v1.0";
    otaVersion.object_device_id = NULL;

    int messageId = IOTA_OTAVersionReport(otaVersion, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportOTAVersion() failed, messageId %d\n", messageId);
    }
}

void Test_ReportUpgradeStatus(int i, char *version)
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
    statusInfo.object_device_id = NULL;

    int messageId = IOTA_OTAStatusReport(statusInfo, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportUpgradeStatus() failed, messageId %d\n", messageId);
    }
}

void Test_GtwAddSubDevice()
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
    subDeviceInfos.deviceInfo[0].product_id = "your_product_id";
    subDeviceInfos.deviceInfo[1].product_id = "5f58768785edc002bc69cbf2";

    subDeviceInfos.event_id = "123123";
    subDeviceInfos.event_time = NULL;

    int messageId = IOTA_AddSubDevice(&subDeviceInfos, deviceNum, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_GtwAddSubDevice() failed, messageId %d\n", messageId);
    }
}


void Test_GtwDelSubDevice()
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

void Test_ReportDeviceInfo()
{
    ST_IOTA_DEVICE_INFO_REPORT deviceInfo;

    deviceInfo.device_sdk_version = SDK_VERSION;
    deviceInfo.sw_version = "v1.0";
    deviceInfo.fw_version = "v1.0";
    deviceInfo.event_time = NULL;
    deviceInfo.object_device_id = username_;

    int messageId = IOTA_ReportDeviceInfo(&deviceInfo, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportDeviceInfo() failed, messageId %d\n", messageId);
    }
}


// ---------------------------------------------------------------------------------------------

void HandleConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: handleConnectSuccess(), login success\n");
    disconnected_ = 0;
#if defined(STORE_DATA_SWITCH)
    int i = 0;
    if (cacheSpace == NULL || cacheSpaceLen <= 0) {
        return;
    }
    // Reconnect -- Send the stored data
    for (i = 0; i < CACHE_SPACE_MAX && cacheSpaceLen > 0; i++) {
        if (cacheSpace[i] != NULL) {
            int serviceNum = sizeof(cacheSpace[i]) / sizeof(ST_IOTA_SERVICE_DATA_INFO);
            int messageId = IOTA_PropertiesReport(cacheSpace[i], serviceNum, 0, (void *)cacheSpace[i]);
            if (messageId != 0) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesReport() failed, messageId %d\n", messageId);
            }
            TimeSleep(100);
        }
    }

#endif
}

void HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: HandleConnectFailure() error, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
    // judge if the network is available etc. and login again
    // ...
    PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: HandleConnectFailure() login again\n");

    connect_failed_times++;
    if (connect_failed_times < 10) {
        TimeSleep(50);
    } else if (connect_failed_times < 50) {
        TimeSleep(2500);
    }

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: HandleAuthFailure() error, login again failed, result %d\n", ret);
    }
}

void HandleConnectionLost(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: HandleConnectionLost() error, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
    // judge if the network is available etc. and login again
    // ...

    if (connect_failed_times < 10) {
        TimeSleep(50);
    } else if (connect_failed_times < 50) {
        TimeSleep(2500);
    }

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: HandleConnectionLost() error, login again failed, result %d\n",
            ret);
    }
}

void HandleDisConnectSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    disconnected_ = 1;
}

void HandleDisConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "device_demo: HandleDisConnectFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

void HandleSubscribesuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubscribesuccess() messageId %d\n", rsp->mqtt_msg_info->messageId);
}

void HandleSubscribeFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING,
        "device_demo: HandleSubscribeFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

void HandlePublishSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePublishSuccess() messageId %d\n", rsp->mqtt_msg_info->messageId);

#if defined(STORE_DATA_SWITCH)
    int i = 0;
    if (rsp->mqtt_msg_info->context == NULL) {
        return;
    }
    for (i = 0; i < CACHE_SPACE_MAX && cacheSpaceLen > 0; i++) {
        if (cacheSpace[i] == rsp->mqtt_msg_info->context) {
            MemFree(cacheSpace[i]);
            cacheSpace[i] = NULL;
            cacheSpaceLen--;
            break;
        } else if (cacheSpace[i] != NULL) { // 重传
            int serviceNum = sizeof(cacheSpace[i]) / sizeof(ST_IOTA_SERVICE_DATA_INFO);
            int messageId = IOTA_PropertiesReport(cacheSpace[i], serviceNum, 0, (void *)cacheSpace[i]);
            if (messageId != 0) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesReport() failed, messageId %d\n", messageId);
            }
            TimeSleep(100);
        }
    }
#endif
}

void HandlePublishFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_WARNING, "device_demo: HandlePublishFailure() warning, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);
}

// -------------------handle  message arrived-------------------------------

void HandleMessageDown(EN_IOTA_MESSAGE *rsp, void *mqttv5)
{
    if (rsp == NULL) {
        return;
    }
#if defined(MQTTV5)
    MQTTV5_DATA *mqtt = (MQTTV5_DATA *)mqttv5;
    MQTTV5_USER_PRO *user_pro = mqtt->properties;
    if (mqtt->contnt_type != NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(),contnt_type = %s\n", mqtt->contnt_type);
    }
    if (user_pro != NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(),properties is:\n");
        while (user_pro != NULL) {
            PrintfLog(EN_LOG_LEVEL_INFO, "key = %s ,Value = %s\n", user_pro->key, user_pro->Value);
            user_pro = (MQTTV5_USER_PRO *)user_pro->nex;
        }
    }
    if (mqtt->correlation_data != NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(),correlation_data = %s\n",
            mqtt->correlation_data);
    }
    if (mqtt->response_topic != NULL) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(),response_topic = %s\n", mqtt->response_topic);
    }
    // 删除链表、释放内存
    listFree(mqtt->properties);
#endif
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), content %s\n", rsp->content);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), id %s\n", rsp->id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), name %s\n", rsp->name);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), object_device_id %s\n", rsp->object_device_id);
}

void HandlePropertiesSet(EN_IOTA_PROPERTY_SET *rsp)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), messageId %d \n", rsp->mqtt_msg_info->messageId);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), request_id %s \n", rsp->request_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), object_device_id %s \n", rsp->object_device_id);

    int i = 0;
    while (rsp->services_count > 0) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), service_id %s \n",
            rsp->services[i].service_id);
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), properties %s \n",
            rsp->services[i].properties);
        rsp->services_count--;
        i++;
    }

    Test_PropSetResponse(rsp->request_id); // response
}

void HandlePropertiesGet(EN_IOTA_PROPERTY_GET *rsp)
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

void HandleDeviceShadowRsp(EN_IOTA_DEVICE_SHADOW *rsp)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), messageId %d \n",
        rsp->mqtt_msg_info->messageId);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), request_id %s \n", rsp->request_id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), object_device_id %s \n", rsp->object_device_id);

    int i = 0;
    while (rsp->shadow_data_count > 0) {
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), service_id %s \n",
            rsp->shadow[i].service_id);
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), desired properties %s \n",
            rsp->shadow[i].desired_properties);
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), reported properties %s \n",
            rsp->shadow[i].reported_properties);
        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), version    %d \n", rsp->shadow[i].version);
        rsp->shadow_data_count--;
        i++;
    }
}

void HandleUserTopicMessageDown(EN_IOTA_USER_TOPIC_MESSAGE *rsp)
{
    if (rsp == NULL) {
        return;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), topic_para %s\n", rsp->topic_para);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), content %s\n", rsp->content);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), id %s\n", rsp->id);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), name %s\n", rsp->name);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), object_device_id %s\n", rsp->object_device_id);
}

void HandleCommandRequest(EN_IOTA_COMMAND *command)
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

#ifdef SSH_SWITCH
void HandleTunnelMgr(EN_IOTA_EVENT *message, int i)
{
    URL_INFO info = {NULL, NULL, NULL, NULL};
    int ret = 0;

    if (message->services[i].event_type != EN_IOTA_EVENT_TUNNEL_NOTIFY)
        return;

    ret = WssClientSplitUrl(&info, message->services[i].tunnel_mgr_paras->tunnel_url, message->services[i].tunnel_mgr_paras->tunnel_access_token);
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

void HandleEventsDown(EN_IOTA_EVENT *message)
{
    if (message == NULL) {
        return;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), messageId %d\n", message->mqtt_msg_info->messageId);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), services_count %d\n", message->services_count);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), object_device_id %s\n", message->object_device_id);
    int i = 0;
    while (message->services_count > 0) {
        // sub device manager
        if (message->services[i].servie_id == EN_IOTA_EVENT_SUB_DEVICE_MANAGER) {
            // if it is the platform inform the gateway to add or delete the sub device
            if (message->services[i].event_type == EN_IOTA_EVENT_ADD_SUB_DEVICE_NOTIFY ||
                message->services[i].event_type == EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY) {
                int j = 0;
                while (message->services[i].paras->devices_count > 0) {
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), parent_device_id: %s \n",
                        message->services[i].paras->devices[j].parent_device_id);
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), device_id: %s \n",
                        message->services[i].paras->devices[j].device_id);
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), node_id: %s \n",
                        message->services[i].paras->devices[j].node_id);

                    // add a sub device
                    if (message->services[i].event_type == EN_IOTA_EVENT_ADD_SUB_DEVICE_NOTIFY) {
                        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), name: %s \n",
                            message->services[i].paras->devices[j].name);
                        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), manufacturer_id: %s \n",
                            message->services[i].paras->devices[j].manufacturer_id);
                        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), product_id: %s \n",
                            message->services[i].paras->devices[j].product_id);

                        Test_UpdateSubDeviceStatus(
                            message->services[i].paras->devices[j].device_id); // report status of the sub device
                        Test_BatchPropertiesReport(
                            message->services[i].paras->devices[j].device_id); // report data of the sub device
                    } else if (message->services[i].event_type ==
                        EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY) { // delete a sub device
                        PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), the sub device is deleted: %s\n",
                            message->services[i].paras->devices[j].device_id);
                    }

                    j++;
                    message->services[i].paras->devices_count--;
                }
            } else if (message->services[i].event_type == EN_IOTA_EVENT_ADD_SUB_DEVICE_RESPONSE) {
                int j = 0;
                while (message->services[i].gtw_add_device_paras->successful_devices_count > 0) {
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), device_id: %s \n",
                        message->services[i].gtw_add_device_paras->successful_devices[j].device_id);
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), name: %s \n",
                        message->services[i].gtw_add_device_paras->successful_devices[j].name);
                    j++;
                    message->services[i].gtw_add_device_paras->successful_devices_count--;
                }
                j = 0;
                while (message->services[i].gtw_add_device_paras->failed_devices_count > 0) {
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), error_code: %s \n",
                        message->services[i].gtw_add_device_paras->failed_devices[j].error_code);
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), error_msg: %s \n",
                        message->services[i].gtw_add_device_paras->failed_devices[j].error_msg);
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), node_id: %s \n",
                        message->services[i].gtw_add_device_paras->failed_devices[j].node_id);
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), product_id: %s \n",
                        message->services[i].gtw_add_device_paras->failed_devices[j].product_id);

                    j++;
                    message->services[i].gtw_add_device_paras->failed_devices_count--;
                }
            } else if (message->services[i].event_type == EN_IOTA_EVENT_DEL_SUB_DEVICE_RESPONSE) {
                int j = 0;
                while (message->services[i].gtw_del_device_paras->successful_devices_count > 0) {
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), device_id: %s \n",
                        message->services[i].gtw_del_device_paras->successful_devices[j]);
                    j++;
                    message->services[i].gtw_del_device_paras->successful_devices_count--;
                }
                j = 0;
                while (message->services[i].gtw_del_device_paras->failed_devices_count > 0) {
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), error_code: %s \n",
                        message->services[i].gtw_del_device_paras->failed_devices[j].error_code);
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), error_msg: %s \n",
                        message->services[i].gtw_del_device_paras->failed_devices[j].error_msg);
                    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), device_id: %s \n",
                        message->services[i].gtw_del_device_paras->failed_devices[j].device_id);

                    j++;
                    message->services[i].gtw_del_device_paras->failed_devices_count--;
                }
            }
        } else if (message->services[i].servie_id == EN_IOTA_EVENT_OTA) {
            if (message->services[i].event_type == EN_IOTA_EVENT_VERSION_QUERY) {
                // report OTA version
                Test_ReportOTAVersion();
            }

            if (message->services[i].event_type == EN_IOTA_EVENT_FIRMWARE_UPGRADE ||
                message->services[i].event_type == EN_IOTA_EVENT_SOFTWARE_UPGRADE) {
                // check md5
                char *pkg_md5 = "yourMd5"; // the md5 value of your ota package
                if (strcmp(pkg_md5, message->services[i].ota_paras->sign)) {
                    // report failed status
                    Test_ReportUpgradeStatus(-1, message->services[i].ota_paras->version);
                }

                // start to receive packages and firmware_upgrade or software_upgrade
                if (IOTA_GetOTAPackages(message->services[i].ota_paras->url,
                    message->services[i].ota_paras->access_token, 1000) == 0) {
                    usleep(3000 * 1000);
                    // report successful upgrade status
                    Test_ReportUpgradeStatus(0, message->services[i].ota_paras->version);
                } else {
                    // report failed status
                    Test_ReportUpgradeStatus(-1, message->services[i].ota_paras->version);
                }
            }
        } else if (message->services[i].servie_id == EN_IOTA_EVENT_TIME_SYNC) {
            if (message->services[i].event_type == EN_IOTA_EVENT_GET_TIME_SYNC_RESPONSE) {
                PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), device_real_time: %lld \n",
                    message->services[i].ntp_paras->device_real_time);
            }
        } else if (message->services[i].servie_id == EN_IOTA_EVENT_DEVICE_LOG) {
            if (message->services[i].event_type == EN_IOTA_EVENT_LOG_CONFIG) {
                PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), log_switch: %s \n",
                    message->services[i].device_log_paras->log_switch);
                PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), end_time: %s \n",
                    message->services[i].device_log_paras->end_time);
            }
        } else if (message->services[i].servie_id == EN_IOTA_EVENT_TUNNEL_MANAGER) {
#ifdef SSH_SWITCH            
            HandleTunnelMgr(message, i);
#endif
		}
        i++;
        message->services_count--;
    }
}

// -------------------------------------------------------------------------

void SetAuthConfig()
{
    IOTA_ConfigSetStr(EN_IOTA_CFG_MQTT_ADDR, serverIp_);
    IOTA_ConfigSetUint(EN_IOTA_CFG_MQTT_PORT, port_);
    IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICEID, username_);
    IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICESECRET, password_);
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);
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


void SetMyCallbacks()
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
    IOTA_SetUserTopicMsgCallback(HandleUserTopicMessageDown);
    IOTA_SetCmdCallback(HandleCommandRequest);
    IOTA_SetPropSetCallback(HandlePropertiesSet);
    IOTA_SetPropGetCallback(HandlePropertiesGet);
    IOTA_SetEventCallback(HandleEventsDown);
    IOTA_SetShadowGetCallback(HandleDeviceShadowRsp);
}

void MyPrintLog(int level, char *format, va_list args)
{
    vprintf(format, args);
    /*
     * if you want to printf log in system log files,you can do this:
     * vsyslog(level, format, args);
     *   */
}

int main(int argc, char **argv)
{
#if defined(_DEBUG)
    setvbuf(stdout, NULL, _IONBF, 0); // in order to make the console log printed immediately at debug mode
#endif

    IOTA_SetPrintLogCallback(MyPrintLog);
    PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: start test ===================>\n");

    if (IOTA_Init(workPath) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: IOTA_Init() error, init failed\n");
        return 1;
    }

    SetAuthConfig();
    SetMyCallbacks();

    // see handleLoginSuccess and handleLoginFailure for login result
    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: IOTA_Auth() error, Auth failed, result %d\n", ret);
    }

    TimeSleep(10500);
    int count = 0;
    while (count < 10000) {
        // report device info
        Test_ReportDeviceInfo();
        TimeSleep(1500);

        // NTP
        IOTA_GetNTPTime(NULL);

        TimeSleep(1500);

        // report device log
        unsigned long long timestamp = getTime();
        char timeStampStr[14];
        sprintf_s(timeStampStr, sizeof(timeStampStr), "%llu", timestamp);
        char *log = "device log";
        IOTA_ReportDeviceLog("DEVICE_STATUS", log, strlen(log), timeStampStr, NULL);

        // message up
        Test_MessageReport();

        TimeSleep(1500);

        // properties report
        Test_PropertiesReport();

        TimeSleep(1500);

        // batchProperties report
        Test_BatchPropertiesReport(NULL);

        TimeSleep(1500);

        // command response
        Test_CommandResponse("1005");

        TimeSleep(1500);

        // propSetResponse
        Test_PropSetResponse("1006");

        TimeSleep(1500);

        // propSetResponse
        Test_PropGetResponse("1007");

        TimeSleep(5500);

        IOTA_SubscribeUserTopic("devMsg");

        TimeSleep(1500);

        // get device shadow
        IOTA_GetDeviceShadow("1232", NULL, NULL, NULL);

        count++;
    }
    count = 0;
    while (count < 100) {
        count++;
        TimeSleep(50);
    }

    return 0;
}
