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

#ifndef INCLUDE_SUBSCRIBE_H_
#define INCLUDE_SUBSCRIBE_H_

#define TOPIC_PREFIX 				"$oc/devices/"
#define TOPIC_SUFFIX_MESSAGEDOWN 	"/sys/messages/down"
#define TOPIC_SUFFIX_COMMAND 		"/sys/commands/"
#define TOPIC_SUFFIX_PROP_SET 		"/sys/properties/set/"
#define TOPIC_SUFFIX_PROP_GET 		"/sys/properties/get/"
#define TOPIC_SUFFIX_PROP_RSP 		"/sys/shadow/get/response/"
#define TOPIC_SUFFIX_EVENT_DOWN 	"/sys/events/down"
#define TOPIC_SUFFIX_USER 			"/user/"
#define WILDCARD 					"#"

_DLLEXPORT int SubscribeCommand(void);
_DLLEXPORT void SubscribeAll(void);
_DLLEXPORT int SubscribeMessageDown(void);
_DLLEXPORT int SubscribePropSet(void);
_DLLEXPORT int SubscribePropget(void);
_DLLEXPORT int SubscribePropResp(void);
_DLLEXPORT int SubscribeSubDeviceEvent(void);
_DLLEXPORT int SubscribeUserTopic(char *topicParas);
_DLLEXPORT int SubsribeTopic(char *topic);

#endif /* INCLUDE_SUBSCRIBE_H_ */
