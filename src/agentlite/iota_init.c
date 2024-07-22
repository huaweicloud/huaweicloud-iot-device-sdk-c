/*
 * Copyright (c) 2020-2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#include "base.h"
#include "callback_func.h"
#include "iota_cfg.h"
#include "iota_error_type.h"
#include "securec.h"
#include "rule_trans.h"
#include "rule_manager.h"
#include "log_util.h"
#include "iota_init.h"

#define MAX_UINT_DIGIT_LENGTH 10
HW_API_FUNC HW_INT IOTA_Init(HW_CHAR *workPath)
{
    if (!RuleMgr_Init()) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "can't create rule info list\n");
    }

    return init(workPath);
}

HW_API_FUNC HW_INT IOTA_Destroy()
{
    return destory();
}

static void GetConnectEnv()
{
    char *ip = getenv("IOTDA_MQTTC_ADDRESS");
    if (ip && strlen (ip) != 0) {
        SetConfig(EN_IOTA_CFG_MQTT_ADDR, ip);
    }
    
    char *port = getenv("IOTDA_MQTTC_PORT");
    if (port && strlen (port) != 0) {
        SetConfig(EN_IOTA_CFG_MQTT_PORT, port);
    }

    char *deviceId = getenv("IOTDA_MQTTC_DEVICEID");
    if (deviceId && strlen (deviceId) != 0) {
    SetConfig(EN_IOTA_CFG_DEVICEID, deviceId);
    }

    char *pasw = getenv("IOTDA_MQTTC_PASSWOROD");
    if (pasw && strlen (pasw) != 0) {
        SetConfig(EN_IOTA_CFG_DEVICESECRET, pasw);
    }
}

HW_API_FUNC HW_INT IOTA_ConnectConfigSet(HW_CHAR *ip, HW_CHAR *port, HW_CHAR *deviceId, HW_CHAR *password) 
{
    if (ip == NULL || port == NULL || deviceId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_ConnectConfigSet err, ip or port or deviceId == NULL\n");
        return;
    }
    SetConfig(EN_IOTA_CFG_MQTT_ADDR, ip);
    RuleMgr_SetCurrentUserName(deviceId);
    SetConfig(EN_IOTA_CFG_DEVICEID, deviceId);
    SetConfig(EN_IOTA_CFG_MQTT_PORT, port);
    if (password != NULL) {
        SetConfig(EN_IOTA_CFG_DEVICESECRET, password);
    }
#ifdef GLOBAL_VAR_CONFIG
    GetConnectEnv();
#endif
}

HW_API_FUNC HW_INT IOTA_ConfigSetStr(HW_INT item, HW_CHAR *value)
{
    if (item == EN_IOTA_CFG_DEVICEID) {
        RuleMgr_SetCurrentUserName(value);
    }
    return SetConfig(item, value);
}

HW_API_FUNC HW_INT IOTA_ConfigSetUint(HW_INT item, HW_UINT value)
{
    char str[MAX_UINT_DIGIT_LENGTH] = {0};
    (void)sprintf_s(str, sizeof(str), "%u", value);
    return SetConfig(item, str);
}

HW_API_FUNC HW_VOID IOTA_SetPrintLogCallback(PFN_LOG_CALLBACK_HANDLER pfnLogCallbackHandler)
{
    SetLogCallback(pfnLogCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetEventCallback(PFN_EVENT_CALLBACK_HANDLER callbackHandler)
{
    SetEventCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetCmdCallback(PFN_CMD_CALLBACK_HANDLER callbackHandler)
{
    RuleMgr_SetCommandCallbackHandler(callbackHandler);
    SetCmdCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetCmdCallbackV3(PFN_CMD_CALLBACK_HANDLER_V3 callbackHandler)
{
    SetCmdCallbackV3(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetProtocolCallback(EN_IOTA_CALLBACK_SETTING item,
    PFN_PROTOCOL_CALLBACK_HANDLER callbackHandler)
{
    switch (item) {
        case EN_IOTA_CALLBACK_CONNECT_SUCCESS:
            SetProtocolCallback(EN_CALLBACK_CONNECT_SUCCESS, callbackHandler);
            break;
        case EN_IOTA_CALLBACK_CONNECT_FAILURE:
            SetProtocolCallback(EN_CALLBACK_CONNECT_FAILURE, callbackHandler);
            break;
        case EN_IOTA_CALLBACK_DISCONNECT_SUCCESS:
            SetProtocolCallback(EN_CALLBACK_DISCONNECT_SUCCESS, callbackHandler);
            break;
        case EN_IOTA_CALLBACK_DISCONNECT_FAILURE:
            SetProtocolCallback(EN_CALLBACK_DISCONNECT_FAILURE, callbackHandler);
            break;
        case EN_IOTA_CALLBACK_CONNECTION_LOST:
            SetProtocolCallback(EN_CALLBACK_CONNECTION_LOST, callbackHandler);
            break;
        case EN_IOTA_CALLBACK_PUBLISH_SUCCESS:
            SetProtocolCallback(EN_CALLBACK_PUBLISH_SUCCESS, callbackHandler);
            break;
        case EN_IOTA_CALLBACK_PUBLISH_FAILURE:
            SetProtocolCallback(EN_CALLBACK_PUBLISH_FAILURE, callbackHandler);
            break;
        case EN_IOTA_CALLBACK_SUBSCRIBE_SUCCESS:
            SetProtocolCallback(EN_CALLBACK_SUBSCRIBE_SUCCESS, callbackHandler);
            break;
        case EN_IOTA_CALLBACK_SUBSCRIBE_FAILURE:
            SetProtocolCallback(EN_CALLBACK_SUBSCRIBE_FAILURE, callbackHandler);
            break;
        default:
            PrintfLog(EN_LOG_LEVEL_WARNING,
                "iota_init: IOTA_SetProtocolCallback() warning, the item (%d) to be set is not available\n", item);
    }
}

HW_API_FUNC HW_VOID IOTA_SetMessageCallback(PFN_MESSAGE_CALLBACK_HANDLER callbackHandler)
{
    SetMessageCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetRawMessageCallback(PFN_RAW_MESSAGE_CALLBACK_HANDLER callbackHandler)
{
    SetRawMessageCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetPropSetCallback(PFN_PROP_SET_CALLBACK_HANDLER callbackHandler)
{
    SetPropSetCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetPropGetCallback(PFN_PROP_GET_CALLBACK_HANDLER callbackHandler)
{
    SetPropGetCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetShadowGetCallback(PFN_SHADOW_GET_CALLBACK_HANDLER callbackHandler)
{
    SetShadowGetCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetUserTopicMsgCallback(PFN_USER_TOPIC_MSG_CALLBACK_HANDLER callbackHandler)
{
    SetUserTopicMsgCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetBridgesDeviceLoginCallback(PFN_BRIDGES_DEVICE_LOGIN callbackHandler)
{
    SetBridgesDeviceLoginCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetBridgesDeviceLogoutCallback(PFN_BRIDGES_DEVICE_LOGOUT callbackHandler)
{
    SetBridgesDeviceLogoutCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetBridgesDeviceResetSecretCallback(PFN_BRIDGES_DEVICE_RESET_SECRET callbackHandler)
{
    SetBridgesDeviceResetSecretCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetBridgesDeviceDisConnCallback(PFN_BRIDGES_DEVICE_PLATE_DISCONNECT callbackHandler)
{
    SetBridgesDevicePalletDisConnCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetUndefinedMessageCallback(PFN_UNDEFINED_MSG_CALLBACK_HANDLER callbackHandler)
{
    SetUndefinedMessageCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetUserTopicRawMsgCallback(PFN_USER_TOPIC_RAW_MSG_CALLBACK_HANDLER callbackHandler)
{
    SetUserTopicRawMsgCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetBootstrapCallback(PFN_BOOTSTRAP_CALLBACK_HANDLER callbackHandler)
{
    SetBootstrapCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetDeviceRuleSendMsgCallback(PFN_DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER callbackHandler)
{
    SetDeviceRuleSendMsgCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetM2mCallback(PFN_M2M_CALLBACK_HANDLER callbackHandler)
{
    SetM2mCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_EnableDeviceRuleStorage(const char *filepath)
{
    RuleMgr_EnableDeviceRuleStorage(filepath);
}

HW_API_FUNC HW_VOID IOTA_SetDeviceConfigCallback(PFN_DEVICE_CONFIG_CALLBACK_HANDLER callbackHandler)
{
    SetDeviceConfigCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetEvenSubDeviceCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    SetEvenSubDeviceCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetEvenOtaVersionUpCallback(OTAVERSION_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    SetEvenOtaVersionUpCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetEvenOtaUrlResponseCallback(OTAURL_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    SetEvenOtaUrlResponseCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetNtpCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    SetNtpCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetEvenDeviceLogCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    SetEvenDeviceLogCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetEvenSoftBusCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    SetEvenSoftBusCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetEvenTunnelManagerCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    SetEvenTunnelManagerCallback(callbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetEvenFileManagerCallback(EVENT_CALLBACK_HANDLER_SPECIFY callbackHandler)
{
    SetEvenFileManagerCallback(callbackHandler);
}


