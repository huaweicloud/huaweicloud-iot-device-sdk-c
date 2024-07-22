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

#ifndef SUBSCRIBE_H
#define SUBSCRIBE_H

int SubscribeM2m(void);
int SubscribeCommand(void);
void SubscribeAll(void);
int SubscribeMessageDown(void);
int SubscribePropSet(void);
int SubscribePropget(void);
int SubscribePropResp(void);
int SubscribeSubDeviceEvent(void);
int SubscribeUserTopic(char *topicParas);
int SubscribeJsonCmdV3(void);
int SubscribeBinaryCmdV3(void);
int SubscribeBootstrap(void);


// qos
void SubscribeAllQos(int qos);
int SubscribeCustomTopic(char *topic, const int qos); 
int SubscribeM2mQos(int qos);
int SubscribeCommandQos(int qos);
void SubscribeAllQos(int qos);
int SubscribeMessageDownQos(int qos);
int SubscribePropSetQos(int qos);
int SubscribePropgetQos(int qos);
int SubscribePropRespQos(int qos);
int SubscribeSubDeviceEventQos(int qos);
int SubscribeUserTopicQos(char *topicParas, int qos);
int SubscribeJsonCmdV3Qos(int qos);
int SubscribeBinaryCmdV3Qos(int qos);
int SubscribeBootstrapQos(int qos);


#endif /* SUBSCRIBE_H */
