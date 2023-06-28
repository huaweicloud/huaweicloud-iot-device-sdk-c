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

#ifndef GENERIC_TCP_PROTOCOL_H
#define GENERIC_TCP_PROTOCOL_H

#define TCP_SERVER_IP       "127.0.0.1"
#define TCP_SERVER_PORT     15623
#define MAX_MESSAGE_BUF_LEN 30
#define BACK_LOG            20   // Maximum length of sub device connection request
#define WORK_PATH           "."
#define CONNECTED           0
#define DISCONNECTED        1

int CreateServerSocket(char *serverIp, int serverPort, int backlog);
void TimeSleep(int ms);

void ProcessMessageFromClient(int clientSocket);
void SendReportToIoTPlatform(char *recvBuf);

void SendCommandRspToIoTPlatform(char *requestId);
void SendMessageToSubDevice(int clientSocket, char *payload);

void HandleSubDeviceManagerEvents(EN_IOTA_SERVICE_EVENT *subDeviceServices);
void AddSubDevice(EN_IOTA_DEVICE_PARAS *paras);
void AddSubDevice(EN_IOTA_DEVICE_PARAS *paras);
void DeleteSubDevice(EN_IOTA_DEVICE_PARAS *paras);

void HandleOTAEvents(EN_IOTA_SERVICE_EVENT *otaServices);
void QueryVersion(void);
void UpgradeFirmware(EN_IOTA_OTA_PARAS *paras);
void UpgradeSoftware(EN_IOTA_OTA_PARAS *paras);

char *EncodeCommandParas(EN_IOTA_COMMAND *command);
ST_IOTA_SERVICE_DATA_INFO *DecodeServiceProperty(char *recvBuf);

#endif /* INCLUDE_PROTOCOL_GENERIC_TCP_PROTOCOL_H_ */

