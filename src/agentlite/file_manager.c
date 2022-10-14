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

#include <file_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "string.h"
#include "string_util.h"
#include "log_util.h"
#include "base.h"
#include "hw_type.h"
#include "data_trans.h"
#include "cJSON.h"
#include "subscribe.h"

#define FILE_SLEEP      1000//File transfer completion interval
/* * File operation part * */
static HW_INT File_OpenFileRead(FILE *fp, char *buf, size_t *len)
{
    if (fp == NULL) {
        return FILE_OPEN_ERR;
    }

    size_t sum = fread(buf, 1, FILE_HEADER_LENGTH, fp);
    *len = sum;
    *(buf + sum) = '\0';

    int i;

    if (sum != FILE_HEADER_LENGTH) {
        return FILE_END;
    }
    return FILE_SUCCESS;
}

static HW_INT FILE_OpenFileWrite(FILE *fp, char *buf, size_t len)
{
    if (fp == NULL) {
        return FILE_OPEN_ERR;
    }

    size_t sum = fwrite(buf, 1, len, fp);

    if (sum != len) {
        return FILE_OPEN_ERR;
    }
    if (len < FILE_R_MAX ) {
        return FILE_END;
    }
    return FILE_SUCCESS;
}

static HW_LLONG File_Size(char *filePath)
{
    FILE *fp = fopen(filePath, "rb");
    if (fp == NULL) {
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long long len = ftell(fp);
    fclose(fp);
    fp = NULL;

    return len;
}

/* * data processing part * */
static HW_INT File_UrlHandle(HW_CHAR *url, HW_CHAR *ip, HW_CHAR *uri)
{
    if (url == NULL) {
        return FILE_FAILURE;
    }
    char *tmp = strstr(url, DOUBLE_OBLIQUE_LINE);
    if (tmp == NULL) {
        return FILE_FAILURE;
    }
    int len = strlen(tmp);
    char *tmpContainsColon = strstr(tmp, COLON);
    if (tmpContainsColon == NULL) {
        return FILE_FAILURE;
    }
    // the length of ipTmp is enougt to copy
    if (ip != NULL) {
        strncpy(ip, tmp + strlen(DOUBLE_OBLIQUE_LINE), len - strlen(tmpContainsColon) - strlen(DOUBLE_OBLIQUE_LINE));
    }
    if (uri != NULL) {
        tmp = strstr(tmpContainsColon, SINGLE_SLANT);
        strncpy(uri, tmp, strlen(tmp));
    }
 
    return FILE_SUCCESS;
}

static HW_INT File_HttpHead(char *str, ST_FILE_URL_PAR url_par, ST_FILE_WR_PARAMERER wr)
{
    if (wr.readOrwrite == 1) {
        strncat(str, FILE_HTTP_GET, strlen(FILE_HTTP_GET));
    } else {
        strncat(str, FILE_HTTP_PUT, strlen(FILE_HTTP_PUT));
    }
    strncat(str, url_par.uri, strlen(url_par.uri));
    strncat(str, FILE_HTTP_VERSION, strlen(FILE_HTTP_VERSION));

    strncat(str, OTA_HTTP_HOST, strlen(OTA_HTTP_HOST));
    strncat(str, url_par.ip, strlen(url_par.ip));
    strncat(str, FILE_LINEFEED, strlen(FILE_LINEFEED));

    strncat(str, FILE_CONNECTION, strlen(FILE_CONNECTION));
    strncat(str, FILE_CONTENT_TYPE, strlen(FILE_CONTENT_TYPE));

    if (wr.readOrwrite == 0) {
        char sum[HTTP_CONTENT_LEN] = {0};
        snprintf(sum, HTTP_CONTENT_LEN, "%lld", wr.file_size);
        strncat(str, FILE_CONTENT_LENGTH, strlen(FILE_CONTENT_LENGTH));
        strncat(str, sum, strlen(sum));
        strncat(str, FILE_LINEFEED, strlen(FILE_LINEFEED));

    } else {
        //Breakpoint transmission
        char sum[HTTP_CONTENT_LEN] = {0};
        snprintf(sum, HTTP_CONTENT_LEN, "%ld", wr.flag_start);
        strncat(str, "RANGE: bytes=", strlen("RANGE: bytes="));
        strncat(str, sum);

        strncat(str, "-", strlen("-"));
        snprintf(sum, HTTP_CONTENT_LEN, "%ld", wr.flag_stop);
        strncat(str, sum, strlen(sum));
        strncat(str, FILE_LINEFEED, strlen(FILE_LINEFEED));
    }

    strncat(str, FILE_LINEFEED, strlen(FILE_LINEFEED));

    return FILE_SUCCESS;
}

static HW_INT FILE_HttpsWrite(FILE *fp, SSL *ssl, ST_FILE_URL_PAR *url_par, ST_FILE_WR_PARAMERER *wr)
{
    if (ssl == NULL || url_par->url == NULL || wr->filePath == NULL) {
        return FILE_FAILURE;
    }

    int x = 0;
    int firstEntry = 0;
    do {
        size_t dataLen = 0;
        char str[FILE_HEADER_LENGTH + 1];
        memset(str, 0, FILE_HEADER_LENGTH + 1);

        if (firstEntry == 0) {
            firstEntry = 1;    
            if (File_HttpHead(str, *url_par, *wr) != FILE_SUCCESS) {
                return FILE_FAILURE;
            }
            PrintfLog(EN_LOG_LEVEL_INFO, "FILE_datatrans: FILE_HttpsWrite() the request header is \n%s.\n", str);
        
        } else {
            x = File_OpenFileRead(fp, str, &dataLen);
            if (x == FILE_END) {
                strncat(str, FILE_CRLF, strlen(FILE_CRLF));
                PrintfLog(EN_LOG_LEVEL_INFO, "FILE_END\n");
            }
            if (x < 0) {
                PrintfLog(EN_LOG_LEVEL_ERROR, "FIEL_ERR\n");
                return FILE_FAILURE;
            }
        }

        int res = SSL_write(ssl, str, strlen(str));
        if (res < 0) {
            PrintfLog(EN_LOG_LEVEL_ERROR,
                "FILE_datatrans: FILE_HttpsWrite() send https request failed, response is %d.\n", res);
            return FILE_FAILURE;
        }
    } while (x != FILE_END && wr->readOrwrite == 0);
    return FILE_SUCCESS;
}

static HW_INT FILE_HttpsReadDataProcessing(char *buf, HW_LLONG *lenx)
{
    // the length of rspStatusCode is enougt to copy
    int result = -1;
    char rspStatusCode[HTTP_STATUS_LENGTH + 1] = { "" };
    char contentLen[HTTP_CONTENT_LEN + 1] = { "" };
    HW_LLONG len = 0;
    char *conLenStart = NULL;

    strncpy(rspStatusCode, buf + strlen(FILE_HTTP_RESPONSE_VERSION), HTTP_STATUS_LENGTH);
    rspStatusCode[HTTP_STATUS_LENGTH] = '\0';
    if (strcmp(rspStatusCode, HTTP_OK) || strcmp(rspStatusCode, HTTP_OK_206)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "FILE_datatrans: FILE_HttpsRead() error，the statusCode is %s.\n", rspStatusCode);
    }
    sscanf(rspStatusCode, "%d", &result);

    if (strstr(buf, FILE_CONTENT_RANGE) != NULL) {
         conLenStart = strstr(buf, FILE_CONTENT_RANGE) + strlen(FILE_CONTENT_RANGE);
         conLenStart = strstr(conLenStart, SINGLE_SLANT) + strlen(SINGLE_SLANT);

    } else {
        conLenStart = strstr(buf, FILE_CONTENT_LENGTH) + strlen(FILE_CONTENT_LENGTH);
    }
   
    char *conLenEnd = strstr(conLenStart, FILE_LINEFEED);
    int sum = conLenEnd - conLenStart;

    strncpy(contentLen, conLenStart, sum);
    *(contentLen + sum) = '\0';

    sscanf(contentLen, "%lld", &len);
    *lenx = len;
    return result;
}

