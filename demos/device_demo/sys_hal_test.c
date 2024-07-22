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
#include "sys_hal.h"

/*
 * Anomaly detection example
 * 1. Memory leakage detection
 * 2: Abnormal port detection
 * 3. CPU usage monitor
 * 4. Disk space monitor
 * 5. Battery level monitor
 * Before using this function, you need to enable the abnormality detection function on the platform. By default, the abnormality detection result is reported every 10 minutes.
 * For details about abnormality detection, see https://support.huaweicloud.com/usermanual-iothub/iot_01_0030_5.html.
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

// ------ Response to the property setting (device shadow) ----------------------
static void Test_PropSetResponse(char *requestId)
{
    int messageId = IOTA_PropertiesSetResponse(requestId, 0, "success", NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "sys_hal_test: Test_PropSetResponse() failed, messageId %d\n", messageId);
    }
}

// ------- Platform deliver property settings (Device Shadow) ------------------
void HandlePropertiesSet(EN_IOTA_PROPERTY_SET *rsp) {
	if (rsp == NULL) {
		return;
	}
	PrintfLog(EN_LOG_LEVEL_INFO, "sys_hal_test: HandlePropertiesSet(), messageId %d \n", rsp->mqtt_msg_info->messageId);
	PrintfLog(EN_LOG_LEVEL_INFO, "sys_hal_test: HandlePropertiesSet(), request_id %s \n", rsp->request_id);
	PrintfLog(EN_LOG_LEVEL_INFO, "sys_hal_test: HandlePropertiesSet(), object_device_id %s \n", rsp->object_device_id);

	int i = 0;
	while (rsp->services_count > i) {
		PrintfLog(EN_LOG_LEVEL_INFO, "sys_hal_test: HandlePropertiesSet(), service_id %s \n", rsp->services[i].service_id);
		PrintfLog(EN_LOG_LEVEL_INFO, "sys_hal_test: HandlePropertiesSet(), properties %s \n", rsp->services[i].properties);
		i++;
	}
    /* For abnormality detection delivery, no implementation is required. */
	Test_PropSetResponse(rsp->request_id); //response
}

// ------------ You can write your own implementation --------------------
static long GetMemoryUsed(void)
{
    /* Obtain the memory and return the memory percentage. */

    PrintfLog(EN_LOG_LEVEL_DEBUG, "##sys_hal_test # please implement GetMemoryUsed by yourself ###\n");
    return 80;
}

static long GetTotalMemory(void)
{
    /* Implement memory monitor here, return memory percentage */
    PrintfLog(EN_LOG_LEVEL_DEBUG, "##sys_hal_test # please implement GetTotalMemory by yourself ###\n");
    return 60;
}

static ArrayInfo *GetPortUsed(void)
{
    /* Implement port detection here and return the port list. */
    PrintfLog(EN_LOG_LEVEL_DEBUG, "##sys_hal_test # please implement GetPortUsed by yourself ###\n");
    // The following is an example:
    ArrayInfo *arrayInfo = (ArrayInfo *)malloc(sizeof(ArrayInfo));
    arrayInfo->array = (int *)malloc(sizeof(int) * 2);
    arrayInfo->array[0] = 1883;
    arrayInfo->array[1] = 8883;
    arrayInfo->arrayLen = 2;
    return arrayInfo;
}

static int GetCpuUsage(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "##sys_hal_test # please implement GetCpuUsage by yourself ###\n");
    return 60;
}

static long GetTotalDiskSpace(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "##sys_hal_test # please implement GetTotalDiskSpace by yourself ###\n");
    return 70;
}

static long GetDiskSpaceUsed(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "##sys_hal_test # please implement GetDiskSpaceUsed by yourself ###\n");
    return 88;
}

static int GetBatteryPercentage(void)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "##sys_hal_test # please implement GetBatteryPercentage by yourself ###\n");
    return 33;
}

// --------------- Loading Abnormaltiy Detection Callback Function ----------------------
static const TagSysHalOps g_sysHalOps = {
    .getMemoryUsed = GetMemoryUsed,
    .getTotalMemory = GetTotalMemory,
    .getPortUsed = GetPortUsed,
    .getCpuUsage = GetCpuUsage,
    .getTotalDiskSpace = GetTotalDiskSpace,
    .getDiskSpaceUsed = GetDiskSpaceUsed,
    .getBatteryPercentage = GetBatteryPercentage,
};

static const TagSysHal g_sysHal = {
    .name = "SysHal",
    .ops = &g_sysHalOps,
};

// ---------------------------- secret authentication --------------------------------------
static void mqttDeviceSecretInit(char *address, char *port, char *deviceId, char *password) 
{
    IOTA_Init("."); // The certificate address is ./conf/rootcert.pem
    IOTA_SetPrintLogCallback(MyPrintLog); 
 
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
    
    // Loading abnormality detection callback
    SysHalInstall(&g_sysHal);

    // Callback function for delivering device shadow data
    IOTA_SetPropSetCallback(HandlePropertiesSet);

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "sys_hal_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    TimeSleep(1500);

    while(1);
    IOTA_Destroy();
}
