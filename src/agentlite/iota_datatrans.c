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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "string.h"
#include "string_util.h"
#include "log_util.h"
#include "base.h"
#include "hw_type.h"
#include "data_trans.h"
#include "iota_datatrans.h"
#include "cJSON.h"
#include "iota_error_type.h"
#include "subscribe.h"

#if defined(WIN32) || defined(WIN64)
#include <winsock2.h>
#include <winsock.h>
#else
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

//message up
HW_API_FUNC HW_INT IOTA_MessageReport(HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content, HW_CHAR *topicParas) {
	if (content == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the content cannot be null.\n");
		return IOTA_PARAMETER_EMPTY;
	}

	cJSON *root;
	root = cJSON_CreateObject();

	cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, object_device_id);
	cJSON_AddStringToObject(root, NAME, name);
	cJSON_AddStringToObject(root, ID, id);
	cJSON_AddStringToObject(root, CONTENT, content);

	char *payload = cJSON_Print(root);
	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = ReportDeviceData(payload, topicParas);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_ReportData() with payload %s By topic %s==>\n", payload, topicParas);
		free(payload);
		return messageId;
	}
}

//device reports properties
HW_API_FUNC HW_INT IOTA_PropertiesReport(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum) {
	if (serviceNum == 0 || pServiceData == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
		return IOTA_PARAMETER_EMPTY;
	}
	cJSON *root, *serviceDatas;
	root = cJSON_CreateObject();
	serviceDatas = cJSON_CreateArray();
	int i;
	for (i = 0; i < serviceNum; i++) {
		cJSON *tmp;
		tmp = cJSON_CreateObject();

		cJSON *properties = cJSON_Parse(pServiceData[i].properties);
		if (properties == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
			return IOTA_PARSE_JSON_FAILED;
		}

		cJSON_AddStringToObject(tmp, SERVICE_ID, pServiceData[i].service_id);
		cJSON_AddStringToObject(tmp, EVENT_TIME, pServiceData[i].event_time);
		cJSON_AddItemToObject(tmp, PROPERTIES, properties);
		cJSON_AddItemToArray(serviceDatas, tmp);
	}

	cJSON_AddItemToObject(root, SERVICES, serviceDatas);

	char *payload;
	payload = cJSON_Print(root);
	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = ReportDeviceProperties(payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_PropertiesReport() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}
}

//devices report properties
HW_API_FUNC HW_INT IOTA_BatchPropertiesReport(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum, HW_INT serviceLenList[]) {
	if (deviceNum == 0 || serviceLenList == NULL || pDeviceData == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
		return IOTA_PARAMETER_EMPTY;
	}

	if (deviceNum < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the deviceNum cannot be minus.\n");
		return IOTA_PARAMETER_ERROR;
	}

	if (deviceNum > MaxServiceReportNum) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the deviceNum exceeds maximum.\n");
		return IOTA_NUMBER_EXCEEDS;
	}

	cJSON *root, *deviceDatas;
	root = cJSON_CreateObject();
	deviceDatas = cJSON_CreateArray();
	int ii, jj;
	for (ii = 0; ii < deviceNum; ii++) {
		cJSON *deviceData;
		cJSON *services;
		deviceData = cJSON_CreateObject();
		services = cJSON_CreateArray();
		cJSON_AddStringToObject(deviceData, DEVICE_ID, pDeviceData[ii].device_id);

		for (jj = 0; jj < serviceLenList[ii]; jj++) {
			cJSON *tmp;
			tmp = cJSON_CreateObject();

			cJSON *properties = cJSON_Parse(pDeviceData[ii].services[jj].properties);
			if (properties == NULL) {
				PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
				return IOTA_PARSE_JSON_FAILED;
			}

			cJSON_AddStringToObject(tmp, SERVICE_ID, pDeviceData[ii].services[jj].service_id);
			cJSON_AddStringToObject(tmp, EVENT_TIME, pDeviceData[ii].services[jj].event_time);
			cJSON_AddItemToObject(tmp, PROPERTIES, properties);
			cJSON_AddItemToArray(services, tmp);

		}

		cJSON_AddItemToObject(deviceData, SERVICES, services);
		cJSON_AddItemToArray(deviceDatas, deviceData);
	}

	cJSON_AddItemToObject(root, DEVICES, deviceDatas);
	char *payload = cJSON_Print(root);

	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = ReportBatchDeviceProperties(payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_BatchPropertiesReport() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}
}

