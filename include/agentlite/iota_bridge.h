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
#ifndef IOTA_BRIDGE_H
#define IOTA_BRIDGE_H

#define IOTA_SIGN_TYPE      "sign_type"
#define IOTA_TIMESTAMP      "timestamp"
#define IOTA_PASSWORD       "password"
#define IOTA_OLD_SECRET     "old_secret"
#define IOTA_NEW_SECRET     "new_secret"

#ifndef HW_TYPE_H
#include "hw_type.h"
#endif 

#ifndef IOTA_DATATRANS_H
#include "iota_datatrans.h"
#endif 

/**
 * Bridge device login
 *
 * @param deviceId: device id
 * @param password: device password
 * @param requestId: request id
 * @param context: context
 * @return: message id
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceLogin(HW_CHAR *deviceId, HW_CHAR *password, HW_CHAR *requestId, void *context);

/**
 * Bridge device logout
 *
 * @param deviceId: device id
 * @param requestId: request id
 * @param context: context
 * @return: message id
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceLogout(HW_CHAR *deviceId, HW_CHAR *requestId, void *context);

/**
 * Bridge device reset secret
 *
 * @param deviceId: device id
 * @param oldSecret: old secret
 * @param newSecret: new secret
 * @param requestId: request id
 * @param context: context
 * @return: message id
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceResetSecret(HW_CHAR *deviceId, HW_CHAR *oldSecret, HW_CHAR *newSecret, HW_CHAR *requestId, void *context);

/**
 * Bridge device messages report
 *
 * @param deviceId: device id
 * @param object_device_id: object device id
 * @param name: message name
 * @param id: message id
 * @param content: message content
 * @param context: context
 * @return: message id
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceMessagesReport(HW_CHAR *deviceId, HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content, void *context);

/**
 * Bridge device properties report
 *
 * @param deviceId: device id
 * @param pServiceData: service data
 * @param serviceNum: service number
 * @param context: context
 * @return: message id
 */
HW_API_FUNC HW_INT IOTA_BridgeDevicePropertiesReport(HW_CHAR *deviceId, ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum, void *context);

/**
 * Bridge device batch properties report
 *
 * @param deviceId: device id
 * @param pServiceData: service data
 * @param serviceNum: service number
 * @param serviceLenList: service length list
 * @param context: context
 * @return: message id
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceBatchPropertiesReport(HW_CHAR *deviceId, ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum, HW_INT serviceLenList[], void *context);

/**
 * Bridge device properties set response
 *
 * @param deviceId: device id
 * @param requestId: request id
 * @param result_code: result code
 * @param result_desc: result description
 * @param context: context
 * @return: message id
 */
HW_API_FUNC HW_INT IOTA_BridgeDevicePropertiesSetResponse(HW_CHAR *deviceId, HW_CHAR *requestId, HW_INT result_code, HW_CHAR *result_desc, void *context);

/**
 * Bridge device properties get response
 *
 * @param deviceId: device id
 * @param requestId: request id
 * @param serviceProp: service property
 * @param serviceNum: service number
 * @param context: context
 * @return: message id
 */
HW_API_FUNC HW_INT IOTA_BridgeDevicePropertiesGetResponse(HW_CHAR *deviceId, HW_CHAR *requestId, ST_IOTA_SERVICE_DATA_INFO serviceProp[], HW_INT serviceNum, void *context);

