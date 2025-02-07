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

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <libgen.h>
#include <string.h>
#include <linux/limits.h>
#include "securec.h"
#include "string_util.h"
#include "log_util.h"
#include "base.h"
#include "hw_type.h"
#include "data_trans.h"
#include "cJSON.h"
#include "iota_error_type.h"
#include "subscribe.h"
#include "limits.h"
#include "rule_trans.h"
#include "rule_manager.h"
#include "iota_datatrans.h"
#include "curl/curl.h"
#include "mqttv5_util.h"
#include "iota_payload.h"

HW_API_FUNC HW_INT IOTA_MessageReport(HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content,
    HW_CHAR *topicParas, HW_INT compressFlag, void *context)
{
    if (content == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the content cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    int messageId = 0;
    char *payload = IOTA_MessageReportPayload(object_device_id, name, id, content);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDeviceData(payload, topicParas, compressFlag, context, NULL);
        MemFree(&payload);
    }
    return messageId;
}

HW_API_FUNC HW_INT IOTA_MessageDataReport(ST_IOTA_MESS_REP_INFO mass, void *context)
{
    return IOTA_MessageReport(mass.object_device_id, mass.name, mass.id, mass.content, mass.topicParas, 0, context);
}

#if defined(MQTTV5)

HW_API_FUNC HW_INT IOTA_MessageReportV5(ST_IOTA_MESS_REP_INFO mass, HW_INT compressFlag, void *context,
    MQTTV5_DATA *mqttv5)
{
    if (mass.content == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the content cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char *payload = IOTA_MessageReportPayload(mass.object_device_id, mass.name, mass.id, mass.content);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDeviceData(payload, mass.topicParas, compressFlag, context, (void *)mqttv5);
        MemFree(&payload);
    }
    return messageId;
}

HW_API_FUNC HW_INT IOTA_RawTopicMessageReportV5(HW_CHAR *topic, HW_CHAR *payload, int qos, void *context, MQTTV5_DATA *mqttv5)
{
    if (topic == NULL || payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot or topic be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char *topicParas = CombineStrings(1, topic);
    int messageId = ReportData(topicParas, payload, context, mqttv5);
    return messageId;
}
#endif

HW_API_FUNC HW_INT IOTA_RawTopicMessageReport(HW_CHAR *topic, HW_CHAR *payload, int qos, void *context)
{
    if (topic == NULL || payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload or topic be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    char *topicParas = CombineStrings(1, topic);
    int messageId = ReportDataSetQos(topicParas, payload, qos, context, NULL);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_PropertiesReport(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum,
    HW_INT compressFlag, void *context)
{
    char *payload = IOTA_PropertiesReportPayload(pServiceData, serviceNum);
    int messageId = 0;

    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDeviceProperties(payload, compressFlag, context, NULL);
        MemFree(&payload);
    }
    return messageId;
}

#if defined(MQTTV5)

HW_API_FUNC HW_INT IOTA_PropertiesReportV5(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum,
    HW_INT compressFlag, void *context, MQTTV5_DATA *mqttv5)
{
    char *payload = IOTA_PropertiesReportPayload(pServiceData, serviceNum);
    int messageId = 0;

    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDeviceProperties(payload, compressFlag, context, (void *)mqttv5);
        MemFree(&payload);
    }
    return messageId;
}
#endif

HW_API_FUNC HW_INT IOTA_BatchPropertiesReport(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum,
    HW_INT serviceLenList[], HW_INT compressFlag, void *context)
{
    char *payload = IOTA_BatchPropertiesReportPayload(pDeviceData, deviceNum, serviceLenList);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        int messageId = ReportBatchDeviceProperties(payload, compressFlag, context, NULL);
        MemFree(&payload);
        return messageId;
    }
}
#if defined(MQTTV5)

HW_API_FUNC HW_INT IOTA_BatchPropertiesReportV5(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum,
    HW_INT serviceLenList[], HW_INT compressFlag, void *context, MQTTV5_DATA *mqttv5)
{
    char *payload = IOTA_BatchPropertiesReportPayload(pDeviceData, deviceNum, serviceLenList);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        int messageId = ReportBatchDeviceProperties(payload, compressFlag, context, (void *)mqttv5);
        MemFree(&payload);
        return messageId;
    }
}
#endif

HW_API_FUNC HW_INT IOTA_CommandResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name,
    HW_CHAR *pcCommandResponse, void *context)
{
    if (requestId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_CommandResponse:the requestId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    int messageId = 0;
    char *payload = IOTA_CommandResponsePayload(result_code, response_name, pcCommandResponse);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportCommandReponse(requestId, payload, context, NULL);
        MemFree(&payload);
        return messageId;
    }
}
#if defined(MQTTV5)