static HW_INT FILE_HttpsRead(FILE *fp ,SSL *ssl, ST_FILE_WR_PARAMERER *wr)
{
    if (ssl == NULL) {
        return FILE_FAILURE;
    }

    int result = FILE_FAILURE;
    // Return value
    int read_length = 0;
    char buf[BUFSIZE];
    int headerFlag = 0;
    do {
        memset(buf, 0, sizeof(buf));
        read_length = SSL_read(ssl, buf, sizeof(buf) - 1);
        buf[read_length] = '\0';
         
        if (headerFlag == 0) {
            headerFlag = 1;
            if (read_length == 0) {
                return FILE_NO_RESPONSE;
            }
            result = FILE_HttpsReadDataProcessing(buf, &wr->file_size);
            PrintfLog(EN_LOG_LEVEL_INFO, "FILE_datatrans: FILE_HttpsRead() buf = %s.\n", buf);

        } else {
            if (wr->readOrwrite == 1) {
                if (FILE_OpenFileWrite(fp, buf, read_length) < 0) {
                    PrintfLog(EN_LOG_LEVEL_ERROR, "FILE_OpenFileWrite()  FILE_OPEN_ERR\n");
                    return FILE_OPEN_ERR;
                }
            }
        }
    } while (read_length > 0 && read_length != FILE_R_MAX);

    return result;
}

