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
 * Device Shadow Example
 * 1、The device obtains the device shadow stored on the platform.
 * 2、The IoT platform sets device properties(expected values of the device shadow).
 * About device shadow visibility https://support.huaweicloud.com/usermanual-iothub/iot_01_0049.html
 */

// You can get the access address from IoT Console "Overview" -> "Access Information"
char *g_address = "XXXX"; 
char *g_port = "8883";

// deviceId, the mqtt protocol requires the user name to be filled in.
// Please fill in the deviceId
char *g_deviceId = "XXXX"; 
char *g_password = "XXXX";

int g_smoke_value = 14; // Product Model Settings
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

// -------------- Platform Sets Device Properties --------------------
static void Test_PropSetResponse(char *requestId)
{
    int messageId = IOTA_PropertiesSetResponse(requestId, 0, "success", NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "shadow_test: Test_PropSetResponse() failed, messageId %d\n", messageId);
    }
}

void HandlePropertiesSet(EN_IOTA_PROPERTY_SET *rsp) {
	if (rsp == NULL) {
		return;
	}
	PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandlePropertiesSet(), messageId %d \n", rsp->mqtt_msg_info->messageId);
	PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandlePropertiesSet(), request_id %s \n", rsp->request_id);
	PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandlePropertiesSet(), object_device_id %s \n", rsp->object_device_id);

	int i = 0;
	while (rsp->services_count > i) {
		PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandlePropertiesSet(), service_id %s \n", rsp->services[i].service_id);
		PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandlePropertiesSet(), properties %s \n", rsp->services[i].properties);
		i++;
	}

    /* Developers can implement device property processing here. */

	Test_PropSetResponse(rsp->request_id); //response
}

// ------------ Querying Device Properties --------------------
static void Test_PropGetResponse(char *requestId)
{
    const int serviceNum = 1;
    ST_IOTA_SERVICE_DATA_INFO serviceProp[serviceNum];

    char property[100] = {0};
    (void)sprintf_s(property, sizeof(property), "{\"Smoke_value\": %d}", g_smoke_value); 

    serviceProp[0].event_time = GetEventTimesStamp();
    serviceProp[0].service_id = "Smoke";
    serviceProp[0].properties = property;

    int messageId = IOTA_PropertiesGetResponse(requestId, serviceProp, serviceNum, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "shadow_test: Test_PropGetResponse() failed, messageId %d\n", messageId);
    }

    MemFree(&serviceProp[0].event_time);
}

void HandlePropertiesGet(EN_IOTA_PROPERTY_GET *rsp) {
	if (rsp == NULL) {
		return;
	}
	PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandlePropertiesSet(), messageId %d \n", rsp->mqtt_msg_info->messageId);
	PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandlePropertiesSet(), request_id %s \n", rsp->request_id);
	PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandlePropertiesSet(), object_device_id %s \n", rsp->object_device_id);

	PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandlePropertiesSet(), service_id %s \n", rsp->service_id);

	Test_PropGetResponse(rsp->request_id); //response
}

// ------------ Callback function for device shadows -----------------
void HandleDeviceShadowRsp(EN_IOTA_DEVICE_SHADOW *rsp) {
	if (rsp == NULL) {
		return;
	}
	PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandleDeviceShadowRsp(), messageId %d \n", rsp->mqtt_msg_info->messageId);
	PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandleDeviceShadowRsp(), request_id %s \n", rsp->request_id);
	PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandleDeviceShadowRsp(), object_device_id %s \n", rsp->object_device_id);

	int i = 0;
	while (rsp->shadow_data_count > i) {
		PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandleDeviceShadowRsp(), service_id %s \n", rsp->shadow[i].service_id);
		PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandleDeviceShadowRsp(), desired properties %s \n", rsp->shadow[i].desired_properties);
		PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandleDeviceShadowRsp(), reported properties %s \n", rsp->shadow[i].reported_properties);
		PrintfLog(EN_LOG_LEVEL_INFO, "shadow_test: HandleDeviceShadowRsp(), version    %d \n", rsp->shadow[i].version);
		i++;
	}
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
    // secret authentication
    mqttDeviceSecretInit(g_address, g_port, g_deviceId, g_password); 

    // Configuration of the callback function for the device to obtain the platform device shadow
    IOTA_SetShadowGetCallback(HandleDeviceShadowRsp);
    // Configuration of the callback function for the platform to set and obtain device shadows
    IOTA_SetPropSetCallback(HandlePropertiesSet);
    IOTA_SetPropGetCallback(HandlePropertiesGet);

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "shadow_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    TimeSleep(1500);
    
    SubscribeAll(); // Subscribing to System Topics

    // Obtain all the device shadow data stored in the platform. The request ID is requestId.
    IOTA_GetDeviceShadow("requestId", NULL, NULL, NULL);

    TimeSleep(5500);
    IOTA_Destroy();
}