/**
 * Bridge device get device shadow
 *
 * @param deviceId: device id
 * @param requestId: request id
 * @param object_device_id: object device id
 * @param service_id: service id
 * @param context: context
 * @return: message id
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceGetDeviceShadow(HW_CHAR *deviceId, HW_CHAR *requestId, HW_CHAR *object_device_id, HW_CHAR *service_id, void *context);

/**
 * Bridge device command response
 *
 * @param deviceId: device id
 * @param requestId: request id
 * @param result_code: result code
 * @param response_name: response name
 * @param pcCommandResponse: command response
 * @param context: context
 * @return: message id
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceCommandResponse(HW_CHAR *deviceId, HW_CHAR *requestId, HW_INT result_code, HW_CHAR *response_name, HW_CHAR *pcCommandResponse, void *context);

/**
 * @Description:  Bridge Device report sub device satuses
 * @param deviceId: device id
 * @param device_statuses[]: the array of ST_IOTA_DEVICE_DATA_INFO structure.
 * @param deviceNum: number of sub device needed to report status
 * @param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success
 *                  or failure callback functions to provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceUpdateSubDeviceStatus(HW_CHAR *deviceId, ST_IOTA_DEVICE_STATUSES *device_statuses, HW_INT deviceNum, void *context);

/**
 * @Description: Bridge Device gateway adds sub device
 * @param deviceId: bridge id
 * @param subDevicesInfo: the pointer of ST_IOTA_SUB_DEVICE_INFO structure.
 * @param deviceNum: number of sub device needed to add
 * @param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success
 *                  or failure callback functions to provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceAddSubDevice(HW_CHAR *deviceId, ST_IOTA_SUB_DEVICE_INFO *subDevicesInfo, HW_INT deviceNum, void *context);

/**
 * @Description: Bridge Device gateway deletes sub device
 * @param deviceId: bridge id
 * @param delSubDevices: the pointer of ST_IOTA_DEL_SUB_DEVICE structure.
 * @param deviceNum: number of sub device needed to delete
 * @param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success
                    or failure callback functions to provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceDelSubDevice(HW_CHAR *deviceId, ST_IOTA_DEL_SUB_DEVICE *delSubDevices, HW_INT deviceNum, void *context);

/**
 * @Description: Bridge Device get the url for uploading file
 * @param deviceId: bridge id
 * @param upload: the pointer of ST_IOTA_UPLOAD_FILE structure.
 * @param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success
                    or failure callback functions to provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceGetUploadFileUrl(HW_CHAR *deviceId, const ST_IOTA_UPLOAD_FILE *upload, void *context);

/**
 * @Description: Bridge Device get the url for Downloading file
 * @param deviceId: bridge id
 * @param upload: the pointer of ST_IOTA_DOWNLOAD_FILE structure.
 * @param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success
                    or failure callback functions to provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceGetDownloadFileUrl(HW_CHAR *deviceId, const ST_IOTA_DOWNLOAD_FILE *download, void *context);

/**
 * @Description: Bridge Device report device info to the iot platform
 * @param deviceId: bridge id
 * @param device_info_report: ST_IOTA_DEVICE_INFO_REPORT structure
 * @param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success
 *                  or failure callback functions to provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceReportDeviceInfo(HW_CHAR *deviceId, ST_IOTA_DEVICE_INFO_REPORT *device_info_report, void *context);

/**
 * @Description: Bridge Device get NTP time.Value is returned in the event callback function
 * @param deviceId: bridge id
 * @param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success
 *                  or failure callback functions to provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceGetNTPTime(HW_CHAR *deviceId, void *context);

 /**
  * @Description: Bridge Device get newest soft bus info from the iotda
  * @param deviceId: bridge id
  * @param bus_id: the bus id of the soft bus group.
  * @param event_id: if the parameter is not used, it is automatically generated by the iot platform, and the generation
  *                  rule is a 36 bit random string composed of numbers, letters and middle dashes
  * @param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success
  *                  or failure callback functions to provide access to the context information in the callback.
  * @return: IOTA_SUCCESS represents success, others represent specific failure
  */
HW_API_FUNC HW_INT IOTA_BridgeDeviceGetLatestSoftBusInfo(HW_CHAR *deviceId, HW_CHAR *busId, HW_CHAR *eventId, void *context);

/**
 * @Description: Bridge Device report device log to the iot platform
 * @param deviceId: bridge id
 * @param type: the type of device log, it can only be as follows
             DEVICE_STATUS, DEVICE_PROPERTY, DEVICE_MESSAGE, DEVICE_COMMAND
 * @param content: the log content
 * @param timestamp: time stamp accurated to milliseconds
 * @param context:  A pointer to any application-specific context. The the <i>context</i> pointer is passed to success
 *                  or failure callback functions to provide access to the context information in the callback.
 * @return: IOTA_SUCCESS represents success, others represent specific failure
 */
HW_API_FUNC HW_INT IOTA_BridgeDeviceReportDeviceLog(HW_CHAR *deviceId, HW_CHAR *type, HW_CHAR *content, HW_CHAR *timestamp, void *context);

/**
 * @brief Bridge Connect
 * @return Success returns 0
 */
int IOTA_BridgeConnect(void);

/**
 * @brief Is the Bridge device connected
 * @return Connected returns 1, unconnected returns 0
 */
int IOTA_BridgeIsConnected(void);

/**
 * @brief Bridge DisConnect
 * @return Success returns 0
 */
int IOTA_BridgeDisConnect(void);
#endif 