HW_API_FUNC HW_INT IOTA_CommandResponseV5(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name,
    HW_CHAR *pcCommandResponse, void *context, MQTTV5_DATA *properties)
{
    if ((requestId == NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_CommandResponseV5:the requestId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    int messageId = 0;
    char *payload = IOTA_CommandResponsePayload(result_code, response_name, pcCommandResponse);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportCommandReponse(requestId, payload, context, (void *)properties);
        MemFree(&payload);
        return messageId;
    }
}
#endif

HW_API_FUNC HW_INT IOTA_PropertiesSetResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *result_desc,
    void *context)
{
    if (requestId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_PropertiesSetResponse:the requestId cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }
    int messageId = 0;
    char *payload = IOTA_PropertiesSetResponsePayload(result_code, result_desc);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportPropSetReponse(requestId, payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_PropertiesGetResponse(HW_CHAR *requestId, ST_IOTA_SERVICE_DATA_INFO serviceProp[],
    HW_INT serviceNum, void *context)
{
    if ((serviceNum <= 0) || (requestId == NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_PropertiesGetResponse:the requestId or serviceNum is wrong.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    char *payload = IOTA_PropertiesGetResponsePayload(serviceProp, serviceNum);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportPropGetReponse(requestId, payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_UpdateSubDeviceStatus(ST_IOTA_DEVICE_STATUSES *device_statuses, HW_INT deviceNum, void *context)
{
    char *payload = IOTA_UpdateSubDeviceStatusPayload(device_statuses, deviceNum);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_AddSubDevice(ST_IOTA_SUB_DEVICE_INFO *subDevicesInfo, HW_INT deviceNum, void *context)
{
    char *payload = IOTA_AddSubDevicePayload(subDevicesInfo, deviceNum);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_DelSubDevice(ST_IOTA_DEL_SUB_DEVICE *delSubDevices, HW_INT deviceNum, void *context)
{
    char *payload = IOTA_DelSubDevicePayload(delSubDevices, deviceNum);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_OTAVersionReport(ST_IOTA_OTA_VERSION_INFO otaVersionInfo, void *context)
{
    char *payload = IOTA_OTAVersionReportPayload(otaVersionInfo);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_OTAStatusReport(ST_IOTA_UPGRADE_STATUS_INFO otaStatusInfo, void *context)
{
    char *payload = IOTA_OTAStatusReportPayload(otaStatusInfo);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_SubscribeUserTopic(HW_CHAR *topicParas)
{
    return SubscribeUserTopic(topicParas);
}

HW_API_FUNC HW_INT IOTA_SubscribeTopic(HW_CHAR *topic, HW_INT qos)
{
    return SubscribeCustomTopic(topic, qos);
}

// for v2 api
static HW_INT IOTA_OTAv2GetFileNameFromUrl(const char *url, char *fileName, int fileNameBufferLength)
{
    // Split Url data
    const char *queryStringStart = strstr(url, HTTP_URL_QUERY_START_MARK);
    const char *start;
    for (start = queryStringStart; start != url; --start) {
        if (*start == SINGLE_SLANT_CHAR) {
            start++;
            if (strncpy_s(fileName, fileNameBufferLength, start, queryStringStart - start) != EOK) {
                return IOTA_FAILURE;
            } else {
                return IOTA_SUCCESS;
            }
        }
    }
    return IOTA_FAILURE;
}

static char *IOTA_GetUrlData(char *url, char *port, size_t portBufferLength, char *ip, size_t ipBufferLength)
{
    // Split Url data
    char *tmp = strstr(url, DOUBLE_OBLIQUE_LINE);
    if (tmp == NULL) {
        return NULL;
    }

    int len = strlen(tmp);
    char *tmpContainsColon = strstr(tmp, COLON);
    if (tmpContainsColon == NULL) {
        return NULL;
    }
    // the length of ipTmp is enough to copy
    int ret = strncpy_s(ip, ipBufferLength, tmp + strlen(DOUBLE_OBLIQUE_LINE),
        len - strlen(tmpContainsColon) - strlen(DOUBLE_OBLIQUE_LINE));
    if (ret != 0) {
        return NULL;
    }

    char *uri = strstr(tmpContainsColon, SINGLE_SLANT);
    if (strncpy_s(port, portBufferLength, tmpContainsColon + 1, uri - tmpContainsColon - 1) != EOK) {
        return NULL;
    }
    return uri;
}

static int IOTA_ConnectionDomainName(HW_CHAR *ip, HW_CHAR *port, HW_INT timeout)
{
    struct addrinfo hints = { 0 };
    struct addrinfo *resloved_addr = NULL;
    // fine-tune hints according to which socket you want to open
    hints.ai_family = AF_INET; // IP_v4;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_CANONNAME | AI_ALL | AI_ADDRCONFIG;
    if (getaddrinfo(ip, port, &hints, &resloved_addr) < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_ConnectionDomainName() get addr info error.\n");
        return IOTA_FAILURE;
    }
    int fdRet = -1;
    struct addrinfo *addr = NULL;
    for (addr = resloved_addr; addr != NULL; addr = addr->ai_next) {
        int fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fd < 0) {
            PrintfLog(EN_LOG_LEVEL_WARNING, "iota_datatrans: IOTA_ConnectionDomainName() creat socket error.\n");
            continue;
        }

        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        if ((setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval))) != 0) {
            PrintfLog(EN_LOG_LEVEL_WARNING, "iota_datatrans: IOTA_ConnectionDomainName() setsockopt error.\n");
            close(fd);
            continue;
        }

        uint16_t portInt = (uint16_t)strtoul(port, NULL, 10);
        if (portInt == 0) {
            PrintfLog(EN_LOG_LEVEL_WARNING, "iota_datatrans: IOTA_ConnectionDomainName() invalid port\n");
            close(fd);
            continue;
        }

        if (connect(fd, addr->ai_addr, addr->ai_addrlen) < 0) {
            PrintfLog(EN_LOG_LEVEL_WARNING,
                "iota_datatrans: IOTA_ConnectionDomainName() fail to connect server by tcp.\n");
            close(fd);
            continue;
        }
        fdRet = fd;
        break;
    }
    freeaddrinfo(resloved_addr);

    return fdRet;
}

static SSL *IOTA_OpensslConnect(int fd, SSL_CTX *context)
{
    SSL *ssl = SSL_new(context);
    if (ssl == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_OpensslConnect() new ssl failed.\n");
        return NULL;
    }

    if (SSL_set_fd(ssl, fd) <= 0) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_OpensslConnect() SSL_set_fd fail.\n");
        return NULL;
    }

    if (SSL_connect(ssl) == -1) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_OpensslConnect() ssl connect failed.\n");
        return NULL;
    }
    return ssl;
}

static HW_INT IOTA_GetHttpHeaderLength(const char *uri, const char *ip, const char *token)
{
    int retSum = 0;
    retSum += strlen(OTA_HTTP_GET);
    retSum += strlen(uri);
    retSum += strlen(OTA_HTTP_VERSION);
    retSum += strlen(OTA_HTTP_HOST);
    retSum += strlen(ip);
    retSum += strlen(OTA_LINEFEED);
    if (token != NULL) {
        retSum += strlen(OTA_CONTENT_TYPE);
        retSum += strlen(OTA_AUTH);
        retSum += strlen(token);
    }
    retSum += strlen(OTA_CRLF);
    return retSum;
}

static HW_INT IOTA_GetHttpHeader(char *header, size_t headerBufferLength, char *uri, char *ip, char *token)
{
    if (memset_s(header, headerBufferLength, 0, headerBufferLength) != EOK) {
        return IOTA_FAILURE;
    }

    if ((strcat_s(header, headerBufferLength, OTA_HTTP_GET) != EOK) || (strcat_s(header, headerBufferLength, uri) != EOK) ||
        (strcat_s(header, headerBufferLength, OTA_HTTP_VERSION) != EOK) ||
        (strcat_s(header, headerBufferLength, OTA_HTTP_HOST) != EOK) || (strcat_s(header, headerBufferLength, ip) != EOK) ||
        (strcat_s(header, headerBufferLength, OTA_LINEFEED) != EOK)) {
        return IOTA_FAILURE;
    };

    if (token != NULL) {
        if ((strcat_s(header, headerBufferLength, OTA_CONTENT_TYPE) != EOK) ||
            (strcat_s(header, headerBufferLength, OTA_AUTH) != EOK) ||
            (strcat_s(header, headerBufferLength, token) != EOK)) {
            return IOTA_FAILURE;
        }
    }
    if (strcat_s(header, headerBufferLength, OTA_CRLF) != EOK) {
        return IOTA_FAILURE;
    }
    return IOTA_SUCCESS;
}

static HW_INT IOTA_IotReadHeaderFlag(char *buf, unsigned long readLength, char *fileName, long *fileSize)
{
    // the length of rspStatusCode is enougt to copy
    char rspStatusCode[HTTP_STATUS_LENGTH + 1] = { "" };
    int result = 0;
    int ret = strncpy_s(rspStatusCode, sizeof(rspStatusCode),
        buf + strlen(OTA_HTTP_RESPONSE_VERSION), HTTP_STATUS_LENGTH);
    if (ret != 0) {
        return IOTA_FAILURE;
    }

    rspStatusCode[HTTP_STATUS_LENGTH] = '\0';
    if (strcmp(rspStatusCode, HTTP_OK)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() error: the statusCode is %s.\n",
            rspStatusCode);
        result = IOTA_FAILURE;
    }

    // get packageSize  from  Content-Length in the reponse Header
    int content_Length_index = GetSubStrIndex((const char *)buf, OTA_CONTENT_LENGTH);
    if (content_Length_index < IOTA_SUCCESS) {
        return IOTA_FAILURE;
    }
    long p = 0;
    unsigned long k;
    char pkgSize[PKG_LENGTH];
    for (k = content_Length_index + strlen(OTA_CONTENT_LENGTH); k < readLength - 1; k++) {
        if ((buf[k] == '\r') || (buf[k] == '\n') || (buf[k] == ';')) {
            break;
        } else {
            pkgSize[p++] = buf[k];
        }
    }
    pkgSize[p] = '\0';

    char *end;
    *fileSize = strtol(pkgSize, &end, 10);
    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() the fileSize is %d.\n", *fileSize);

    // get filename  from  Content-Disposition in the reponse Header, for v1 only
    int filename_index = GetSubStrIndex((const char *)buf, FILE_NAME_EQUAL);
    p = 0;
    if (filename_index >= IOTA_SUCCESS) {
        for (k = filename_index + strlen(FILE_NAME_EQUAL); k < readLength - 1; k++) {
            if (p >= PKGNAME_MAX) {
                result = IOTA_FAILURE;
                PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() the fileName is too long \n");
                break;
            }
            if ((buf[k] == '\r') || (buf[k] == '\n') || (buf[k] == ';')) {
                break;
            } else {
                fileName[p++] = buf[k];
            }
        }
        fileName[p] = '\0';
        PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() the fileName is %s \n", fileName);
    }
    return result;
}

static HW_INT IOTA_IotReadHeaderData(char *buf, long readLength, long *writeSize, FILE *fp)
{
    int ret = IOTA_SUCCESS;
    long writeLength = 0;
    long httpHeaderLength = GetSubStrIndex(buf, OTA_CRLF);
    if (httpHeaderLength < 0) {
        return IOTA_PARAMETER_EMPTY;
    }
    httpHeaderLength += (long)strlen(OTA_CRLF);

    long otaBodySize = readLength - httpHeaderLength;
    writeLength = (long)fwrite(buf + httpHeaderLength, sizeof(char), otaBodySize, fp);
    if (writeLength < otaBodySize) {
        ret = IOTA_FAILURE;
    }
    *writeSize = writeLength;
    return ret;
}

static HW_INT IOTA_IotRead(SSL *ssl, const char *otaFilePath, char *otaFileNameOut)
{
    long readLength = 0;
    long writeLength = 0;
    long writeSize = 0L;
    long fileSize = 0L;
    int ret = IOTA_SUCCESS;
    int result = 0;

    int headerFlag = 0; // to judge if read the response header
    FILE *fp = NULL;

    char buf[BUFSIZE];
    do {
        memset_s(buf, sizeof(buf), 0, sizeof(buf));
        readLength = SSL_read(ssl, buf, sizeof(buf) - 1);

        if (headerFlag == 0) {
            headerFlag = 1;
            result = IOTA_IotReadHeaderFlag(buf, readLength, otaFileNameOut, &fileSize);
            if ((strlen(otaFileNameOut) > PATH_MAX) || (result < 0)) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_IotReadHeaderFlag() return = %d.\n", result);
                break;
            }
            char canonicalFilePath[PATH_MAX] = { 0 };
            if (otaFilePath == NULL) {
                realpath(otaFileNameOut, canonicalFilePath);
            } else {
                char *fileNameWithPath = NULL;
                if (StrEndWith(otaFilePath, "/")) {
                    fileNameWithPath = CombineStrings(2, otaFilePath, otaFileNameOut);
                } else {
                    fileNameWithPath = CombineStrings(3, otaFilePath, "/", otaFileNameOut);
                }
                if (!fileNameWithPath) {
                    result = IOTA_FAILURE;
                    break;
                }
                realpath(fileNameWithPath, canonicalFilePath);
                MemFree((void **)&fileNameWithPath);
            }

            fp = fopen(canonicalFilePath, "wb");
            if (fp == NULL) {
                result = IOTA_FAILURE;
                PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_IotRead() fopen %s Failed\n", otaFileNameOut);
                break;
            }
            PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_IotRead() destination file path is %s\n",
                canonicalFilePath);

            ret = IOTA_IotReadHeaderData(buf, readLength, &writeSize, fp);
            if (ret == IOTA_FAILURE) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_IotRead() %s Write Failed!.\n", otaFileNameOut);
                break;
            }
            if (writeSize >= fileSize) {
                break;
            }
        } else {
            if (((writeSize + readLength) < fileSize) && (fp != NULL)) {
                writeLength = (long)fwrite((void *)buf, sizeof(char), readLength, fp);
                if (writeLength < readLength) {
                    PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_IotRead() %s Write Failed.\n", otaFileNameOut);
                    ret = IOTA_FAILURE;
                    break;
                }
                writeSize += writeLength;
            } else if (((writeSize + readLength) >= fileSize) && (writeSize < fileSize) && (fp != NULL)) {
                writeLength = (int)fwrite((void *)buf, sizeof(char), fileSize - writeSize, fp);
                if (writeLength < fileSize - writeSize) {
                    PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_IotRead() %s Write Failed!.\n", otaFileNameOut);
                    ret = IOTA_FAILURE;
                    break;
                }
                writeSize += writeLength;
                break;
            } else {
                break;
            }
        }
        buf[readLength] = '\0';
    } while (readLength > 0);

    (void)fflush(fp);
    if (fp != NULL) {
        fclose(fp);
    }
    return result;
}

HW_API_FUNC HW_INT IOTA_OTAVerifySign(HW_CHAR *sign, const HW_CHAR *otaFilePath, HW_CHAR *otaFilename)
{
    FILE* file;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char canonicalFilePath[PATH_MAX] = { 0 };
    SHA256_CTX sha256;
    unsigned char data[1024];
    unsigned int bytes;

    if (sign == NULL || strlen(sign) != SHA256_DIGEST_LENGTH * 2) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_OTAVerifySign() sign is NULL or strlen(sign) !=  64\n");
        return 0;
    }

    if (otaFilePath == NULL) {
        realpath(otaFilename, canonicalFilePath);
    } else {
        char *fileNameWithPath = NULL;
        if (StrEndWith(otaFilePath, "/")) {
            fileNameWithPath = CombineStrings(2, otaFilePath, otaFilename);
        } else {
            fileNameWithPath = CombineStrings(3, otaFilePath, "/", otaFilename);
        }
        if (!fileNameWithPath) {
            return IOTA_FAILURE;
        }
        realpath(fileNameWithPath, canonicalFilePath);
        MemFree((void **)&fileNameWithPath);
    }

    file = fopen(canonicalFilePath, "rb");
    if (file == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_OTAVerifySign() fopen %s Failed\n", otaFilename);
        return IOTA_RESOURCE_NOT_AVAILABLE;
    }
    SHA256_Init(&sha256);
    while ((bytes = fread(data, 1, 1024, file)) != 0) {
        SHA256_Update(&sha256, data, bytes);
    }
    SHA256_Final(hash, &sha256);
    fclose(file);

    int i, j;
    char mac32[2] = {0};
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf_s(mac32, 4, 4, "%02x", hash[i]);
        for (j = 0; j < 2; j++) {
            if (sign[i*2 + j] != mac32[j]) {
                return IOTA_FAILURE;
            }
        }
    }
    return IOTA_SUCCESS;
}

