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
#include "bootstrap.h"

/*
 * For bootstrap device provision examples, please see：https://support.huaweicloud.com/qs-iotps/iot_03_0006.html
 * 1) Provision with certificate authentication
 * The default certificate storage address is ./conf/deviceCert.pem and ./conf/deviceCert.key
 * 2) Provision with secret authentication 
 * The default CA certificate storage address is: ./conf/rootcert.pem
 * ps：After the device is distributed, it will actively create a device in the device access console. 
 *   you can directly use the same DeviceId to connect to the Device Access Service Device_demo
 **/

// bootstrapAddress = "iot-bs.cn-north-4.myhuaweicloud.com";
char *bootstrapAddress = "XXXX";
char *port = "8883";

// deviceId，the mqtt protocol requires the user name to be filled in.
char *deviceId = "XXXX"; 
char *password = "XXXX"; 

char *baseStrategyKeyword = NULL; // Fill in when the keyword source of static strategy is "property reporting"
char *privateKeyPassword = ""; // Fill in the password of the certificate key when using certificate authentication

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

//  Connection failure callback function
static void HandleConnectFailure(EN_IOTA_MQTT_PROTOCOL_RSP *rsp)
{
    PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: HandleConnectFailure() error, messageId %d, code %d, messsage %s\n",
        rsp->mqtt_msg_info->messageId, rsp->mqtt_msg_info->code, rsp->message);

    /* You can set the disconnection reconnection mechanism here */
}

void SetMyCallbacks(void)
{
    // Set connection callback function
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECT_FAILURE, HandleConnectFailure);
    IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_CONNECTION_LOST, HandleConnectFailure);
}

int main(int argc, char **argv) {
    IOTA_Destroy();
    IOTA_Init("."); // Set the certificate address to: ./conf/rootcert.pem
    IOTA_SetPrintLogCallback(MyPrintLog); // Set the log printing method
    
    SetMyCallbacks();

    BOOTSTEAP_INIT parameter = {bootstrapAddress, port, deviceId, password, baseStrategyKeyword, 
        {NULL}, {privateKeyPassword}};
    // secret authentication
    bootstrapAuth(parameter, 0, 0);
    
    // Certificate authentication
    // bootstrapAuth(parameter, 0, 1);
    TimeSleep(3000);
    IOTA_Destroy();
    while(1);
}