/* * Connecting part * */
static HW_INT FILE_Socket(HW_INT timeout)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "FILE_datatrans: FILE_Socket() creat socket error.\n");
        return FILE_FAILURE;
    }

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if ((setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval))) != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "FILE_datatrans: FILE_Socket() setsockopt error.\n");
        close(fd);
        return FILE_FAILURE;
    }

    return fd;
}

static HW_INT FILE_TcpConn(int fd, char *id, char *port)
{
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(id);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        PrintfLog(EN_LOG_LEVEL_ERROR, "FILE_datatrans: FILE_TcpConn() fail to connect server by tcp.\n");
        return FILE_FAILURE;
    }
    return FILE_SUCCESS;
}

HW_API_FUNC SSL_CTX *FILE_ssl_init()
{
    PrintfLog(EN_LOG_LEVEL_INFO, "FILE_datatrans: evssl_init1() start to init ssl.\n");
    static SSL_CTX *server_ctx;

    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    if (!RAND_poll()) {
        return NULL;
    }

    server_ctx = SSL_CTX_new(SSLv23_client_method());

    if (server_ctx == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "FILE_datatrans: evssl_init1() New SSL_CTX failed.\n");
        return NULL;
    }

    PrintfLog(EN_LOG_LEVEL_INFO, "FILE_datatrans: evssl_init1() end to init ssl.\n");
    return server_ctx;
}

static SSL *FILE_SslConnect(int fd)
{
    SSL_CTX *context = NULL;
    context = FILE_ssl_init();
    SSL *ssl = SSL_new(context);
    SSL_CTX_free(context);
    if (ssl == NULL) {
        close(fd);
        PrintfLog(EN_LOG_LEVEL_ERROR, "FILE_datatrans: FILE_SslConnect() new ssl failed.\n");
        return NULL;
    }

    if (SSL_set_fd(ssl, fd) <= 0) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        PrintfLog(EN_LOG_LEVEL_ERROR, "FILE_datatrans: FILE_SslConnect() SSL_set_fd fail.\n");
        return NULL;
    }

    if (SSL_connect(ssl) == -1) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        PrintfLog(EN_LOG_LEVEL_ERROR, "FILE_datatrans: FILE_SslConnect() ssl connect failed.\n");
        return NULL;
    }
    return ssl;
}