HW_API_FUNC HW_INT IOTA_CommandResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name, HW_CHAR *pcCommandResponse) {
	if (pcCommandResponse == NULL || requestId == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_CommandResponse:the requestId or commandResponse cannot be null.\n");
		return IOTA_PARAMETER_EMPTY;
	}

	cJSON *root;
	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, RESULT_CODE, result_code);
	cJSON_AddStringToObject(root, RESPONSE_NAME, response_name);
	cJSON *commandResponse = cJSON_Parse(pcCommandResponse);
	if (commandResponse == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the payload cannot be null.\n");
		return IOTA_PARSE_JSON_FAILED;
	}

	cJSON_AddItemToObject(root, PARAS, commandResponse);

	char *payload;
	payload = cJSON_Print(root);
	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = ReportCommandReponse(requestId, payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_CommandResponse() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}
}

HW_API_FUNC HW_INT IOTA_PropertiesSetResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *result_desc) {
	if (requestId == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_PropertiesSetResponse:the requestId cannot be null.\n");
		return IOTA_PARAMETER_EMPTY;
	}

	cJSON *root;
	root = cJSON_CreateObject();
	char *payload;
	cJSON_AddNumberToObject(root, RESULT_CODE, result_code);

	if (result_desc != 0) {
		cJSON_AddStringToObject(root, RESULT_DESC, result_desc);
	}

	payload = cJSON_Print(root);

	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = ReportPropSetReponse(requestId, payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_PropertiesSetResponse() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}

}

HW_API_FUNC HW_INT IOTA_PropertiesGetResponse(HW_CHAR *requestId, ST_IOTA_SERVICE_DATA_INFO serviceProp[], HW_INT serviceNum) {
	if (serviceNum <= 0 || requestId == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_PropertiesGetResponse:the requestId or serviceNum is wrong.\n");
		return IOTA_PARAMETER_EMPTY;
	}

	cJSON *root, *services;
	root = cJSON_CreateObject();
	char *payload;
	services = cJSON_CreateArray();

	int i;

	if (serviceNum != 0) {
		if (!(serviceProp[0].service_id == 0 || serviceProp[0].properties == 0)) {
			for (i = 0; i < serviceNum; i++) {
				if (serviceProp[i].service_id != 0 && serviceProp[i].properties != 0) {
					cJSON *properties = cJSON_Parse(serviceProp[i].properties);
					if (properties == NULL) {
						PrintfLog(EN_LOG_LEVEL_ERROR, "parse JSON failed.\n");
						return IOTA_PARSE_JSON_FAILED;
					}

					cJSON *tmp;
					tmp = cJSON_CreateObject();

					cJSON_AddStringToObject(tmp, SERVICE_ID, serviceProp[i].service_id);
					cJSON_AddStringToObject(tmp, EVENT_TIME, serviceProp[i].event_time);
					cJSON_AddItemToObject(tmp, PROPERTIES, properties);

					cJSON_AddItemToArray(services, tmp);
				} else {
					PrintfLog(EN_LOG_LEVEL_ERROR, "the payload is wrong.\n");
					return IOTA_PARAMETER_ERROR;
				}
			}

		}
	}

	cJSON_AddItemToObject(root, SERVICES, services);
	payload = cJSON_Print(root);

	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = ReportPropGetReponse(requestId, payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_PropertiesGetResponse() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}
}

