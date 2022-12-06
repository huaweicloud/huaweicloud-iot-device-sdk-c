/* Copyright (c) <2020>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * &Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  */

#include "base.h"
#include "callback_func.h"
#include "stdio.h"
#include "iota_init.h"
#include "iota_cfg.h"
#include "iota_error_type.h"

#include "rule_trans.h"
#include "log_util.h"

HW_API_FUNC HW_INT IOTA_Init(HW_CHAR *pcWorkPath)
{
    if(!RuleMgr_Init()) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "can't create rule info list\n");
	}

    return init(pcWorkPath);
}

HW_API_FUNC HW_INT IOTA_Destroy()
{
    return destory();
}

HW_API_FUNC HW_INT IOTA_ConfigSetStr(HW_INT iItem, HW_CHAR *pValue)
{
    return SetConfig(iItem, pValue);
}

HW_API_FUNC HW_INT IOTA_ConfigSetUint(HW_INT iItem, HW_UINT uiValue)
{
    char str[10];
    sprintf_s(str, sizeof(str), "%u", uiValue);
    return SetConfig(iItem, str);
}

HW_API_FUNC HW_VOID IOTA_SetPrintLogCallback(PFN_LOG_CALLBACK_HANDLER pfnLogCallbackHandler)
{
    SetLogCallback(pfnLogCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetEventCallback(PFN_EVENT_CALLBACK_HANDLER pfnCallbackHandler)
{
    SetEventCallback(pfnCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetCmdCallback(PFN_CMD_CALLBACK_HANDLER pfnCallbackHandler)
{
    RuleMgr_SetCommandCallbackHandler(pfnCallbackHandler);
    SetCmdCallback(pfnCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetCmdCallbackV3(PFN_CMD_CALLBACK_HANDLER_V3 pfnCallbackHandler)
{
    SetCmdCallbackV3(pfnCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetProtocolCallback(HW_INT iItem, PFN_PROTOCOL_CALLBACK_HANDLER pfnCallbackHandler)
{
    SetProtocolCallback(iItem, pfnCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetMessageCallback(PFN_MESSAGE_CALLBACK_HANDLER pfnCallbackHandler)
{
    SetMessageCallback(pfnCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetPropSetCallback(PFN_PROP_SET_CALLBACK_HANDLER pfnCallbackHandler)
{
    SetPropSetCallback(pfnCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetPropGetCallback(PFN_PROP_GET_CALLBACK_HANDLER pfnCallbackHandler)
{
    SetPropGetCallback(pfnCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetShadowGetCallback(PFN_SHADOW_GET_CALLBACK_HANDLER pfnCallbackHandler)
{
    SetShadowGetCallback(pfnCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetUserTopicMsgCallback(PFN_USER_TOPIC_MSG_CALLBACK_HANDLER pfnCallbackHandler)
{
    SetUserTopicMsgCallback(pfnCallbackHandler);
}

HW_API_FUNC HW_VOID IOTA_SetBootstrapCallback(PFN_BOOTSTRAP_CALLBACK_HANDLER pfnCallbackHandler)
{
    SetBootstrapCallback(pfnCallbackHandler);
}
