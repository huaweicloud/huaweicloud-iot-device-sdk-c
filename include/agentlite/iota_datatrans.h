/*
 * Copyright (c) 2020-2022 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#ifndef _IOTA_DATATRANS_H_
#define _IOTA_DATATRANS_H_

#include <openssl/ossl_typ.h>
#include "cJSON.h"
#include "hw_type.h"
#include "mqttv5_util.h"

#define MaxServiceReportNum 10  // set the max count of reported services
#define MaxSubDeviceCount 100
#define MaxAddedSubDevCount 50
#define MaxDelSubDevCount 50

typedef struct {
	HW_CHAR *service_id; // the device service id obtained from the profile
	HW_CHAR *event_time; // UTC time, e.g. 20190531T011540Z
	HW_CHAR *properties; // the service data obtained from the profile can be parsed into JSON
} ST_IOTA_SERVICE_DATA_INFO;

typedef struct {
	HW_CHAR *device_id;
	ST_IOTA_SERVICE_DATA_INFO services[MaxServiceReportNum];
} ST_IOTA_DEVICE_DATA_INFO;

typedef struct {
	HW_CHAR *event_time; // UTC time, e.g. 20190531T011540Z
	HW_CHAR *sw_version; // software version
	HW_CHAR *fw_version; // firmware version
	HW_CHAR *object_device_id; // the target device id, NULL means the target device id is the gateway device id
} ST_IOTA_OTA_VERSION_INFO;

typedef struct {
	HW_CHAR *device_id;
	HW_CHAR *status;
} ST_IOTA_DEVICE_STATUS;

typedef struct {
	HW_CHAR *event_time; // UTC time, e.g. 20190531T011540Z
	ST_IOTA_DEVICE_STATUS device_statuses[MaxSubDeviceCount];
} ST_IOTA_DEVICE_STATUSES;

typedef struct {
	HW_CHAR *parent_device_id;
	HW_CHAR *node_id;
	HW_CHAR *device_id;
	HW_CHAR *name;
	HW_CHAR *description;
	HW_CHAR *product_id;
	HW_CHAR *extension_info;
} ST_IOTA_DEVICE_INFO;


typedef struct {
	HW_CHAR *event_time; // UTC time, e.g. 20190531T011540Z
	HW_CHAR *event_id; // event id, If the parameter is not used, it is automatically generated by the iot platform, and the generation rule is a 36 bit random string composed of numbers, letters and middle dashes
	ST_IOTA_DEVICE_INFO deviceInfo[MaxAddedSubDevCount];
} ST_IOTA_SUB_DEVICE_INFO;

typedef struct {
	HW_CHAR *event_time; // UTC time, e.g. 20190531T011540Z
	HW_CHAR *event_id; // event id, If the parameter is not used, it is automatically generated by the iot platform, and the generation rule is a 36 bit random string composed of numbers, letters and middle dashes
	HW_CHAR *delSubDevice[MaxDelSubDevCount]; // the array of deleted sub devices
} ST_IOTA_DEL_SUB_DEVICE;

typedef struct 
{
	HW_CHAR *object_device_id;
	HW_CHAR *name;
	HW_CHAR *id;
	HW_CHAR *content;
	HW_CHAR *topicParas; 
} ST_IOTA_MESS_REP_INFO;


/**
 * ST_IOTA_UPGRADE_STATUS_INFO:
 * event_time: UTC time, e.g. 20190531T011540Z
 * result_code: The upgrade status of the device. The result code is defined as follows:
				0 processed successfully,
				1. Equipment in use,
				2. Poor signal quality,
				3. It is the latest version,
				4. Insufficient power,
				5. Insufficient space,
				6. Download timeout,
				7. Upgrade package verification failed,
				8. The upgrade package type is not supported,
				9. Out of memory,
				10. Failed to install upgrade package,
				255 internal exception
 * progress: Upgrade progress of the device, range: 0 to 100
 * description: description
 * object_device_id: the target device id, NULL means the target device id is the gateway device id
 * version: Current version of the device
 */
typedef struct {
	HW_CHAR *event_time; // UTC time, e.g. 20190531T011540Z
	HW_INT result_code;
	HW_INT progress;
	HW_CHAR *description;
	HW_CHAR *object_device_id;
	HW_CHAR *version;
} ST_IOTA_UPGRADE_STATUS_INFO;

