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

/*
 * Software and Firmware Upgrade Example
 * The steps are the following:
 *  1) Platform delivers an command to obtain the version information
 *  2) The device reports the versions
 *  3) Platform delivers an upgrade command
 *  4) The device retrieves the upgrading files from the URL in the upgrade command 
 *  5) The device reports the upgrade status
 * About software and firmware upgrade, please see https://support.huaweicloud.com/usermanual-iothub/iot_01_0155.html
 * For API, please see https://support.huaweicloud.com/api-iothub/iot_06_v5_3028.html
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

// ------------------ report device firmware or software version -----------------------
char *softwareVersion = "v1.0"; // Please fill in the version
char *firmwareVersion = "v2.0"; // Please fill in the version
static void Test_ReportOTAVersion(char *object_device_id)
{
    ST_IOTA_OTA_VERSION_INFO otaVersion;
    otaVersion.event_time = NULL;
    otaVersion.sw_version = softwareVersion;
    otaVersion.fw_version = firmwareVersion;
    otaVersion.object_device_id = object_device_id;

    int messageId = IOTA_OTAVersionReport(otaVersion, NULL);
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "ota_test: Test_ReportOTAVersion() failed, messageId %d\n", messageId);
    }
}

// ---------------- report device upgraded status ---------------------
// success equals to 0 meaning success, others meaning failures
static void Test_ReportUpgradeStatus(int success, char *version, char *object_device_id)
{
    ST_IOTA_UPGRADE_STATUS_INFO statusInfo;
    if (success == 0) {
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
    if (messageId < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "ota_test: Test_ReportUpgradeStatus() failed, messageId %d\n", messageId);
    }
}
// ---------------- OTA implementation process -------------------------
static void HandleEvenOtaVersion(char *objectDeviceId)
{
    // report OTA version
    // When the platform issues a version upgrade notification, it returns firmware version information
    Test_ReportOTAVersion(objectDeviceId);
}
    
static void HandleEvenOtaUrlResponse(char *objectDeviceId, int event_type, EN_IOTA_OTA_PARAS *ota_paras)
{
   
    /* The following is an example of OTA, please modify according to your needs */

    /*  event_type = 3 firmware upgrade
     *  event_type = 4 software upgrade
     *  event_type = 13 firmware upgrade v2
     *  event_type = 14 software upgrade v2
     */
    PrintfLog(EN_LOG_LEVEL_INFO, "event_type is %d\n", event_type);

    // start to receive firmware_upgrade or software_upgrade packages
    // Store file packages in ./${filename}中
    char filename[PKGNAME_MAX + 1];
    if (IOTA_GetOTAPackages_Ext(ota_paras->url, ota_paras->access_token,
            1000, ".", filename) == 0) {
        usleep(3000 * 1000);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the filename is %s\n", filename);
        // report successful upgrade status
        if (IOTA_OTAVerifySign(ota_paras->sign, ".", filename) == 0) {
            Test_ReportUpgradeStatus(0, ota_paras->version, objectDeviceId);
            return;
        }
        PrintfLog(EN_LOG_LEVEL_ERROR, "File verification failed, file may be incomplete. the filename is %s", filename);
        Test_ReportUpgradeStatus(-1, ota_paras->version, objectDeviceId);
        return;
    }
    // report failed status
    Test_ReportUpgradeStatus(-1, ota_paras->version, objectDeviceId);
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

    // ota upgrade callback
    IOTA_SetEvenOtaVersionUpCallback(HandleEvenOtaVersion);
    IOTA_SetEvenOtaUrlResponseCallback(HandleEvenOtaUrlResponse);

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "ota_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    TimeSleep(1500);

    while(1);

    IOTA_Destroy();
}