HW_API_FUNC HW_INT IOTA_UpdateSubDeviceStatus(ST_IOTA_DEVICE_STATUSES *device_statuses, HW_INT deviceNum) {
	if((device_statuses == NULL) || (deviceNum < 0) || (deviceNum > MaxSubDeviceCount)) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_UpdateSubDeviceStatus() error, the input is invalid.\n");
		return IOTA_PARAMETER_ERROR;
	}
	if(device_statuses->device_statuses == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_UpdateSubDeviceStatus() error, the input of device_statuses->device_statuses is invalid.\n");
		return IOTA_PARAMETER_ERROR;
	}

	if(device_statuses->device_statuses[0].device_id == NULL || device_statuses->device_statuses[0].status == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_UpdateSubDeviceStatus() error, the input of device_statuses->device_statuses[0] is invalid.\n");
		return IOTA_PARAMETER_ERROR;
	}

	cJSON *root, *services, *device_statuses_json;
	root = cJSON_CreateObject();
	services = cJSON_CreateArray();
	device_statuses_json = cJSON_CreateArray();
	cJSON *service = cJSON_CreateObject();
	cJSON_AddStringToObject(service, SERVICE_ID, SUB_DEVICE_MANAGER);
	cJSON_AddStringToObject(service, EVENT_TYPE, SUB_DEVICE_UPDATE_STATUS);
	cJSON_AddStringToObject(service, EVENT_TIME, device_statuses->event_time);

	int i;
	for (i = 0; i < deviceNum; i++) {
		if (device_statuses->device_statuses[i].device_id != NULL && device_statuses->device_statuses[i].status != NULL) {

			cJSON *tmp;
			tmp = cJSON_CreateObject();

			cJSON_AddStringToObject(tmp, DEVICE_ID, device_statuses->device_statuses[i].device_id);
			cJSON_AddStringToObject(tmp, STATUS, device_statuses->device_statuses[i].status);

			cJSON_AddItemToArray(device_statuses_json, tmp);
		} else {
			PrintfLog(EN_LOG_LEVEL_ERROR, "the payload is wrong.\n");
			cJSON_Delete(device_statuses_json);
			cJSON_Delete(service);
			cJSON_Delete(services);
			cJSON_Delete(root);
			return IOTA_PARAMETER_ERROR;
		}
	}

	cJSON *paras = cJSON_CreateObject();
	cJSON_AddItemToObject(paras, DEVICE_STATUS, device_statuses_json);

	cJSON_AddItemToObject(service, PARAS, paras);


	cJSON_AddItemToArray(services, service);
	cJSON_AddItemToObject(root, SERVICES, services);

	char *payload;
	payload = cJSON_Print(root);

	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = EventUp(payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_UpdateSubDeviceStatus() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}

}



HW_API_FUNC HW_INT IOTA_OTAVersionReport(ST_IOTA_OTA_VERSION_INFO otaVersionInfo) {
	if (!(otaVersionInfo.fw_version != NULL || otaVersionInfo.sw_version != NULL)) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_OTAVersionReport:the input is invalid.\n");
		return IOTA_PARAMETER_ERROR;
	}

	cJSON *root, *paras, *services, *tmp;
	root = cJSON_CreateObject();
	tmp = cJSON_CreateObject();
	paras = cJSON_CreateObject();
	services = cJSON_CreateArray();
	char *payload;

	cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, otaVersionInfo.object_device_id);
	cJSON_AddStringToObject(tmp, SERVICE_ID, OTA);
	cJSON_AddStringToObject(tmp, EVENT_TIME, otaVersionInfo.event_time);
	cJSON_AddStringToObject(tmp, EVENT_TYPE, VERSION_REPORT);
	cJSON_AddStringToObject(paras, SW_VERSION, otaVersionInfo.sw_version);
	cJSON_AddStringToObject(paras, FW_VERSION, otaVersionInfo.fw_version);
	cJSON_AddItemToObject(tmp, PARAS, paras);

	cJSON_AddItemToArray(services, tmp);

	cJSON_AddItemToObject(root, SERVICES, services);

	payload = cJSON_Print(root);
	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = EventUp(payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_OTAVersionReport() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}
}

