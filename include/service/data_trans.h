/*Copyright (c) <2020>, <Huawei Technologies Co., Ltd>
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
 * */

#ifndef INCLUDE_DATATRANS_H_
#define INCLUDE_DATATRANS_H_

#define TOPIC_PREFIX_UP 				"$oc/devices/"
#define TOPIC_SUFFIX_MESSAGEUP 			"/sys/messages/up"
#define TOPIC_SUFFIX_USER_UP 			"/user/"
#define TOPIC_SUFFIX_PROP_UP 			"/sys/properties/report"
#define TOPIC_SUFFIX_PROPS_UP 			"/sys/gateway/sub_devices/properties/report"
#define TOPIC_SUFFIX_COMMAND_RSP_UP 	"/sys/commands/response/request_id="
#define TOPIC_SUFFIX_PROP_SET_RSP_UP 	"/sys/properties/set/response/request_id="
#define TOPIC_SUFFIX_PROP_GET_RSP_UP 	"/sys/properties/get/response/request_id="
#define TOPIC_SUFFIX_SHADOW_UP 			"/sys/shadow/get/request_id="
#define TOPIC_SUFFIX_EVENT_UP 			"/sys/events/up"
#define TOPIC_SUFFIX_SUB_DEVICE_INFO_UP "/sys/sub_device_manage/messages/up"

_DLLEXPORT int ReportDeviceData(char *payload, char *topicParas);
_DLLEXPORT int ReportDeviceProperties(char *payload);
_DLLEXPORT int ReportBatchDeviceProperties(char *payload);
_DLLEXPORT int ReportData(char *topic, char *payload);
_DLLEXPORT int ReportCommandReponse(char *requestId, char *pcCommandRespense);
_DLLEXPORT int ReportPropSetReponse(char *requestId, char *pcCommandRespense);
_DLLEXPORT int ReportPropGetReponse(char *requestId, char *pcCommandRespense);
_DLLEXPORT int GetPropertiesRequest(char *requestId, char *pcCommandRespense);
_DLLEXPORT int ReportSubDeviceInfo(char *payload);
_DLLEXPORT int EventUp(char *payload);

#endif /* INCLUDE_DATATRANS_H_ */