typedef struct {
	HW_INT mid; // command id
	HW_INT errcode; // 0 for success, 1 for failure
	HW_CHAR *body; // the response body obtained from the profile can be parsed into JSON
} ST_IOTA_COMMAND_RSP_V3;

typedef struct {
	HW_CHAR *object_device_id;
	HW_CHAR *event_time;
	HW_CHAR *device_sdk_version;
	HW_CHAR *sw_version;
	HW_CHAR *fw_version;
} ST_IOTA_DEVICE_INFO_REPORT;

/**
 *@Description: report message to IoT platform
 *@param object_device_id: the target device id, NULL means the target device id is the gateway device id
 *@param name: the message name
 *@param id: the message id
 *@param content: the message content
 *@param topicParas: customize the topic parameters, such as "devmsg" (do not add '/' or special characters in front of it)
 *					 If it is set to NULL, the platform default topic will be used for reporting.
 *@param compressFlag : 0 for no compression, 1 for compression
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_MessageReport(HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content, HW_CHAR *topicParas, HW_INT compressFlag, void *context);

/**
 *@Description: report device properties to IoT platform
 *@param pServiceData[]: the array of ST_IOTA_SERVICE_DATA_INFO structure
 *@param serviceNum: number of reported services
 *@param compressFlag : 0 for no compression, 1 for compression
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_PropertiesReport(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum, HW_INT compressFlag, void *context);

/**
 *@Description: batch report data of the sub devices to IoT platform
 *@param pDeviceData[]: the array of ST_IOTA_DEVICE_DATA_INFO structure.
 *@param deviceNum: number of reported sub device
 *@param serviceLenList[]: the array of number of services reported by sub equipment
 *@param compressFlag : 0 for no compression, 1 for compression
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BatchPropertiesReport(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum, HW_INT serviceLenList[], HW_INT compressFlag, void *context);

/**
 *@Description: response command
 *@param requestId: unique identification of the request
 *@param result_code: command execution result, 0 for success, others for failure
 *@param response_name: name of the command response
 *@param pcCommandResponse: command response results obtained from the profile can be parsed into JSON
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_CommandResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name, HW_CHAR *pcCommandResponse, void *context);

/**
 *@Description: response properties setting
 *@param requestId: unique identification of the request
 *@param result_code: command execution result, 0 for success, others for failure
 *@param result_desc: result description
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_PropertiesSetResponse(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *result_desc, void *context);

/**
 *@Description: response properties getting
 *@param requestId: unique identification of the request
 *@param serviceProp[]: the array of ST_IOTA_SERVICE_DATA_INFO structure
 *@param serviceNum: number of reported services
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_PropertiesGetResponse(HW_CHAR *requestId, ST_IOTA_SERVICE_DATA_INFO serviceProp[], HW_INT serviceNum, void *context);

/**
 *@Description: get device shadow data
 *@param requestId: unique identification of the request
 *@param object_device_id: the target device id, NULL means the target device id is the gateway device id
 *@param service_id: the device service id obtained from the profile
 *@param context: A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_GetDeviceShadow(HW_CHAR *requestId, HW_CHAR *object_device_id, HW_CHAR *service_id, void *context);

/**
 *@Description: Reserved interface for transparent transmission
 *@param pcPayload: Load structure
 *@param context: A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_ReportSubDeviceInfo(HW_CHAR *pcPayload, void *context);

/**
 *@Description: Submit the version number of the sub equipment
 *@param version: Sub device version
 *@param context: A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_SubDeviceVersionReport(HW_CHAR *version, void *context);

HW_API_FUNC HW_INT IOTA_SubDeviceProductGetReport(cJSON *product_id_list, void *context);

// to do: participation is struct  ST_IOTA_DEVICE_INFO under iota_device_info.h
HW_API_FUNC HW_INT IOTA_SubDeviceScanReport(cJSON *device_list, void *context);

/**
 *@Description: report OTA version
 *@param otaVersionInfo: ST_IOTA_OTA_VERSION_INFO structure
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_OTAVersionReport(ST_IOTA_OTA_VERSION_INFO otaVersionInfo, void *context);

/**
 *@Description: report OTA status
 *@param otaVersionInfo: ST_IOTA_UPGRADE_STATUS_INFO structure
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_OTAStatusReport(ST_IOTA_UPGRADE_STATUS_INFO otaStatusInfo, void *context);

HW_API_FUNC SSL_CTX* IOTA_ssl_init(void);

/**
 *@Description: get OTA packages
 *@param url: package download address
 *@param token: Temporary token of the download address of the package URL
 *@param timeout: The timeout for requesting to download the package, which needs to be greater than 300 seconds. Less than 24 hours is recommended
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_GetOTAPackages(HW_CHAR *url, HW_CHAR *token, HW_INT timeout);

/**
 *@Description: subscribe user topic
 *@param topicParas: customize the topic parameters, such as "devmsg" (do not add '/' or special characters in front of it)
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_SubscribeUserTopic(HW_CHAR *topicParas);

/**
 *@Description: subscribe full topic
 *@param topic: the full topic
 *@param qos: qos
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_SubscribeTopic(HW_CHAR *topic, HW_INT qos);

/**
 *@Description: report device properties to IoT platform by V3 topic
 *@param pServiceData[]: the array of ST_IOTA_SERVICE_DATA_INFO structure
 *@param serviceNum: number of reported services
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_PropertiesReportV3(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum, void *context);

/**
 *@Description: report binary to IoT platform by V3 topic
 *@param payload: hexadecimal data
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BinaryReportV3(HW_CHAR *payload, void *context);

/**
 *@Description: response command by V3 topic
 *@param cmdRspV3: ST_IOTA_COMMAND_RSP_V3 structure
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_CmdRspV3(ST_IOTA_COMMAND_RSP_V3 *cmdRspV3, void *context);

/**
 *@Description: get NTP time.Value is returned in the event callback function
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_GetNTPTime(void *context);

/**
 *@Description: get the access address
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_Bootstrap();

/**
 *@Description: subscribe the V3 topic of json command . Not recommended
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_SubscribeJsonCmdV3();

/**
 *@Description: subscribe the V3 topic of binary command . Not recommended
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_SubsrcibeBinaryCmdV3();

/**
 *@Description: report sub device satuses
 *@param device_statuses[]: the array of ST_IOTA_DEVICE_DATA_INFO structure.
 *@param deviceNum: number of sub device needed to report status
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_UpdateSubDeviceStatus(ST_IOTA_DEVICE_STATUSES *device_statuses, HW_INT deviceNum, void *context);

/**
 *@Description: subscribe boostrap topic
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_SubscribeBoostrap();

/**
 *@Description: gateway adds sub device
 *@param subDevicesInfo: the pointer of ST_IOTA_SUB_DEVICE_INFO structure.
 *@param deviceNum: number of sub device needed to add
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_AddSubDevice(ST_IOTA_SUB_DEVICE_INFO *subDevicesInfo, HW_INT deviceNum, void *context);

/**
 *@Description: gateway deletes sub device
 *@param delSubDevices: the pointer of ST_IOTA_DEL_SUB_DEVICE structure.
 *@param deviceNum: number of sub device needed to delete
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_DelSubDevice(ST_IOTA_DEL_SUB_DEVICE *delSubDevices, HW_INT deviceNum, void *context);

/**
 *@Description: report device log to the iot platform
 *@param type: the type of device log, it can only be as follows 
 	 	 	   DEVICE_STATUS, DEVICE_PROPERTY, DEVICE_MESSAGE, DEVICE_COMMAND
 *@param content: the log content
 *@param timestamp: time stamp accurated to milliseconds
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_ReportDeviceLog(HW_CHAR *type, HW_CHAR *content, HW_CHAR *timestamp, void *context);
HW_API_FUNC HW_INT IOTA_GetLatestSoftBusInfo(HW_CHAR *bus_id, HW_CHAR *event_id, void *context);
#if defined(MQTTV5)
/**
 *@Description: MQTTV5 response command
 *@param requestId: unique identification of the request
 *@param result_code: command execution result, 0 for success, others for failure
 *@param response_name: name of the command response
 *@param pcCommandResponse: command response results obtained from the profile can be parsed into JSON
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@param properties: the MQTTV5_DATA structure	
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_CommandResponseV5(HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name, HW_CHAR *pcCommandResponse, void *context, MQTTV5_DATA *properties);

/**
 *@Description: MQTT5.0 batch report data of the sub devices to IoT platform
 *@param pDeviceData[]: the array of ST_IOTA_DEVICE_DATA_INFO structure.
 *@param deviceNum: number of reported sub device
 *@param serviceLenList[]: the array of number of services reported by sub equipment
 *@param compressFlag : 0 for no compression, 1 for compression
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@param mqttv5: the MQTTV5_DATA structure	
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BatchPropertiesReportV5(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum, HW_INT serviceLenList[], HW_INT compressFlag, void *context, MQTTV5_DATA *mqttv5);

/**
 *@Description: MQTT5.0 report device properties to IoT platform
 *@param pServiceData[]: the array of ST_IOTA_SERVICE_DATA_INFO structure
 *@param serviceNum: number of reported services
 *@param compressFlag : 0 for no compression, 1 for compression
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@param mqttv5 : the MQTTV5_DATA structure
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_PropertiesReportV5(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum, HW_INT compressFlag, void *context, MQTTV5_DATA *mqttv5);

/**
 *@Description: MQTT5.0 report message to IoT platform
 *@param mass : the ST_IOTA_MESS_REP_INFO structure
 *@param compressFlag : 0 for no compression, 1 for compression
 *@param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success or failure callback functions to
    			   provide access to the context information in the callback.
 *@param mqttv5 : the MQTTV5_DATA structure
 *@return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_MessageReportV5(ST_IOTA_MESS_REP_INFO mass, HW_INT compressFlag, void *context, MQTTV5_DATA *mqttv5);
#endif

#define SDK_VERSION					 "C_v1.0.1"
#define OTA_PORT 					 8943
#define BUFSIZE 					 4096
#define PKGNAME_MAX 				 20  // the max length of the package name
#define HTTP_HEADER_LENGTH 			 500
#define IP_LENGTH 					 50
#define PKG_LENGTH 					 10
#define OTA_TIMEOUT_MIN_LENGTH 		 300
#define DOUBLE_OBLIQUE_LINE 		 "//"
#define SINGLE_SLANT 				 "/"
#define COLON 						 ":"
#define OTA_HTTP_GET 				 "GET "      // do not delete the blank space
#define OTA_HTTP_VERSION 			 " HTTP/1.1\n"   // do not delete the blank space and '\n'
#define OTA_HTTP_HOST 				 "Host: "    // do not delete the blank space
#define HTTP_OK 					 "200"
#define FILE_NAME 					 "filename="
#define OTA_LINEFEED 				 "\n"
#define OTA_CONTENT_TYPE 			 "Content-Type: application/json\n"  // do not delete the blank space and '\n
#define OTA_AUTH 					 "Authorization:Bearer "   // do not delete the blank space
#define OTA_CRLF 					 "\r\n\r\n"
#define OTA_CONTENT_LENGTH 			 "Content-Length: "
#define HTTP_STATUS_LENGTH 			 3
#define OTA_HTTP_RESPONSE_VERSION 	 "HTTP/1.1 "  // do not delete the blank space
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
#define EVENT_ID					 "event_id"
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
#define SUB_DEVICE_MANAGER			 "$sub_device_manager"
#define ADD_SUB_DEVICE_NOTIFY		 "add_sub_device_notify"
#define PARENT_DEVICE_ID 			 "parent_device_id"
#define NODE_ID 					 "node_id"
#define MANUFACTURER_ID				 "manufacturer_id"
#define MODEL					     "model"
#define PRODUCT_ID					 "product_id"
#define FW_VERSION					 "fw_version"
#define SW_VERSION					 "sw_version"
#define EXTENSION_INFO				 "extension_info"
#define STATUS					     "status"
#define DELETE_SUB_DEVICE_NOTIFY	 "delete_sub_device_notify"
#define ADD_SUB_DEVICE_RESPONSE		 "add_sub_device_response"
#define DEL_SUB_DEVICE_RESPONSE		 "delete_sub_device_response"
#define SUCCESSFUL_DEVICES			 "successful_devices"
#define FAILED_DEVICES				 "failed_devices"
#define ERROR_CODE					 "error_code"
#define ERROR_MSG					 "error_msg"
#define FIRMWARE_UPGRADE			 "firmware_upgrade"
#define SOFTWARE_UPGRADE			 "software_upgrade"
#define URL							 "url"
#define FILE_SIZE					 "file_size"
#define ACCESS_TOKEN				 "access_token"
#define EXPIRES						 "expires"
#define SIGN						 "sign"
#define DATA_V3						 "data"
#define SERVICE_ID_V3				 "serviceId"
#define EVENT_TIME_V3      			 "eventTime"
#define SERVICE_DATA_V3      		 "serviceData"
#define MSGTYPE						 "msgType"
#define DEVIVE_REQ					 "deviceReq"
#define CMD							 "cmd"
#define MID							 "mid"
#define ERR_CODE					 "errcode"
#define BODY						 "body"
#define DEVIVE_RSP					 "deviceRsp"
#define VERSION_QUERY				 "version_query"
#define COMMAND_NAME 				 "command_name"
#define SHADOW						 "shadow"
#define DESIRED						 "desired"
#define REPORTED					 "reported"
#define SDK_INFO					 "$sdk_info"
#define SDK_INFO_REPORT				 "sdk_info_report"
#define TYPE						 "type"
#define SDK_LANGUAGE				 "C"
#define SDK_TIME					 "$time_sync"
#define SDK_NTP_REQUEST				 "time_sync_request"
#define DEVICE_SEND_TIME			 "device_send_time"
#define TIME_SYNC_RSP				 "time_sync_response"
#define SERVER_RECV_TIME			 "server_recv_time"
#define SERVER_SEND_TIME			 "server_send_time"
#define VERSION_JSON				 "\"version\""
#define DEVICE_SEND_TIME_JSON		 "\"device_send_time\""
#define SERVER_RECV_TIME_JSON		 "\"server_recv_time\""
#define SERVER_SEND_TIME_JSON		 "\"server_send_time\""
#define DEVICE_STATUS				 "device_statuses"
#define SUB_DEVICE_UPDATE_STATUS	 "sub_device_update_status"
#define ADD_SUB_DEVICE_REQUEST		 "add_sub_device_request"
#define DEL_SUB_DEVICE_REQUEST		 "delete_sub_device_request"
#define ONLINE						 "ONLINE"
#define OFFLINE						 "OFFLINE"
#define ADDRESS						 "address"
#define LOG							 "$log"
#define LOG_CONFIG					 "log_config"
#define SWITCH						 "switch"
#define END_TIME					 "end_time"
#define LOG_REPORT					 "log_report"
#define TIMESTAMP					 "timestamp"
#define DEVICE_SDK_VERSION			 "device_sdk_version"
#define SW_VERSION					 "sw_version"
#define FW_VERSION					 "fw_version"
#define SOFT_BUS_EVENT_REQ 			 "soft_bus_config_request"
#define SOFT_BUS_EVENT_RSP 			 "soft_bus_config_response"
#define SOFT_BUS_SERVICEID			 "$oh_soft_bus"
#define BUS_ID						 "bus_id"

#define DEVICE_RULE                 "$device_rule"
#define CONFIG_REQUEST              "device_rule_config_request"
#define CONFIG_RESPONSE             "device_rule_config_response"
#define DEVICE_RULE_REQUEST_ID      "device_rule_request_id"
#define RULE_ID	                    "ruleIds"
/**
 * ----------------------------deprecated below------------------------------------->
 */
#define IOTA_TOPIC_SERVICE_DATA_REPORT_RET    "IOTA_TOPIC_SERVICE_DATA_REPORT_RET"
#define IOTA_TOPIC_SERVICE_COMMAND_RECEIVE    "IOTA_TOPIC_SERVICE_COMMAND_RECEIVE"
#define IOTA_TOPIC_DATATRANS_REPORT_RSP       "IOTA_TOPIC_DATATRANS_REPORT_RSP"


typedef enum enum_EN_IOTA_DATATRANS_IE_TYPE {
	EN_IOTA_DATATRANS_IE_RESULT = 0,   // unsigned int  命令执行返回结果
	EN_IOTA_DATATRANS_IE_DEVICEID = 1,   // String        设备ID
	EN_IOTA_DATATRANS_IE_REQUESTID = 2,   // String        请求ID
	EN_IOTA_DATATRANS_IE_SERVICEID = 3,   // String        服务ID
	EN_IOTA_DATATRANS_IE_METHOD = 4,   // String        服务方法
	EN_IOTA_DATATRANS_IE_CMDCONTENT = 5,   // String        命令内容
} EN_IOTA_DATATRANS_IE_TYPE;

#endif