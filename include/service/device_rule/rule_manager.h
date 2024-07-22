/*
 * Copyright (c) 2022-2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#ifndef RULE_MANAGER_H
#define RULE_MANAGER_H

#include "iota_init.h"
#include "hw_type.h"
#include "rule_info.h"
#include "callback_func.h"
#include "cJSON.h"

#ifdef DEVICE_RULE_ENALBE

HW_BOOL RuleMgr_Init(void);
void RuleMgr_Destroy(void);
void RuleMgr_Parse(const char *payload);
void RuleMgr_ParseJSON(const cJSON *payload);
void RuleMgr_CheckAndExecuteNoTimers(void);
void RuleMgr_SetSendMsgCallback(PFN_DEVICE_RULE_SEND_MSG_CALLBACK_HANDLER cb);
void RuleMgr_SetCommandCallbackHandler(PFN_CMD_CALLBACK_HANDLER cb);
HW_BOOL RuleMgr_SetCurrentUserName(const char *username);
void RuleMgr_CachePropertiesValue(const cJSON *pServiceData);
HW_BOOL RuleMgr_GetList(const cJSON *properties, RuleInfoList *delList, RuleInfoList *addList);
void RuleMgr_DelRule(RuleInfoList *delList);

#else //  DEVICE_RULE_ENALBE

#define RuleMgr_Init() HW_FALSE
#define RuleMgr_Destroy()
#define RuleMgr_Parse(payload)
#define RuleMgr_ParseJSON(payload)
#define RuleMgr_CheckAndExecuteNoTimers()
#define RuleMgr_SetSendMsgCallback(cb)
#define RuleMgr_SetCommandCallbackHandler(cb)
#define RuleMgr_SetCurrentUserName(username) HW_TRUE
#define RuleMgr_CachePropertiesValue(pServiceData)
#define RuleMgr_GetList(properties, delList, addList) HW_TRUE
#define RuleMgr_DelRule(delList)
#define RuleMgr_EnableDeviceRuleStorage(filepath)

#endif // DEVICE_RULE_ENALBE

#ifdef CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
void RuleMgr_EnableDeviceRuleStorage(const char *filepath);
#else // CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE
#define RuleMgr_EnableDeviceRuleStorage(filepath)
#endif //  CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE

#endif