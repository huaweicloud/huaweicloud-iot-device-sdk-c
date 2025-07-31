/*
 * Copyright (c) 2024-2025 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#include "report_anomaly.h"
#include "string_util.h"
#include "log_util.h"

/*
 * 线下异常数据上报接口示例
 * 使用前请在Makefile中打开SECURITY_AWARENESS_ENABLE := 1注释
 */

// You can get the access address from IoT Console "Overview" -> "Access Information"
char *g_address = "XXXXX"; 
char *g_port = "8883";

// deviceId, the mqtt protocol requires the user name to be filled in.
// Please fill in the deviceId
char *g_deviceId = "XXXXXX"; 
char *g_secret = "XXXXXX";

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

#if defined (SECURITY_AWARENESS_ENABLE)
// 上报不安全功能
static void Test_ReportUnsafeFunction()
{
    int contentNum = 2;
    ANOMALY_UNSAFE_FUNCTION unsafeFunction[contentNum];
    
    unsafeFunction[0].name = "root login";
    unsafeFunction[0].desc = "Enable all 0 monitoring on the device";
    unsafeFunction[1].name = "all zero monitor";
    unsafeFunction[1].desc = "Enable root SSH login for the device";

    IOTA_AnomalyUnsafeFunctionReport(unsafeFunction, contentNum);
}

static void Test_ReportProtocolReport()
{
    int contentNum = 2;
    ANOMALY_PROTOCOL unsafeProtocol[contentNum];

    unsafeProtocol[0].protocol = "mqtt";
    unsafeProtocol[0].version = "5.0";
    unsafeProtocol[0].desc = "Not allowed agreement";
    unsafeProtocol[1].protocol = "ftp";
    unsafeProtocol[1].version = "FTP-5";
    unsafeProtocol[1].desc = "Not allowed agreemen ftp";

    IOTA_AnomalyProtocolReport(unsafeProtocol, contentNum);
}

static void Test_ReportCveVnlnerability()
{
    int contentNum = 2;
    ANOMALY_CVE_VNLNERABILITY cveLoophole[contentNum];

    cveLoophole[0].cve_number = "CVE-2024-11452";
    cveLoophole[0].vulnerability_name = "WeGIA 在文件上传字段中存在跨站点脚本 (XSS) (CVE-2025-22132)";
    cveLoophole[0].desc = NULL;
    cveLoophole[1].cve_number = "CVE-2024-11453";
    cveLoophole[1].vulnerability_name = "WeGIA 在文件上传字段中存在跨站点脚本 (XSS) (CVE-2024-11453)";
    cveLoophole[1].desc = NULL;

    IOTA_AnomalyCveValnerabilityReport(cveLoophole, contentNum);
}

static void Test_ReportMaliciousFile()
{
    int contentNum = 1;
    ANOMALY_MALICIOUS_FILE maliciousFile[contentNum];

    maliciousFile[0].file = "/opt/gardener/docs/backup/24330/default/hw-iot-edge/gateway-manager.yaml";
    maliciousFile[0].desc = NULL;

    IOTA_AnomalyFileReport(maliciousFile, contentNum);
}

static void Test_ReportAbnormalProcess()
{
    int contentNum = 1;
    ANOMALY_ABNORMAL_PROCES abnormalProcess[contentNum];

    abnormalProcess[0].pid = "12534";
    abnormalProcess[0].cmd = "python /usr/bin/osmt_server";
    abnormalProcess[0].desc = NULL;

    IOTA_AnomalyAbnormalProcessReport(abnormalProcess, contentNum);
}
#endif

// ---------------------------- secret authentication --------------------------------------
static void mqttDeviceSecretInit(char *address, char *port, char *deviceId, char *password) 
{
    IOTA_Init("."); // The certificate address is ./conf/rootcert.pem
    IOTA_SetPrintLogCallback(MyPrintLog); 
 
    // MQTT protocol when the connection parameter is 1883; MQTTS protocol when the connection parameter is 8883
    IOTA_ConnectConfigSet(address, port, deviceId, password);

    // Set connection callback function
    IOTA_DefaultCallbackInit();
   
}

int main(int argc, char **argv) 
{
    // secret authentication initialization
    mqttDeviceSecretInit(g_address, g_port, g_deviceId, g_secret); 

    int ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "message_test: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }
    TimeSleep(1500);

    while (!IOTA_IsConnected()) {
        TimeSleep(300);
    }

#if defined (SECURITY_AWARENESS_ENABLE)
    Test_ReportUnsafeFunction();
    TimeSleep(1000);

    Test_ReportProtocolReport();
    TimeSleep(1000);

    Test_ReportCveVnlnerability();
    TimeSleep(1000);

    Test_ReportMaliciousFile();
    TimeSleep(1000);

    Test_ReportAbnormalProcess();
    TimeSleep(1000);

#endif

    TimeSleep(60000);
    IOTA_Destroy();
}
