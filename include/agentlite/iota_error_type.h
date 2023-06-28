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

#ifndef IOTA_ERROR_TYPE_H
#define IOTA_ERROR_TYPE_H

#define IOTA_SUCCESS                     (0)
#define IOTA_FAILURE                     (-1)
#define IOTA_PARAMETER_EMPTY             (-101)
#define IOTA_RESOURCE_NOT_AVAILABLE      (-102)
#define IOTA_INITIALIZATION_REPEATED     (-103)
#define IOTA_LIBRARY_LOAD_FAILED         (-104)
#define IOTA_SECRET_ENCRYPT_FAILED       (-105)
#define IOTA_MQTT_CONNECT_FAILED         (-106)
#define IOTA_MQTT_CONNECT_EXISTED        (-107)
#define IOTA_CERTIFICATE_NOT_FOUND       (-108)
#define IOTA_MQTT_DISCONNECT_FAILED      (-109)
#define IOTA_PARSE_JSON_FAILED           (-110)
#define IOTA_PARAMETER_ERROR             (-111)
#define IOTA_NUMBER_EXCEEDS              (-112)
#define IOTA_WSS_CONNECT_FAILED          (-113)
#define IOTA_THREAD_CREATE_FAILED        (-114)

#endif

