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
#include <string.h>
#include "iota_init.h"
#include "iota_datatrans.h"
#include "string_util.h"
#include "log_util.h"
#include "iota_cfg.h"
#include "mqttv5_util.h"
#include "cJSON.h"

/*
 * Property reporting example
 * 1、Property reporting related documents: https://support.huaweicloud.com/usermanual-iothub/iot_01_0323.html
 * 2、Platform setting device properties documents: https://support.huaweicloud.com/usermanual-iothub/iot_01_0336.html
 * 3、Platform querying device properties documents: https://support.huaweicloud.com/usermanual-iothub/iot_01_0336.html
 */

// You can get the access address from IoT Console "Overview" -> "Access Information"
char *g_address = "XXXX"; 
char *g_port = "8883";

// deviceId, The mqtt protocol requires the user name to be filled in.
// Please fill in the deviceId
char *g_deviceId = "XXXX"; 
char *g_password = "XXXX";

int g_smoke_value = 14; // product models
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

// ---------------------- Property reporting ---------------------------
static void Test_PropertiesReport(void)
{
    const int serviceNum = 1; // reported services' totol count
    ST_IOTA_SERVICE_DATA_INFO services[serviceNum];

    char service[100] = {0};
    (void)sprintf_s(service, sizeof(service), "{\"Smoke_value\": %d}", g_smoke_value); 
    // --------------- the data of service-------------------------------
    services[0].event_time = GetEventTimesStamp(); // if event_time is set to NULL, the time will be the iot-platform's time.
    services[0].service_id = "Smoke";
    services[0].properties = service;

    int messageId = IOTA_PropertiesReport(services, serviceNum, 0, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "properties_test: Test_PropertiesReport() failed, messageId %d\n", messageId);
    }

    MemFree(&services[0].event_time);
}

// ---------------- Set Device Properties --------------------
static void Test_PropSetResponse(char *requestId)
{
    int messageId = IOTA_PropertiesSetResponse(requestId, 0, "success", NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "properties_test: Test_PropSetResponse() failed, messageId %d\n", messageId);
    }
}

void HandlePropertiesSet(EN_IOTA_PROPERTY_SET *rsp) {
	if (rsp == NULL) {
		return;
	}
	PrintfLog(EN_LOG_LEVEL_INFO, "properties_test: HandlePropertiesSet(), messageId %d \n", rsp->mqtt_msg_info->messageId);
	PrintfLog(EN_LOG_LEVEL_INFO, "properties_test: HandlePropertiesSet(), request_id %s \n", rsp->request_id);
	PrintfLog(EN_LOG_LEVEL_INFO, "properties_test: HandlePropertiesSet(), object_device_id %s \n", rsp->object_device_id);

	int i = 0;
	while (rsp->services_count > i) {
		PrintfLog(EN_LOG_LEVEL_INFO, "properties_test: HandlePropertiesSet(), service_id %s \n", rsp->services[i].service_id);
		PrintfLog(EN_LOG_LEVEL_INFO, "properties_test: HandlePropertiesSet(), properties %s \n", rsp->services[i].properties);
        if (strncmp(rsp->services[i].service_id, "Smoke", strlen(rsp->services[i].service_id)) == 0) {
            cJSON *properties = cJSON_Parse(rsp->services[i].properties);
            cJSON *smoke = cJSON_GetObjectItem(properties, "Smoke_value");
            if (cJSON_IsNumber(smoke)) {
                g_smoke_value = (int)cJSON_GetNumberValue(smoke);
                PrintfLog(EN_LOG_LEVEL_INFO,"Set smoke_value success! g_smoke_value = %d\n", g_smoke_value);
            }
            cJSON_Delete(properties);
        }
		i++;
	}
	Test_PropSetResponse(rsp->request_id); //response
}

// --------------- Query Device Properties --------------------
static void Test_PropGetResponse(char *requestId)
{
    
    /* Developers here implement device property querying. The following is an example */
    const int serviceNum = 1;
    ST_IOTA_SERVICE_DATA_INFO serviceProp[serviceNum];

    char property[100] = {0};
    (void)sprintf_s(property, sizeof(property), "{\"Smoke_value\": %d}", g_smoke_value); 

    serviceProp[0].event_time = GetEventTimesStamp();
    serviceProp[0].service_id = "Smoke";
    serviceProp[0].properties = property;

    int messageId = IOTA_PropertiesGetResponse(requestId, serviceProp, serviceNum, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "properties_test: Test_PropGetResponse() failed, messageId %d\n", messageId);
    }

    MemFree(&serviceProp[0].event_time);
}

void HandlePropertiesGet(EN_IOTA_PROPERTY_GET *rsp) {
	if (rsp == NULL) {
		return;
	}
	PrintfLog(EN_LOG_LEVEL_INFO, "properties_test: HandlePropertiesSet(), messageId %d \n", rsp->mqtt_msg_info->messageId);
	PrintfLog(EN_LOG_LEVEL_INFO, "properties_test: HandlePropertiesSet(), request_id %s \n", rsp->request_id);
	PrintfLog(EN_LOG_LEVEL_INFO, "properties_test: HandlePropertiesSet(), object_device_id %s \n", rsp->object_device_id);

	PrintfLog(EN_LOG_LEVEL_INFO, "properties_test: HandlePropertiesSet(), service_id %s \n", rsp->service_id);

	Test_PropGetResponse(rsp->request_id); //response
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
}

int main(int argc, char **argv) 
{
    // secret authentication initialization
    mqttDeviceSecretInit(g_address, g_port, g_deviceId, g_password); 

    // Callback
    IOTA_SetPropSetCallback(HandlePropertiesSet);
    IOTA_SetPropGetCallback(HandlePropertiesGet);

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "properties_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    TimeSleep(1500);

    // properties Report
    Test_PropertiesReport();

    int i = 0;
    for (; i < 10; i++) {
        TimeSleep(3500);
        g_smoke_value++;
        PrintfLog(EN_LOG_LEVEL_INFO, "properties_test: g_smoke_value is %d \n", g_smoke_value);
        Test_PropertiesReport();
    }

    while(1);
    
    IOTA_Destroy();
}




