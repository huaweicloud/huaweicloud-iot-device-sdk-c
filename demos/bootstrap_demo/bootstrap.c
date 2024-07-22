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

static char *g_accessAddress = NULL; // The IOTDA address returned by device provision. No need for users to enter.
static char *g_accessPassword = NULL; // The device access password returned by device provision using bootstrap group. No need for users to enter.

// Device Provision Callback
static void HandleBootstrap(EN_IOTA_MQTT_PROTOCOL_RSP *rsp) {
    PrintfLog(EN_LOG_LEVEL_INFO, "bootstrap_demo: HandleBootstrap(), address is %s\n", rsp->message);
    int address_length = GetSubStrIndex(rsp->message, ":") + 1;
    if (CopyStrValue(&g_accessAddress, (const char *)rsp->message, address_length - 1) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "HandleBootstrap(): there is not enough memory here.\n");
        return;
    }

    if (rsp->deviceSecret != NULL) {
        if (CopyStrValue(&g_accessPassword, (const char *)rsp->deviceSecret, strlen(rsp->deviceSecret)) < 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "HandleBootstrap(): there is not enough memory here.\n");
            return;
        }
    }
    
}

static void SetBootstrapAuthConfig(BOOTSTEAP_INIT parameter, int mode) 
{
    // Load connection parameters
    IOTA_ConnectConfigSet(parameter.bootstrapAddress, parameter.port, parameter.deviceId, parameter.password);

    // Set authentication method. mode: 0, secret authentication. mode: not 0, certificate authentication
    if (mode == EN_IOTA_CFG_AUTH_MODE_SECRET) {
        IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);
        if (parameter.password) {
            IOTA_ConfigSetStr(EN_IOTA_CFG_BS_GROUP_SECRET, parameter.password); 
        }
    } else {
        IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_CERT);
        IOTA_ConfigSetUint(EN_IOTA_CFG_PRIVATE_KEY_PASSWORD, parameter.cert.privateKeyPassword);
    }
    if (parameter.groups.scopeId) {  
        IOTA_ConfigSetStr(EN_IOTA_CFG_BS_SCOPE_ID, parameter.groups.scopeId);
    }
}

// Device Provision
int bootstrapAuth(BOOTSTEAP_INIT parameter, int bootstrapMode, int authMode) {
    
    SetBootstrapAuthConfig(parameter, authMode);
    if (bootstrapMode) {
        // Set device provision configuration as bootstrap group
        IOTA_ConfigSetUint(EN_IOTA_CFG_BS_MODE, EN_IOTA_CFG_BS_SELF_REG);
    }

    IOTA_SetBootstrapCallback(HandleBootstrap);
     int ret = IOTA_Connect();
     if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: IOTA_Connect() error, Auth failed, result %d\n", ret);
        return -1;
    }

    TimeSleep(1500);
    while (!IOTA_IsConnected());

    // Subscribe devices provision downstream topic
    IOTA_SubscribeBoostrap();
    TimeSleep(3000);

    // Obtain the IoTDA connection address, which is contained in the IOTDP callback.
    if (IOTA_Bootstrap(parameter.baseStrategyKeyword) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: IOTA_Bootstrap() error \n");
        return -1;
    };
    TimeSleep(5000);
    IOTA_Destroy();

    // Clear connection data
    IOTA_Init("."); 
    parameter.bootstrapAddress = g_accessAddress;
    if (bootstrapMode) {
        parameter.password = g_accessPassword;
    }

    SetBootstrapAuthConfig(parameter, authMode);
    IOTA_ConfigSetStr(EN_IOTA_CFG_MQTT_ADDR, parameter.bootstrapAddress);
    ret = IOTA_Connect();
    if (ret != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "bootstrap_demo: IOTA_Connect() error, Auth failed, result %d\n", ret);
    }
    TimeSleep(1500);
    if (IOTA_IsConnected()) {
        return 0;
    }
    return -1;
} 