HW_API_FUNC HW_INT IOTA_OTAStatusReport(ST_IOTA_UPGRADE_STATUS_INFO otaStatusInfo) {
	if (otaStatusInfo.progress > 100 || otaStatusInfo.progress < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_OTAVersionReport:the progress is invalid.\n");
		return IOTA_PARAMETER_ERROR;
	}

	if (!((otaStatusInfo.result_code >= 0 && otaStatusInfo.result_code <= 10) || (otaStatusInfo.result_code == 255))) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_OTAVersionReport:the result_code is invalid.\n");
		return IOTA_PARAMETER_ERROR;
	}

	cJSON *root, *paras, *services, *tmp;
	root = cJSON_CreateObject();
	tmp = cJSON_CreateObject();
	paras = cJSON_CreateObject();
	services = cJSON_CreateArray();
	char *payload;

	cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, otaStatusInfo.object_device_id);
	cJSON_AddStringToObject(tmp, SERVICE_ID, OTA);
	cJSON_AddStringToObject(tmp, EVENT_TIME, otaStatusInfo.event_time);
	cJSON_AddStringToObject(tmp, EVENT_TYPE, UPGRADE_PROGRESS_REPORT);
	cJSON_AddNumberToObject(paras, RESULT_CODE, otaStatusInfo.result_code);
	cJSON_AddNumberToObject(paras, PROGRESS, otaStatusInfo.progress);
	cJSON_AddStringToObject(paras, VERSION, otaStatusInfo.version);
	cJSON_AddStringToObject(paras, DESCRIPTION, otaStatusInfo.description);

	cJSON_AddItemToObject(tmp, PARAS, paras);

	cJSON_AddItemToArray(services, tmp);

	cJSON_AddItemToObject(root, SERVICES, services);

	payload = cJSON_Print(root);
	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = EventUp(payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_OTAStatusReport() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}
}

HW_API_FUNC HW_INT IOTA_DeviceStatusUpdate(int num, ST_IOTA_ONLINE_STATUS_INFO onlineStatus[]) {
    if(num < 0 || num > 100){
        PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_DeviceStatusUpdate() with onlineStatus num %d ==>\n", num);
        return IOTA_PARAMETER_ERROR;
    }
	cJSON *root, *paras, *services, *tmp, *device_statuses,*online;
    root = cJSON_CreateObject();
    tmp = cJSON_CreateObject();
    paras = cJSON_CreateObject();
    device_statuses = cJSON_CreateArray();
    services = cJSON_CreateArray();
    char *payload;

    char* event_time = GetEventTimesStamp();
    cJSON_AddStringToObject(tmp, SERVICE_ID, "$sub_device_manager");
    cJSON_AddStringToObject(tmp, EVENT_TIME, event_time);
    cJSON_AddStringToObject(tmp, EVENT_TYPE, "sub_device_update_status");

    for(int i=0; i<num; i++){
        online = cJSON_CreateObject();
        cJSON_AddStringToObject(online, DEVICE_ID, onlineStatus[i].device_id);
        cJSON_AddStringToObject(online, "status", onlineStatus[i].status);
        cJSON_AddItemToArray(device_statuses, online);
    }
    cJSON_AddItemToObject(paras, "device_statuses", device_statuses);
    cJSON_AddItemToObject(tmp, PARAS, paras);
    cJSON_AddItemToArray(services, tmp);
    cJSON_AddItemToObject(root, SERVICES, services);
    payload = cJSON_Print(root);

    int messageId = 0;
   	if (payload == NULL) {
   		return IOTA_FAILURE;
   	} else {
   		messageId = EventUp(payload);
   		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_DeviceStatusUpdate() with payload %s ==>\n", payload);
   		free(payload);
   		return messageId;
   	}
    cJSON_Delete(root);
    if(event_time != NULL){
        MemFree(&event_time);
    }
}

HW_API_FUNC HW_INT IOTA_SubscribeUserTopic(HW_CHAR *topicParas) {
	return SubscribeUserTopic(topicParas);
}