HW_API_FUNC HW_INT IOTA_GetOTAPackages(HW_CHAR *url, HW_CHAR *token, HW_INT timeout)
{
    char filename[PKGNAME_MAX + 1];
    return IOTA_GetOTAPackages_Ext(url, token, timeout, NULL, filename);
}

HW_API_FUNC HW_INT IOTA_GetLatestSoftBusInfo(HW_CHAR *busId, HW_CHAR *eventId, void *context)
{
    char *payload = IOTA_GetLatestSoftBusInfoPayload(busId, eventId);
    int messageId = 0;
    if (payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: get the newest soft bus info failed, the payload is null\n");
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        if (messageId != 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: get the newest soft bus info failed, the result is %d\n",
                messageId);
        }
        free(payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_GetOTAPackages_Ext(HW_CHAR *url, HW_CHAR *token, HW_INT timeout, const HW_CHAR *otaFilePath,
    HW_CHAR *otaFilenameOut)
{
    if ((url == NULL) || (otaFilenameOut == NULL) ||
        (timeout <= OTA_TIMEOUT_MIN_LENGTH)) { // the timeout value must be greater than 300s
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_GetOTAPackages() the input is invalid.\n");
        return IOTA_PARAMETER_ERROR;
    }

    int result = 0;
    char ipBuffer[IP_LENGTH] = { "" };
    char portBuffer[PORT_LENGTH] = { "" };

    char *ip = ipBuffer;
    char *port = portBuffer;
    char *uri = IOTA_GetUrlData(url, port, PORT_LENGTH, ip, IP_LENGTH);
    if (uri == NULL) {
        return IOTA_FAILURE;
    }

    int fd = IOTA_ConnectionDomainName(ip, port, timeout);
    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() connect server success by tcp.\n");

    SSL_CTX *context = IOTA_ssl_init();
    SSL *ssl = IOTA_OpensslConnect(fd, context);
    if (ssl == NULL) {
        SSL_CTX_free(context);
        close(fd);
        return IOTA_FAILURE;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() connect to server.\n");
    if (token == NULL) {
        if (IOTA_OTAv2GetFileNameFromUrl(url, otaFilenameOut, PKGNAME_MAX + 1) == IOTA_FAILURE) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() get v2 filename failed \n");
            SSL_CTX_free(context);
            close(fd);
            return IOTA_FAILURE;
        }
    }
    int httpHeaderLength = IOTA_GetHttpHeaderLength(uri, ip, token);
    int httpHeaderBufferLength = httpHeaderLength + 1;
    char *httpHeaderStr = (char *)malloc(sizeof(char) * httpHeaderBufferLength);
    if (httpHeaderStr == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() can't allocate memory for http header \n");
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(context);
        close(fd);
        return IOTA_FAILURE;
    }

    result = IOTA_GetHttpHeader(httpHeaderStr, httpHeaderBufferLength, uri, ip, token);
    if (result < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "iota_datatrans: IOTA_GetOTAPackages() HTTP_HEADER_LENGTH Insufficient length \n");
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(context);
        close(fd);
        MemFree(&httpHeaderStr);
        return IOTA_FAILURE;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() the request header is \n%s.\n", httpHeaderStr);

    int res = SSL_write(ssl, httpHeaderStr, httpHeaderLength);
    if (res < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR,
            "iota_datatrans: IOTA_GetOTAPackages() send https request failed, response is %d.\n", res);
        SSL_shutdown(ssl);
        close(fd);
        SSL_free(ssl);
        SSL_CTX_free(context);
        MemFree(&httpHeaderStr);
        return IOTA_FAILURE;
    }

    result = IOTA_IotRead(ssl, otaFilePath, otaFilenameOut);
    SSL_shutdown(ssl);
    close(fd);
    SSL_free(ssl);
    SSL_CTX_free(context);
    MemFree(&httpHeaderStr);
    if (result == 0) {
        PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() success.\n");
    }
    usleep(1000 * 1000); // wait connection released
    return result;
}

HW_API_FUNC SSL_CTX *IOTA_ssl_init()
{
    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_ssl_init() start to init ssl.\n");
    static SSL_CTX *server_ctx;

    // init openssl
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    if (!RAND_poll()) {
        return NULL;
    }

    server_ctx = SSL_CTX_new(SSLv23_client_method());
    if (server_ctx == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_ssl_init() New SSL_CTX failed.\n");
        return NULL;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_ssl_init() end to init ssl.\n");
    return server_ctx;
}

HW_API_FUNC HW_INT IOTA_GetDeviceShadow(HW_CHAR *requestId, HW_CHAR *object_device_id, HW_CHAR *service_id,
    void *context)
{
    if (requestId == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the requestId cannot be null\n");
        return IOTA_PARAMETER_EMPTY;
    }

    int messageId = 0;
    char *payload = IOTA_GetDeviceShadowPayload(object_device_id, service_id);
    if (payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetDeviceShadow() error,the payload is null.\n");
        return IOTA_FAILURE;
    } else {
        messageId = GetPropertiesRequest(requestId, payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_Bootstrap(char *baseStrategyKeyword)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_Bootstrap()==>\n");
    if (baseStrategyKeyword == NULL || strlen(baseStrategyKeyword) == 0) {
       return Bootstrap("");
    } 
    int messageId = 0;        
    cJSON * root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, BASE_STRATEGY_KEYWORD, baseStrategyKeyword);
    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    if (payload != NULL) {
        messageId = Bootstrap(payload);
        MemFree(&payload);
    } 
    return messageId;
}

HW_API_FUNC HW_INT IOTA_PropertiesReportV3(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum, void *context)
{
    if ((serviceNum == 0) || (pServiceData == NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();
    int i;
    for (i = 0; i < serviceNum; i++) {
        cJSON *properties = cJSON_Parse(pServiceData[i].properties);
        if (properties == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
            cJSON_Delete(data);
            cJSON_Delete(root);
            return IOTA_PARSE_JSON_FAILED;
        }

        cJSON *serviceData = cJSON_CreateObject();
        cJSON_AddStringToObject(serviceData, SERVICE_ID_V3, pServiceData[i].service_id);
        cJSON_AddStringToObject(serviceData, EVENT_TIME_V3, pServiceData[i].event_time);
        cJSON_AddItemToObject(serviceData, SERVICE_DATA_V3, properties);
        cJSON_AddItemToArray(data, serviceData);
    }

    cJSON_AddItemToObject(root, DATA_V3, data);
    cJSON_AddStringToObject(root, MSGTYPE, DEVIVE_REQ);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDevicePropertiesV3(payload, 0, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_BinaryReportV3(HW_CHAR *payload, void *context)
{
    if (payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    int messageId = ReportDevicePropertiesV3(payload, 1, context);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_CmdRspV3(ST_IOTA_COMMAND_RSP_V3 *cmdRspV3, void *context)
{
    if (cmdRspV3 == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
        return IOTA_PARAMETER_EMPTY;
    }

    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, MID, cmdRspV3->mid);
    cJSON_AddNumberToObject(root, ERR_CODE, cmdRspV3->errcode);
    cJSON_AddStringToObject(root, MSGTYPE, DEVIVE_RSP);

    cJSON *body = cJSON_Parse(cmdRspV3->body);
    cJSON_AddItemToObject(root, BODY, body);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportDevicePropertiesV3(payload, 0, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_SubscribeJsonCmdV3()
{
    return SubscribeJsonCmdV3();
}

HW_API_FUNC HW_INT IOTA_SubsrcibeBinaryCmdV3()
{
    return SubscribeBinaryCmdV3();
}

HW_API_FUNC HW_INT IOTA_SubscribeBoostrap()
{
    return SubscribeBootstrap();
}

HW_API_FUNC HW_INT IOTA_GetNTPTime(void *context)
{
    char *payload = IOTA_GetNTPTimePayload();
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_ReportDeviceLog(HW_CHAR *type, HW_CHAR *content, HW_CHAR *timestamp, void *context)
{
    char *payload = IOTA_ReportDeviceLogPayload(type, content, timestamp);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

/**
 * @Description: report device info to the iot platform
 * @param timestamp: ST_IOTA_DEVICE_INFO_REPORT structure
 * @param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passe
 * to success or failure callback functions to provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_ReportDeviceInfo(ST_IOTA_DEVICE_INFO_REPORT *device_info_report, void *context)
{
    char *payload = IOTA_ReportDeviceInfoPayload(device_info_report);
    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_RptDeviceConfigRst(const ST_IOTA_DEVICE_CONFIG_RESULT *device_config_report, void *context)
{
    cJSON *root = NULL;
    cJSON *services = NULL;
    cJSON *serviceEvent = NULL;
    cJSON *paras = NULL;
    char *payload = NULL;
    char *event_time = NULL;
    int messageId = 0;

    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    serviceEvent = cJSON_CreateObject();
    paras = cJSON_CreateObject();
    event_time = GetEventTimesStamp();
    if ((!root) || (!services) || (!serviceEvent) || (!paras) || (!event_time)) {
        cJSON_Delete(root);
        cJSON_Delete(services);
        cJSON_Delete(serviceEvent);
        cJSON_Delete(paras);
        MemFree(&event_time);
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_RptDeviceConfigRst: memory alloc failed!");
        return IOTA_FAILURE;
    }
    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, device_config_report->object_device_id);

    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, DEVICE_CONFIG);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, DEVICE_CONFIG_UPDATE_RESPONSE);
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, event_time);

    cJSON_AddNumberToObject(paras, RESULT_CODE, device_config_report->result_code);
    cJSON_AddStringToObject(paras, DESCRIPTION, device_config_report->description);

    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    payload = cJSON_Print(root);
    cJSON_Delete(root);
    MemFree(&event_time);

    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        MemFree(&payload);
        return messageId;
    }
}


static HW_INT IOTA_GetFileUrl(const ST_IOTA_DOWNLOAD_FILE *file, void *context, int isUpload)
{
    char *payload = IOTA_GetFileUrlPayload(file, isUpload);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        int messageId = EventUp(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_GetUploadFileUrl(const ST_IOTA_UPLOAD_FILE *upload, void *context)
{
    IOTA_GetFileUrl(upload, context, 1);
}

HW_API_FUNC HW_INT IOTA_GetDownloadFileUrl(const ST_IOTA_DOWNLOAD_FILE *download, void *context) 
{
    IOTA_GetFileUrl(download, context, 0);
}

static size_t CurlIgnoreOutput(void *ptr, size_t size, size_t nmemb, void *stream)
{
    (void) ptr;
    (void) stream;
    return size * nmemb;
}

static size_t CurlReadOutput(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

static HW_INT IOTA_ResultReport(char *object_device_id, char *object_name, int result_code, int status_code, char *status_description, int isUpload) 
{
    if ((object_name == NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the object_name or result_code param cannot be null\n");
        return IOTA_FAILURE;
    }

    cJSON *root = NULL;
    cJSON *services = NULL;
    cJSON *serviceEvent = NULL;
    cJSON * paras = NULL;
    char *event_time = NULL;

    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    serviceEvent = cJSON_CreateObject();
    paras = cJSON_CreateObject();
    event_time = GetEventTimesStamp();
    if ((!root) || (!services) || (!serviceEvent) || (!paras) || (!event_time)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_RptDeviceConfigRst: memory alloc failed!");
        cJSON_Delete(root);
        cJSON_Delete(services);
        cJSON_Delete(serviceEvent);
        cJSON_Delete(paras);
        MemFree(&event_time);
        return IOTA_FAILURE;
    }
    
    cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, object_device_id);
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, event_time);    
    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, FILE_MANAGER);
    if (isUpload) {
        cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, UPLOAD_RESULT_REPORT);
    } else {
        cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, DOWNLOAD_RESULT_REPORT);
    }

    cJSON_AddNumberToObject(paras, RESULT_CODE, result_code);
    cJSON_AddStringToObject(paras, "object_name", object_name);
    cJSON_AddNumberToObject(paras, "status_code", status_code);
    cJSON_AddStringToObject(paras, "status_description", status_description);

    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    MemFree(&event_time);
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        int messageId = EventUp(payload, NULL);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_UploadFileResultReport(char *object_device_id, char *object_name, int result_code, int status_code, char *status_description)  
{
    return IOTA_ResultReport(object_device_id, object_name, result_code, status_code, status_description, 1);
}

HW_API_FUNC HW_INT IOTA_DownloadFileResultReport(char *object_device_id, char *object_name, int result_code, int status_code, char *status_description)  
{
    return IOTA_ResultReport(object_device_id, object_name, result_code, status_code, status_description, 0);
}

static int getFileNameToUrl(char *url, char *fileName, int fileNameLen) {
    char *begin = NULL;
    char data[1024] = {0};
    begin = strchr(url, '?'); // 获取首次出现的地址
    if (!begin) {
        return IOTA_FAILURE;
    }
    int ret = memcpy_s(data, sizeof(data), url, begin - url);
    if (ret != 0) {
        return IOTA_FAILURE;
    }

    begin = strrchr(data, '/'); // 获取最后一次出现的地址
    ret = memcpy_s(fileName, fileNameLen, begin + 1, strlen(begin) - 1);
    if (ret != 0) {
        return IOTA_FAILURE;
    }
    
    return IOTA_SUCCESS;
}

HW_API_FUNC HW_INT IOTA_UploadFile(const char *filePath, const char *url, void *context)
{
    // Set the target URL
    char fileName[1024] = {0}; 
    int urlFileNmae = getFileNameToUrl(url, fileName, sizeof(fileName));
    if (filePath == NULL && urlFileNmae == IOTA_SUCCESS) {
        filePath = fileName;
    }
    
    char realFilePath[PATH_MAX] = {0};
    if (realpath(filePath, realFilePath) == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "realpath can't reslove path\n");
        return IOTA_RESOURCE_NOT_AVAILABLE;
    }
    FILE *fd = fopen(realFilePath, "rb");
    const char *file_name = basename(filePath);
    if (!fd) {
        fprintf(stderr, "Could not open file.\n");
        return IOTA_RESOURCE_NOT_AVAILABLE;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Error initializing libcurl.\n");
        (void)fclose(fd);
        return IOTA_FAILURE;
    }

    struct curl_slist *header = NULL;
    header = curl_slist_append(header, "Content-Type:text/plain");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, fd);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlIgnoreOutput);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    // curl_easy_setopt(curl, CURLOPT_CAINFO,"./conf/obs.pem"); 
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); 
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    int ret = IOTA_FAILURE;
    if (res != CURLE_OK || (http_code != 200 && http_code != CURLE_ABORTED_BY_CALLBACK)) {
        PrintfLog(
            EN_LOG_LEVEL_ERROR, "curl_easy_perform() failed: %s, http code %ld\n", curl_easy_strerror(res), http_code);
        ret = IOTA_FAILURE;
    } else {
        PrintfLog(EN_LOG_LEVEL_INFO, "upload ok, file path = %s\n", realFilePath);
        ret = IOTA_SUCCESS;
    }
     // 返回结果 
    if (urlFileNmae == IOTA_SUCCESS) {
        int result_code = (ret == IOTA_SUCCESS) ? 0 : 1 ; 
        IOTA_ResultReport(NULL, fileName, result_code, http_code, curl_easy_strerror(res), 1);
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(header);
    (void)fclose(fd);
    return ret;
}

HW_API_FUNC HW_INT IOTA_DownloadFile(const char *filePath, const char *url, void *context)
{

    FILE *fd = fopen(filePath, "wb");
    if (!fd) {
        fprintf(stderr, "Could not open file.\n");
        return IOTA_RESOURCE_NOT_AVAILABLE;
    }
    
    const char *file_name = basename(filePath);
    char realFilePath[PATH_MAX] = {0};
    if (realpath(filePath, realFilePath) == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "realpath can't reslove path\n");
        (void)fclose(fd);
        return IOTA_RESOURCE_NOT_AVAILABLE;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    if (!curl) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Error initializing libcurl.\n");
        (void)fclose(fd);
        return IOTA_FAILURE;
    }

    struct curl_slist *header = NULL;
    header = curl_slist_append(header, "Content-Type:text/plain");
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1200); // 设置下载超时时间，单位s
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fd);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlReadOutput);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    int ret = IOTA_FAILURE;
    if (res != CURLE_OK || (http_code != 200 && http_code != CURLE_ABORTED_BY_CALLBACK)) {
        PrintfLog(
            EN_LOG_LEVEL_ERROR, "curl_easy_perform() failed: %s, http code %ld\n", curl_easy_strerror(res), http_code);
        ret = IOTA_FAILURE;
    } else {
        PrintfLog(EN_LOG_LEVEL_INFO, "download ok, file path = %s\n", realFilePath);
        ret = IOTA_SUCCESS;
    }
    // 返回结果
    char fileName[1024] = {0};  
    if (getFileNameToUrl(url, fileName, sizeof(fileName)) == IOTA_SUCCESS) {
        int result_code = (ret == IOTA_SUCCESS) ? 0 : 1 ; 
        IOTA_ResultReport(NULL, fileName, result_code, http_code, curl_easy_strerror(res), 0);
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(header);
    (void)fclose(fd);
    return ret;
}

HW_API_FUNC HW_INT IOTA_M2MSendMsg(HW_CHAR *to, HW_CHAR *from, HW_CHAR *content, HW_CHAR *requestId, void *context)
{
    if ((to == NULL) || (from == NULL) || (content == NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the intput param cannot be null\n");
        return -1;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "cJSON_CreateObject failed\n");
    }

    cJSON_AddStringToObject(root, REQUEST_ID, requestId);
    cJSON_AddStringToObject(root, TO, to);
    cJSON_AddStringToObject(root, FROM, from);
    cJSON_AddStringToObject(root, CONTENT, content);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = OCM2MSendMsg(to, from, payload, requestId, context);
        MemFree(&payload);
        return messageId;
    }
}

/**
 * ----------------------------deprecated below, do not use it-------------------------------------->
 */

// Reserved interface for transparent transmission
HW_API_FUNC HW_INT IOTA_ReportSubDeviceInfo(HW_CHAR *pcPayload, void *context)
{
    if (pcPayload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the pcPayload cannot be null\n");
        return -1;
    }

    HW_INT messageId = ReportSubDeviceInfo(pcPayload, context);
    return messageId;
}

HW_API_FUNC HW_INT IOTA_SubDeviceVersionReport(HW_CHAR *version, void *context)
{
    if (version == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the version cannot be null\n");
        return -1;
    }
    cJSON *root;
    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, MESSAGE_NAME, SUB_DEVICE_VERSION_REPORT);
    cJSON_AddStringToObject(root, VERSION, version);

    char *payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportSubDeviceInfo(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_SubDeviceProductGetReport(cJSON *product_id_list, void *context)
{
    if (product_id_list == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the product_id_list cannot be null\n");
        return -1;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, MESSAGE_NAME, GET_PRODUCTS);
    cJSON_AddItemToObject(root, PRODUCTID_LIST, product_id_list);

    char *payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportSubDeviceInfo(payload, context);
        MemFree(&payload);
        return messageId;
    }
}

HW_API_FUNC HW_INT IOTA_SubDeviceScanReport(cJSON *device_list, void *context)
{
    if (device_list == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the device_list cannot be null\n");
        return -1;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, MESSAGE_NAME, SCAN_SUB_DEVICE_RESULT);
    cJSON_AddItemToObject(root, DEVICE_LIST, device_list);

    char *payload = cJSON_Print(root);

    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return IOTA_FAILURE;
    } else {
        messageId = ReportSubDeviceInfo(payload, context);
        MemFree(&payload);
        return messageId;
    }
}
