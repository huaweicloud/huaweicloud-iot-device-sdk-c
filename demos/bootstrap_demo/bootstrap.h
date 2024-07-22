/*
 * Copyright (c) 2022-2024 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#ifndef __BOOTSTREP_H
#define __BOOTSTREP_H

// Configure device groups
typedef struct {
    char *scopeId;
} BOOTSTEAP_GROUPS;

// Configure certificates
typedef struct {
    char *privateKeyPassword;
} BOOTSTEAP_CERT;

typedef struct {
    char *bootstrapAddress;
    char *port;
    char *deviceId;
    char *password;
    char *baseStrategyKeyword;
    BOOTSTEAP_GROUPS groups;
    BOOTSTEAP_CERT cert; 
} BOOTSTEAP_INIT;

/**
 * @Description: bootstrap auth
 * @param parameter: BOOTSTEAP_INIT structure
 * @param bootstrapMode: 0 for Registered Devices, 1 for Registration Groups
 * @param authMode : 0 for secret, 1 for X.509 certificate
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
int bootstrapAuth(BOOTSTEAP_INIT parameter, int bootstrapMode, int authMode);

#endif /* __BOOTSTREP_H */