HW_API_FUNC HW_INT IOTA_GetOTAPackages(HW_CHAR *url, HW_CHAR *token, HW_INT timeout) {
	if (url == NULL || token == NULL || timeout <= OTA_TIMEOUT_MIN_LENGTH) //the timeout value must be greater than 300s
	{
		PrintfLog(EN_LOG_LEVEL_ERROR, "the input is invalid.\n");
		return IOTA_PARAMETER_ERROR;
	}

	int result = 0;
	char *tmp = strstr(url, DOUBLE_OBLIQUE_LINE);
	if (tmp == NULL) {
		return IOTA_FAILURE;
	}
	char ipTmp[IP_LENGTH] = { "" };
	int len = strlen(tmp);
	char *tmpContainsColon = strstr(tmp, COLON);
	if (tmpContainsColon == NULL) {
		return IOTA_FAILURE;
	}
	//the length of ipTmp is enougt to copy
	strncpy(ipTmp, tmp + strlen(DOUBLE_OBLIQUE_LINE), len - strlen(tmpContainsColon) - strlen(DOUBLE_OBLIQUE_LINE));
	char *uri = strstr(tmpContainsColon, SINGLE_SLANT);
	char *ip = ipTmp;

	if (uri == NULL || ip == NULL) {
		return IOTA_FAILURE;
	}

	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	char buf[BUFSIZE];
	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(OTA_PORT);
	addr.sin_addr.s_addr = inet_addr(ip);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() creat socket error.\n");
		return IOTA_FAILURE;
	}

	if ((setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(struct timeval))) != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() setsockopt error.\n");
		close(fd);
		return IOTA_FAILURE;
	}

	SSL_CTX *context = NULL;
	context = IOTA_ssl_init();

	if (connect(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		close(fd);
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() fail to connect server by tcp.\n");
		return IOTA_FAILURE;
	}
	PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() connect server success by tcp.\n");
	SSL *ssl = SSL_new(context);
	if (ssl == NULL) {
		close(fd);
		SSL_CTX_free(context);
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() new ssl failed.\n");
		return IOTA_FAILURE;
	}

	if (SSL_set_fd(ssl, fd) <= 0) {
		SSL_shutdown(ssl);
		close(fd);
		SSL_free(ssl);
		SSL_CTX_free(context);
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() SSL_set_fd fail.\n");
		return IOTA_FAILURE;
	}

	if (SSL_connect(ssl) == -1) {
		SSL_shutdown(ssl);
		close(fd);
		SSL_free(ssl);
		SSL_CTX_free(context);
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() ssl connect failed.\n");
		return IOTA_FAILURE;
	}

	PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() connect to server.\n");

	//send http header
	char str1[HTTP_HEADER_LENGTH];
	memset(str1, 0, HTTP_HEADER_LENGTH);

	strcat(str1, OTA_HTTP_GET);
	strcat(str1, uri);
	strcat(str1, OTA_HTTP_VERSION);

	strcat(str1, OTA_HTTP_HOST);
	strcat(str1, ip);
	strcat(str1, OTA_LINEFEED);

	strcat(str1, OTA_CONTENT_TYPE);
	strcat(str1, OTA_AUTH);
	strcat(str1, token);
	strcat(str1, OTA_CRLF);

	PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() the request header is \n%s.\n", str1);

	int res = SSL_write(ssl, str1, strlen(str1));

	if (res < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() send https request failed, response is %d.\n", res);
		SSL_shutdown(ssl);
		close(fd);
		SSL_free(ssl);
		SSL_CTX_free(context);
		return IOTA_FAILURE;
	}

	int read_length = 0;
	int write_length = 0;
	long write_size = 0L;
	long fileSize = 0L;

	char rspStatusCode[HTTP_STATUS_LENGTH + 1] = { "" };
	char fileName[PKGNAME_MAX];
	char pkgSize[PKG_LENGTH];
	int headerFlag = 0;    //to judge if read the response header
	FILE *fp = { 0 };
	int k;

	do {
		memset(buf, 0, sizeof(buf));
		k = 0;
		read_length = SSL_read(ssl, buf, sizeof(buf) - 1);

		if (headerFlag == 0) {
			headerFlag = 1;

			//the length of rspStatusCode is enougt to copy
			strncpy(rspStatusCode, buf + strlen(OTA_HTTP_RESPONSE_VERSION), HTTP_STATUS_LENGTH);
			rspStatusCode[HTTP_STATUS_LENGTH] = '\0';
			if (strcmp(rspStatusCode, HTTP_OK)) {
				PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() error，the statusCode is %s.\n", rspStatusCode);
				result = IOTA_FAILURE;
			}

			//get packageSize  from  Content-Length in the reponse Header
			int content_Length_index = GetSubStrIndex((const char*) buf, OTA_CONTENT_LENGTH);
			if (content_Length_index < IOTA_SUCCESS) {
				break;
			}
			int p = 0;
			for (k = content_Length_index + strlen(OTA_CONTENT_LENGTH); k < read_length - 1; k++) {
				if (buf[k] == '\r' || buf[k] == '\n' || buf[k] == ';') {
					break;
				} else {
					pkgSize[p++] = buf[k];
				}

			}
			pkgSize[p] = '\0';

			char *end;
			fileSize = strtol(pkgSize, &end, 10);
			PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() the fileSize is %d.\n", fileSize);

			//get filename  from  Content-Disposition in the reponse Header
			int filename_index = GetSubStrIndex((const char*) buf, FILE_NAME);
			p = 0;
			if (filename_index < IOTA_SUCCESS) {
				break;
			}
			for (k = filename_index + strlen(FILE_NAME); k < read_length - 1; k++) {
				if (buf[k] == '\r' || buf[k] == '\n' || buf[k] == ';') {
					break;
				} else {
					fileName[p++] = buf[k];
				}

			}

			fileName[p] = '\0';

			PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() the fileName is %s \n", fileName);

			if (strlen(fileName) > PATH_MAX || NULL == fileName) {
				PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() the fileName is invalid.\n");
				break;
			}

			fp = fopen(fileName, "wb+");
			int bodyStart = GetSubStrIndex(buf, OTA_CRLF);
			if (bodyStart < IOTA_SUCCESS) {
				break;
			}
			if ((write_size + read_length) < fileSize) {
				write_length = fwrite(buf + bodyStart + strlen(OTA_CRLF), sizeof(char), read_length - bodyStart - strlen(OTA_CRLF), fp);
				if (write_length < read_length - bodyStart - (int) strlen(OTA_CRLF)) {
					PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() %s Write Failed!.\n", fileName);
					break;
				}
				write_size += write_length;
			} else if (((write_size + read_length) >= fileSize) && (write_size < fileSize)) {
				write_length = fwrite(buf + bodyStart + strlen(OTA_CRLF), sizeof(char), fileSize - write_size - bodyStart - strlen(OTA_CRLF), fp);
				if (write_length < fileSize - write_size - bodyStart - (int) strlen(OTA_CRLF)) {
					PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() %s Write Failed!.\n", fileName);
					break;
				}
				write_size += write_length;
				break;
			} else {
				break;
			}

		} else {
			if ((write_size + read_length) < fileSize) {
				write_length = fwrite(buf, sizeof(char), read_length, fp);
				if (write_length < read_length) {
					PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() %s Write Failed.\n", fileName);
					break;
				}
				write_size += write_length;

			} else if (((write_size + read_length) >= fileSize) && (write_size < fileSize)) {
				write_length = fwrite(buf, sizeof(char), fileSize - write_size, fp);
				if (write_length < fileSize - write_size) {
					PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetOTAPackages() %s Write Failed!.\n", fileName);
					break;
				}
				write_size += write_length;
				break;
			} else {
				break;
			}

		}
		buf[read_length] = '\0';

	} while (read_length > 0);

	fflush(fp);
	if (fp != NULL) {
		fclose(fp);
	}

	SSL_shutdown(ssl);
	close(fd);
	SSL_free(ssl);
	SSL_CTX_free(context);
	PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: IOTA_GetOTAPackages() success.\n");

	usleep(1000 * 1000);  // wait connection released
	return result;
}

