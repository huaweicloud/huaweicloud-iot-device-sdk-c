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

#ifndef _IOTA_DATATRANS_H_
#define _IOTA_DATATRANS_H_

#include <openssl/ossl_typ.h>
#include "cJSON.h"
#include "hw_type.h"

#define MaxServiceReportNum 10  //set the max count of reported services

typedef struct {
	HW_CHAR *service_id;
	HW_CHAR *event_time;
	HW_CHAR *properties;
} ST_IOTA_SERVICE_DATA_INFO;

typedef struct {
	HW_CHAR *device_id;
	ST_IOTA_SERVICE_DATA_INFO services[MaxServiceReportNum];
} ST_IOTA_DEVICE_DATA_INFO;

typedef struct {
	HW_CHAR *event_time;
	HW_CHAR *sw_version;
	HW_CHAR *fw_version;
	HW_CHAR *object_device_id;
} ST_IOTA_OTA_VERSION_INFO;

typedef struct {
	HW_CHAR *device_id;
	HW_CHAR *status;
} ST_IOTA_ONLINE_STATUS_INFO;

typedef struct {
	HW_CHAR *event_time;
	HW_INT result_code;
	HW_INT progress;
	HW_CHAR *description;
	HW_CHAR *object_device_id;
	HW_CHAR *version;
} ST_IOTA_UPGRADE_STATUS_INFO;

HW_API_FUNC HW_INT IOTA_ServiceReportData(HW_CHAR *pcDeviceId, HW_CHAR *pcServiceId, HW_CHAR *pcServiceProperties);
HW_API_FUNC HW_INT IOTA_ServiceCommandRespense(HW_UINT uiMid, HW_UINT uiResultCode, HW_CHAR *pcCommandRespense);
HW_API_FUNC HW_INT IOTA_MessageReport(HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content, HW_CHAR *topicParas);
HW_API_FUNC HW_INT IOTA_PropertiesReport(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum);
HW_API_FUNC HW_INT IOTA_BatchPropertiesReport(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum, HW_INT serviceLenList[]);
HW_API_FUNC HW_INT IOTA_CommandResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name, HW_CHAR *pcCommandResponse);
HW_API_FUNC HW_INT IOTA_PropertiesSetResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *result_desc);
HW_API_FUNC HW_INT IOTA_PropertiesGetResponse(HW_CHAR *requestId, ST_IOTA_SERVICE_DATA_INFO serviceProp[], HW_INT serviceNum);
HW_API_FUNC HW_INT IOTA_GetDeviceShadow(HW_CHAR *requestId, HW_CHAR *deviceId, HW_CHAR *service_id);
HW_API_FUNC HW_INT IOTA_ReportSubDeviceInfo(HW_CHAR *pcPayload);
HW_API_FUNC HW_INT IOTA_SubDeviceVersionReport(HW_CHAR *version);
HW_API_FUNC HW_INT IOTA_SubDeviceProductGetReport(cJSON *product_id_list);
HW_API_FUNC HW_INT IOTA_SubDeviceScanReport(cJSON *device_list);
HW_API_FUNC HW_INT IOTA_OTAVersionReport(ST_IOTA_OTA_VERSION_INFO otaVersionInfo);
HW_API_FUNC HW_INT IOTA_OTAStatusReport(ST_IOTA_UPGRADE_STATUS_INFO otaStatusInfo);
HW_API_FUNC HW_INT IOTA_DeviceStatusUpdate(int num, ST_IOTA_ONLINE_STATUS_INFO onlineStatus[]);
HW_API_FUNC SSL_CTX* IOTA_ssl_init(void);
HW_API_FUNC HW_INT IOTA_GetOTAPackages(HW_CHAR *url, HW_CHAR *token, HW_INT timeout);
HW_API_FUNC HW_INT IOTA_SubscribeUserTopic(HW_CHAR *topicParas);

