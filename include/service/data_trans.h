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

#ifndef DATATRANS_H
#define DATATRANS_H
#include "mqttv5_util.h"

int ReportDeviceData(char *payload, char *topicParas, int compressFlag, void *context, void *properties);
int ReportDeviceProperties(char *payload, int compressFlag, void *context, void *properties);
int ReportBatchDeviceProperties(char *payload, int compressFlag, void *context, void *properties);
int ReportData(char *topic, char *payload, void *context, void *properties);
int ReportCommandReponse(char *requestId, char *commandResponse, void *context, void *properties);
int ReportPropSetReponse(char *requestId, char *commandResponse, void *context);
int ReportPropGetReponse(char *requestId, char *commandResponse, void *context);
int GetPropertiesRequest(char *requestId, char *commandResponse, void *context);
int ReportSubDeviceInfo(char *payload, void *context);
int EventUp(char *payload, void *context);
int ReportDevicePropertiesV3(char *payload, int codecMode, void *context);
int BinaryReportV3(char *payload);
int Bootstrap(char *payload);
int OCM2MSendMsg(char *to, char *from, char *payload, char *requestId, char *context);

#endif /* DATATRANS_H */