HW_API_FUNC SSL_CTX* IOTA_ssl_init()

{
	PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: evssl_init1() start to init ssl.\n");
	static SSL_CTX *server_ctx;

	//init openssl
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();

	if (!RAND_poll()) {
		return NULL;
	}

	server_ctx = SSL_CTX_new(SSLv23_client_method());

	if (server_ctx == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: evssl_init1() New SSL_CTX failed.\n");
		return NULL;
	}

	PrintfLog(EN_LOG_LEVEL_INFO, "iota_datatrans: evssl_init1() end to init ssl.\n");
	return server_ctx;
}

//get device shadow data
HW_API_FUNC HW_INT IOTA_GetDeviceShadow(HW_CHAR *requestId, HW_CHAR *object_device_id, HW_CHAR *service_id) {
	if (requestId == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the requestId cannot be null\n");
		return IOTA_PARAMETER_EMPTY;
	}

	cJSON *root;
	root = cJSON_CreateObject();
	char *payload;

	cJSON_AddStringToObject(root, OBJECT_DEVICE_ID, object_device_id);
	cJSON_AddStringToObject(root, SERVICE_ID, service_id);

	payload = cJSON_Print(root);

	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "iota_datatrans: IOTA_GetDeviceShadow() error,the payload is null.\n");
		return IOTA_FAILURE;
	} else {
		messageId = GetPropertiesRequest(requestId, payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_GetDeviceShadow() with requestId is %s, payload %s ==>\n", requestId, payload);
		free(payload);
		return messageId;
	}
}