#define OTA_PORT 					 8943
#define BUFSIZE 					 4096
#define PKGNAME_MAX 				 20  //the max length of the package name
#define HTTP_HEADER_LENGTH 			 500
#define IP_LENGTH 					 50
#define PKG_LENGTH 					 10
#define OTA_TIMEOUT_MIN_LENGTH 		 300
#define DOUBLE_OBLIQUE_LINE 		 "//"
#define SINGLE_SLANT 				 "/"
#define COLON 						 ":"
#define OTA_HTTP_GET 				 "GET "      //do not delete the blank space
#define OTA_HTTP_VERSION 			 " HTTP/1.1\n"   //do not delete the blank space and '\n'
#define OTA_HTTP_HOST 				 "Host: "    //do not delete the blank space
#define HTTP_OK 					 "200"
#define FILE_NAME 					 "filename="
#define OTA_LINEFEED 				 "\n"
#define OTA_CONTENT_TYPE 			 "Content-Type: application/json\n"  //do not delete the blank space and '\n
#define OTA_AUTH 					 "Authorization:Bearer "   //do not delete the blank space
#define OTA_CRLF 					 "\r\n\r\n"
#define OTA_CONTENT_LENGTH 			 "Content-Length: "
#define HTTP_STATUS_LENGTH 			 3
#define OTA_HTTP_RESPONSE_VERSION 	 "HTTP/1.1 "  //do not delete the blank space
#define SUB_DEVICE_VERSION_REPORT    "sub_device_list_version_report"
#define GET_PRODUCTS    			 "get_products"
#define SCAN_SUB_DEVICE_RESULT       "scan_result"
#define MESSAGE_NAME       			 "message_name"
#define VERSION                      "version"
#define PRODUCTID_LIST       		 "product_id_list"
#define DEVICE_LIST       			 "device_list"
#define SERVICES      			 	 "services"
#define DEVICES      			 	 "devices"
#define RESULT_CODE      			 "result_code"
#define RESULT      			 	 "result"
#define TARGET_DEVICE_ID      		 "target_device_id"
#define DEVICE_ID       			 "device_id"
#define SERVICE_ID       			 "service_id"
#define EVENT_TIME      			 "event_time"
#define PROPERTIES      			 "properties"
#define RESPONSE_NAME      			 "response_name"
#define PARAS  	    			 	 "paras"
#define RESULT_DESC      			 "result_desc"
#define CONTENT      				 "content"
#define ID      					 "id"
#define NAME	      				 "name"
#define OBJECT_DEVICE_ID      		 "object_device_id"
#define EVENT_TYPE      			 "event_type"
#define VERSION_REPORT      	     "version_report"
#define UPGRADE_PROGRESS_REPORT      "upgrade_progress_report"
#define SW_VERSION                   "sw_version"
#define FW_VERSION                   "fw_version"
#define PROGRESS                     "progress"
#define DESCRIPTION                  "description"
#define OBJECT_DEVICE_ID             "object_device_id"
#define OTA                          "$ota"
#define VERSION                      "version"
#define SERVICE_ID					 "service_id"

/**
 * ----------------------------deprecated below------------------------------------->
 */
#define IOTA_TOPIC_SERVICE_DATA_REPORT_RET    "IOTA_TOPIC_SERVICE_DATA_REPORT_RET"
#define IOTA_TOPIC_SERVICE_COMMAND_RECEIVE    "IOTA_TOPIC_SERVICE_COMMAND_RECEIVE"
#define IOTA_TOPIC_DATATRANS_REPORT_RSP       "IOTA_TOPIC_DATATRANS_REPORT_RSP"

typedef enum enum_EN_IOTA_DATATRANS_IE_TYPE {
	EN_IOTA_DATATRANS_IE_RESULT = 0,   //nsigned int  命令执行返回结果
	EN_IOTA_DATATRANS_IE_DEVICEID = 1,   //String        设备ID
	EN_IOTA_DATATRANS_IE_REQUESTID = 2,   //String        请求ID
	EN_IOTA_DATATRANS_IE_SERVICEID = 3,   //String        服务ID
	EN_IOTA_DATATRANS_IE_METHOD = 4,   //String        服务方法
	EN_IOTA_DATATRANS_IE_CMDCONTENT = 5,   //String        命令内容
} EN_IOTA_DATATRANS_IE_TYPE;

#endif
