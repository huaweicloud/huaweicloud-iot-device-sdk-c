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

/*
 * Device Remote Config Example
 * 
 * For relevant information, please see https://support.huaweicloud.com/intl/zh-cn/usermanual-iothub/iot_01_00302.html
 * For API document, please see https://support.huaweicloud.com/intl/zh-cn/api-iothub/iot_0802.html
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

// ------------------ Device Remote Config Callback -----------------------
static int HandleDeviceConfig(JSON *cfg, char *description)
{
    char *cfgstr = cJSON_Print(cfg);
    if (cfgstr) {
        PrintfLog(EN_LOG_LEVEL_INFO, "HandleDeviceConfig config content: %s\n", cfgstr);
    } else {
        PrintfLog(EN_LOG_LEVEL_WARNING, "HandleDeviceConfig config content is null\n");
    }
    // description shows the returned execution result information
    (void)strcpy_s(description, MaxDescriptionLen, "update config success");
    MemFree(&cfgstr);
    return 0; 
}

void HandleCommandRequest(EN_IOTA_COMMAND *command) 
{
	if (command == NULL) {
		return;
	}

	PrintfLog(EN_LOG_LEVEL_INFO, "command_test: HandleCommandRequest(), messageId %d, service_id %s, command_name %s, request_id %s\n", 
        command->mqtt_msg_info->messageId, command->service_id, command->command_name, command->request_id);
	PrintfLog(EN_LOG_LEVEL_INFO, "command_test: HandleCommandRequest(), request_id %s\n", command->request_id);

}
// ---------------------------- secret authentication --------------------------------------
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

    // secret authentication
    mqttDeviceSecretInit(g_address, g_port, g_deviceId, g_password); 
    
    // remote configuration function callback function
    IOTA_SetDeviceConfigCallback(HandleDeviceConfig);
    
    // Create connection
    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "basic_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    TimeSleep(15000);
    
    // Disconnect
    IOTA_Destroy();
}