HW_API_FUNC HW_INT FILE_OBSTransmission(HW_CHAR *url, HW_CHAR *filePath, HW_INT timeout, HW_INT readOrwrite)
{
    if (url == NULL ||filePath == NULL ||
        timeout <= OTA_TIMEOUT_MIN_LENGTH) { // the timeout value must be greater than 300s
        PrintfLog(EN_LOG_LEVEL_ERROR, "the download is invalid.\n");
        return FILE_PARAMETER_ERROR;
    }

    HW_CHAR ip[FILE_IP_URI_LEN] = {0};
    HW_CHAR uri[FILE_IP_URI_LEN] = {0};

    ST_FILE_URL_PAR url_par = {url, ip, uri};
    ST_FILE_WR_PARAMERER wr = {0,  FILE_R_MAX - 1, 0, readOrwrite, filePath}; 

    char *ip_16 = NULL;
    int result = 0;

    // Domain name converted to IP address
    result = File_UrlHandle(url, url_par.ip, url_par.uri);
    if (result < 0) {
        return FILE_FAILURE;
    }
    struct hostent *name = gethostbyname(ip);
    if (name == NULL) {
         return FILE_FAILURE;
    }
    ip_16 = inet_ntoa(*(struct in_addr*)name->h_addr_list[0]);
   
    // set up soket
    int fd = FILE_Socket(timeout);
    if (fd < 0) {
        return FILE_HTTP_CONNECT_EXISTED;
    }
    // TCP connect
    result = FILE_TcpConn(fd, ip_16, FILE_PORT);
    if (result < 0) {
        return FILE_HTTP_DISCONNECT_FAILED;
       
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "FILE_datatrans: FILE_OBSTransmission() connect server success by tcp.\n");

    // SSL handshake
    SSL *ssl = FILE_SslConnect(fd);
    if (ssl == NULL) {
        return FILE_SSL_CONNECT_FAILED;
    }
    PrintfLog(EN_LOG_LEVEL_INFO, "FILE_datatrans: FILE_OBSTransmission() connect to server.\n");

    // Open File
    FILE *file = NULL;
    if (wr.readOrwrite == 1) {
        file = fopen(wr.filePath, "wb");
    } else {
        wr.file_size = File_Size(wr.filePath);
        file = fopen(wr.filePath, "rb");
    }
    if (file == NULL || wr.file_size < 0) {
        result = FILE_OPEN_ERR;
    }

    // Sending and receiving data  
    do{
         // HTTP Data transmission
        result = FILE_HttpsWrite(file, ssl, &url_par, &wr);
        if (result != FILE_SUCCESS) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(fd);
            fclose(file); 
            return FILE_WRITE_FAILED;
        }

        // receive data
        result = FILE_HttpsRead(file, ssl, &wr);
        if (result != HTTP_OK_206_INIT && result != HTTP_OK_INIT) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "FILE_datatrans: FILE_OBSTransmission() ERROR.\n");
            break;
        }
        wr.flag_start += FILE_R_MAX;
        wr.flag_stop +=  FILE_R_MAX;
    }while(wr.flag_stop - FILE_R_MAX < wr.file_size && wr.readOrwrite == 1);
    
    if (result == HTTP_OK_INIT || (wr.flag_start > wr.file_size  && wr.readOrwrite == 1)) {
        PrintfLog(EN_LOG_LEVEL_INFO, "FILE_datatrans: FILE_OBSTransmission() success.\n");
        result = HTTP_OK_INIT;
    }
        
    fclose(file);  
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(fd);
    usleep(FILE_SLEEP); // wait connection released
    return result;
}
/**
 * @Description: upload File
 * @param url: package download address
 * @param filePath: File path to upload
 * @param timeout: The timeout for requesting to download the package, which needs to be greater than 300 seconds. Less
 * than 24 hours is recommended
 * @return: FILE_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT FILE_Upload(HW_CHAR *url, HW_CHAR *filePath, HW_INT timeout)
{
    int result = FILE_OBSTransmission(url, filePath, timeout, 0);
    PrintfLog(EN_LOG_LEVEL_INFO, "FILE_datatrans: FILE_Upload() result = %d.\n", result);
    FILE_FileResponse(result, filePath, 0);

    return result;
}
/**
 * @Description: Download File
 * @param url: package download address
 * @param filePath: File path to upload
 * @param timeout: The timeout for requesting to download the package, which needs to be greater than 300 seconds. Less
 * than 24 hours is recommended
 * @return: FILE_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT FiLE_Download(HW_CHAR *url, HW_CHAR *filePath, HW_INT timeout)
{
    int result = FILE_OBSTransmission(url, filePath, timeout, 1);
    PrintfLog(EN_LOG_LEVEL_INFO, "FILE_datatrans: FiLE_Download() result = %d.\n", result);
    FILE_FileResponse(result, filePath, 1);

    return result;
}

/* * MQTT request* */
/**
 * @Description: upload File get URL obtain
 * @param device_info_report: Relevant data of documents
 * @param context: A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_UploadFileGetUrl(ST_FILE_MANA_INFO_REPORT *device_info_report, void *context)
{
    if (device_info_report == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the deviceInfo cannot be null\n");
        return -1;
    }
    cJSON *root, *services, *serviceDatas, *paras, *file;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    serviceDatas = cJSON_CreateObject();
    paras = cJSON_CreateObject();
    file = cJSON_CreateObject();

    if (device_info_report->object_device_id != NULL) {
        cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, device_info_report->object_device_id);
    }
    cJSON_AddStringToObject(serviceDatas, SERVICE_ID, FILE_SERVICE_ID);
    cJSON_AddStringToObject(serviceDatas, EVENT_TYPE, FILE_EVENT_UP);
    cJSON_AddStringToObject(serviceDatas, EVENT_TIME, GetEventTimesStamp());

    cJSON_AddStringToObject(paras, FILE_NAME_L, device_info_report->file_name);
    cJSON_AddStringToObject(file, HASH_CODE, device_info_report->file_hash_code);
    if (device_info_report->file_size != 0) {
        cJSON_AddNumberToObject(file, SIZE, device_info_report->file_size);
    }

    cJSON_AddItemToObject(paras, FILE_ATTRIBUTES, file);
    cJSON_AddItemToObject(serviceDatas, PARAS, paras);
    cJSON_AddItemToArray(services, serviceDatas);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload;
    payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return FILE_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "FILE_datatrans: FILE_ReportFile() with payload %s ==>\n", payload);
        free(payload);
        return messageId;
    }
}

/**
 * @Description: Download File get URL obtain
 * @param device_info_report: Relevant data of documents
 * @param context: A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_DownloadFileGetUrl(ST_FILE_MANA_INFO_REPORT *device_info_report, void *context)
{
    if (device_info_report == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the deviceInfo cannot be null\n");
        return -1;
    }
    cJSON *root, *services, *serviceDatas, *paras, *file;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    serviceDatas = cJSON_CreateObject();
    paras = cJSON_CreateObject();
    file = cJSON_CreateObject();

    if (device_info_report->object_device_id != NULL) {
        cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, device_info_report->object_device_id);
    }
    cJSON_AddStringToObject(serviceDatas, SERVICE_ID, FILE_SERVICE_ID);
    cJSON_AddStringToObject(serviceDatas, EVENT_TYPE, FILE_EVENT_DOWN);
    cJSON_AddStringToObject(serviceDatas, EVENT_TIME, GetEventTimesStamp());

    cJSON_AddStringToObject(paras, FILE_NAME_L, device_info_report->file_name);
    cJSON_AddStringToObject(file, HASH_CODE, device_info_report->file_hash_code);
    if (device_info_report->file_size != 0) {
        cJSON_AddNumberToObject(file, SIZE, device_info_report->file_size);
    }

    cJSON_AddItemToObject(paras, FILE_ATTRIBUTES, file);
    cJSON_AddItemToObject(serviceDatas, PARAS, paras);
    cJSON_AddItemToArray(services, serviceDatas);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload;
    payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return FILE_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "FILE_datatrans: FILE_ReportFile() with payload %s ==>\n", payload);
        free(payload);
        return messageId;
    }
}

static HW_INT FILE_ReportFileResponse(ST_FILE_MANA_RES_REPORT *result_code, void *context)
{
    if (result_code == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "the result_code cannot be null\n");
        return -1;
    }

    cJSON *root, *services, *serviceDatas, *paras, *file;
    root = cJSON_CreateObject();
    services = cJSON_CreateArray();
    serviceDatas = cJSON_CreateObject();
    paras = cJSON_CreateObject();
    if (result_code->object_device_id != NULL) {
        cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, result_code->object_device_id);
    }
    cJSON_AddStringToObject(serviceDatas, SERVICE_ID, FILE_SERVICE_ID);
    cJSON_AddStringToObject(serviceDatas, EVENT_TYPE, result_code->event_type);
    cJSON_AddStringToObject(serviceDatas, EVENT_TIME, GetEventTimesStamp());

    cJSON_AddStringToObject(paras, OBJECT_NAME, result_code->object_name);
    cJSON_AddNumberToObject(paras, RESULT_CODE, result_code->result_code);
    cJSON_AddNumberToObject(paras, STATUS_CODE, result_code->status_code);
    if (result_code->status_description != NULL) {
        cJSON_AddStringToObject(paras, STATUS_DESCRIPTION, result_code->status_description);
    }

    cJSON_AddItemToObject(serviceDatas, PARAS, paras);
    cJSON_AddItemToArray(services, serviceDatas);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload;
    payload = cJSON_Print(root);
    cJSON_Delete(root);

    int messageId = 0;
    if (payload == NULL) {
        return FILE_FAILURE;
    } else {
        messageId = EventUp(payload, context);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "FILE_datatrans: FILE_ReportFileResponse() with payload %s ==>\n", payload);
        free(payload);
        return messageId;
    }
}

static void FILE_FileResponse(int result, HW_CHAR *filePath, HW_INT readOrwrite)
{
    ST_FILE_MANA_RES_REPORT result_code;

    result_code.object_device_id = NULL;
    result_code.object_name = filePath;
    if (result == HTTP_OK_INIT) {
        result_code.result_code = 0;
    } else {
        result_code.result_code = 1;
    }
    result_code.status_code = result;
    result_code.status_description = NULL;

    if (readOrwrite == 0) {
        result_code.event_type = UPLOAD_RESULT_REPORT;
    } else {
        result_code.event_type = DOWN_RESULT_REPORT;
    }

    int messageId = FILE_ReportFileResponse(&result_code, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: FILE_ReportFileResponse() failed, messageId %d\n", messageId);
    }
}
