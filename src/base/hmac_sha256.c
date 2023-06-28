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

#include <string.h>
#include "openssl/ossl_typ.h"
#include "openssl/hmac.h"
#include "log_util.h"
#include "securec.h"
#include "string_util.h"
#include "iota_error_type.h"
#include "iota_cfg.h"
#include "hmac_sha256.h"

int EncryptWithHMac(const char *inputData, char **inputKey, int inEncryDataLen, char *outData, int checkTimestamp)
{
    if ((inputData == NULL) || ((*inputKey) == NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "EncryptWithHMac(): the input is invalid.\n");
        return IOTA_FAILURE;
    }

    if (TIME_STAMP_LENGTH != strlen(*inputKey)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "EncryptWithHMac(): the length of inputKey is invalid.\n");
        return IOTA_FAILURE;
    }

    char *end = NULL;
    unsigned int macLength = 0;
    unsigned int tryTime = 1;
    size_t lenData = strlen(inputData);
    long timeTmp = strtol(*inputKey, &end, 10);
    unsigned char *temp = NULL;
    if (checkTimestamp <= EN_IOTA_CFG_CHECK_STAMP_SHA256) {
        temp = HMAC(EVP_sha256(), *inputKey, TIME_STAMP_LENGTH, (const unsigned char *)inputData, lenData,
            NULL, &macLength);
    } else {
        temp = HMAC(EVP_sm3(), *inputKey, TIME_STAMP_LENGTH, (const unsigned char *)inputData, lenData,
            NULL, &macLength);
    }

    while (strlen(temp) != SHA256_ENCRYPTION_LENGRH) {
        tryTime++;
        if (tryTime > TRY_MAX_TIME) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "EncryptWithHMac(): Encryption failed after max times attempts.\n");
            return IOTA_FAILURE;
        }

        timeTmp++;
        (void)snprintf_s(*inputKey, TIME_STAMP_LENGTH + 1, TIME_STAMP_LENGTH, "%ld", timeTmp);
        if (checkTimestamp <= EN_IOTA_CFG_CHECK_STAMP_SHA256) {
            temp = HMAC(EVP_sha256(), *inputKey, TIME_STAMP_LENGTH, (const unsigned char *)inputData, lenData,
                NULL, &macLength);
        } else {
            temp = HMAC(EVP_sm3(), *inputKey, TIME_STAMP_LENGTH, (const unsigned char *)inputData, lenData,
                NULL, &macLength);
        }
    }

    int index;
    int loop;
    char hexChar;

    for (index = 0, loop = 0; loop < inEncryDataLen; loop++) {
        hexChar = (temp[loop] >> 4) & 0x0F;
        outData[index++] = (hexChar <= 9) ? (hexChar + '0') : (hexChar + 'a' - 10);

        hexChar = temp[loop] & 0x0F;
        outData[index++] = (hexChar <= 9) ? (hexChar + '0') : (hexChar + 'a' - 10);
    }

    outData[index] = '\0';

    return IOTA_SUCCESS;
}
