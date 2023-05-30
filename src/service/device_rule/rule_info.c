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

#include <string.h>
#include <stdlib.h>
#include "hw_type.h"
#include "securec.h"
#include "string_util.h"
#include "rule_info.h"

HW_BOOL TimeRangeCtor(TimeRange *self)
{
    return (memset_s(self, sizeof(*self), 0, sizeof(*self)) == EOK);
}

void TimeRangeDtor(TimeRange *self)
{
    (void)self;
}

HW_BOOL TimeRangeIsNuLL(const TimeRange *self)
{
    return (self->daysOfWeek == 0);
}

HW_BOOL RuleInfoCtor(RuleInfo *self)
{
    (void)memset_s(self, sizeof(*self), 0, sizeof(*self));
    if (!TimeRangeCtor(&self->timeRange)) {
        goto RuleInfoRelaseWhenCtorFailedAtTimeRange;
    }
    if (!ConditionListCtor(&self->conditions)) {
        goto RuleInfoRelaseWhenCtorFailedAtConditionList;
    }
    if (!ActionListCtor(&self->actions)) {
        goto RuleInfoRelaseWhenCtorFailedAtActionList;
    }
    return HW_TRUE;

RuleInfoRelaseWhenCtorFailedAtActionList:
    ConditionListDtor(&self->conditions);
RuleInfoRelaseWhenCtorFailedAtConditionList:
    TimeRangeDtor(&self->timeRange);
RuleInfoRelaseWhenCtorFailedAtTimeRange:
    return HW_FALSE;
}

void RuleInfoDtor(RuleInfo *self)
{
    MemFree(&self->ruleId);
    MemFree(&self->ruleName);
    MemFree(&self->logic);
    TimeRangeDtor(&self->timeRange);
    MemFree(&self->status);
    ConditionListDtor(&self->conditions);
    ActionListDtor(&self->actions);
}

DECL_DYARRY_FUNC_UTIL_IMPL(RuleInfoList, RuleInfo, elements, RuleInfoCtor, RuleInfoDtor)
