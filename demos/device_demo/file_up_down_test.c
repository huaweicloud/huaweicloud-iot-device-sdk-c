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
#include "syslog.h"

/*
 * File upload and download
 * Instructions: https://support.huaweicloud.com/usermanual-iothub/iot_01_0033.html
 * API Interface: https://support.huaweicloud.com/api-iothub/iot_06_v5_3033.html
 */

// You can get the access address from IoT Console "Overview" -> "Access Information"
char *g_address = "XXXX"; 
char *g_port = "8883";

// deviceId, the mqtt protocol requires the user name to be filled in.
// Please fill in the deviceId
char *g_deviceId = "XXXX"; 
char *g_password = "XXXX";
    
char *uploadFilePath = "BUILD.txt"; // directory of files to be uploaded
char *downloadFilePath = "downloadFilePath.txt"; // The file directory downloaded by OBS
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

// event callback
static void HandleEventFile(char *objectDeviceId, EN_IOTA_MQTT_MSG_INFO *mqtt_msg_info, EN_IOTA_SERVICE_EVENT *message)
{
    if (message == NULL) {
        return;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "file_up_demo_test: HandleEventFile(), messageId %d\n", mqtt_msg_info->messageId);
    PrintfLog(EN_LOG_LEVEL_INFO, "file_up_demo_test: HandleEventFile(), object_device_id %s\n", objectDeviceId);
   
    // If it is a file upload event
    if (message->event_type == EN_IOTA_EVENT_GET_UPLOAD_URL_RESPONSE) {
        // Upload to OBS through the URL issued by the platform
        IOTA_UploadFile(uploadFilePath, message->file_mgr_paras->url, NULL);
    } else {
        IOTA_DownloadFile(downloadFilePath, message->file_mgr_paras->url, NULL);
    }
}

// ---------------------------- Key authentication --------------------------------------
static void mqttDeviceSecretInit(char *address, char *port, char *deviceId, char *password) {

    IOTA_Init("."); // Set certificate address to：./conf/rootcert.pem
    IOTA_SetPrintLogCallback(MyPrintLog); 
 
    // load connection parameters
    IOTA_ConnectConfigSet(address, port, deviceId, password);
    // Key authentication
    IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);
    IOTA_DefaultCallbackInit();
}

int main(int argc, char **argv) {

    mqttDeviceSecretInit(g_address, g_port, g_deviceId, g_password); 
    
    // set file upload and download callback functions
    IOTA_SetEvenFileManagerCallback(HandleEventFile);
    
    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "file_up_demo_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    TimeSleep(1500);

    char *time = GetEventTimesStamp();
    ST_IOTA_UPLOAD_FILE uploadFile = {NULL , NULL, 0};
    uploadFile.file_name = CombineStrings(5, g_deviceId, "_", time, "_", uploadFilePath);
    
    // The name uploaded to OBS is ${deviceid}_${timeStamp}_${FileName}
    IOTA_GetUploadFileUrl(&uploadFile, NULL);

    // Download the file with the name of ${uploadFilePath}
    IOTA_GetDownloadFileUrl(&uploadFile, NULL);

    // Directly upload files using OBS
    // char *url = ""; // Please enter OBS address
    // IOTA_UploadFile(uploadFilePath, url, NULL);

    // Download files directly using OBS
    // char *url = ""; // Please enter OBS address
    // IOTA_DownloadFile(uploadFilePath, url, NULL);

    TimeSleep(15000);

    // disconnect
    IOTA_Destroy();
    MemFree(&time);
}