/**
 * ----------------------------deprecated below，do not use it-------------------------------------->
 */

//Reserved interface for transparent transmission
HW_API_FUNC HW_INT IOTA_ReportSubDeviceInfo(HW_CHAR *pcPayload) {
	if (pcPayload == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the pcPayload cannot be null\n");
		return -1;
	}

	HW_INT messageId = ReportSubDeviceInfo(pcPayload);
	PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_ReportSubDeviceInfo() with payload %s ==>\n", pcPayload);
	return messageId;
}

HW_API_FUNC HW_INT IOTA_SubDeviceVersionReport(HW_CHAR *version) {
	if (version == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the version cannot be null\n");
		return -1;
	}
	cJSON *root;
	root = cJSON_CreateObject();
	char *payload;
	cJSON_AddStringToObject(root, MESSAGE_NAME, SUB_DEVICE_VERSION_REPORT);
	cJSON_AddStringToObject(root, VERSION, version);

	payload = cJSON_Print(root);

	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = ReportSubDeviceInfo(payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_SubDeviceVersionReport() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}
}

HW_API_FUNC HW_INT IOTA_SubDeviceProductGetReport(cJSON *product_id_list) {
	if (product_id_list == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the product_id_list cannot be null\n");
		return -1;
	}
	cJSON *root;
	root = cJSON_CreateObject();
	char *payload;
	cJSON_AddStringToObject(root, MESSAGE_NAME, GET_PRODUCTS);
	cJSON_AddItemToObject(root, PRODUCTID_LIST, product_id_list);

	payload = cJSON_Print(root);

	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = ReportSubDeviceInfo(payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_SubDeviceProductGetReport() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}
}

//to do: participation is struct  ST_IOTA_DEVICE_INFO under iota_device_info.h
HW_API_FUNC HW_INT IOTA_SubDeviceScanReport(cJSON *device_list) {
	if (device_list == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "the device_list cannot be null\n");
		return -1;
	}
	cJSON *root;
	root = cJSON_CreateObject();
	char *payload;
	cJSON_AddStringToObject(root, MESSAGE_NAME, SCAN_SUB_DEVICE_RESULT);
	cJSON_AddItemToObject(root, DEVICE_LIST, device_list);

	payload = cJSON_Print(root);

	cJSON_Delete(root);

	int messageId = 0;
	if (payload == NULL) {
		return IOTA_FAILURE;
	} else {
		messageId = ReportSubDeviceInfo(payload);
		PrintfLog(EN_LOG_LEVEL_DEBUG, "iota_datatrans: IOTA_SubDeviceScanReport() with payload %s ==>\n", payload);
		free(payload);
		return messageId;
	}
}
