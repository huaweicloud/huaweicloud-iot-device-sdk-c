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
#ifndef IOTA_PAYLOAD_H
#define IOTA_PAYLOAD_H

#ifndef IOTA_DATATRANS_H
#include "iota_datatrans.h"
#endif
/**
* @brief Generate the payload for message reporting.
*
* @param[in] object_device_id The device ID.
* @param[in] name The name of the message.
* @param[in] id The ID of the message.
* @param[in] content The content of the message.
* @return The payload string.
*/
char *IOTA_MessageReportPayload(HW_CHAR *object_device_id, HW_CHAR *name, HW_CHAR *id, HW_CHAR *content);

/**
* @brief Generate the payload for properties reporting.
*
* @param[in] pServiceData The service data information.
* @param[in] serviceNum The number of services.
* @return The payload string.
*/
char *IOTA_PropertiesReportPayload(ST_IOTA_SERVICE_DATA_INFO pServiceData[], HW_INT serviceNum);

/**
* @brief Generate the payload for batch properties reporting.
*
* @param[in] pDeviceData The device data information.
* @param[in] deviceNum The number of devices.
* @param[in] serviceLenList The length of the service list.
* @return The payload string.
*/
char *IOTA_BatchPropertiesReportPayload(ST_IOTA_DEVICE_DATA_INFO pDeviceData[], HW_INT deviceNum,
   HW_INT serviceLenList[]);

/**
* @brief Generate the payload for command response.
*
* @param[in] result_code The result code.
* @param[in] response_name The name of the response.
* @param[in] pcCommandResponse The command response.
* @return The payload string.
*/
char *IOTA_CommandResponsePayload(HW_INT result_code, HW_CHAR *response_name, HW_CHAR *pcCommandResponse);

/**
* @brief Generate the payload for properties set response.
*
* @param[in] result_code The result code.
* @param[in] result_desc The result description.
* @return The payload string.
*/
char *IOTA_PropertiesSetResponsePayload(HW_INT result_code, HW_CHAR *result_desc);

/**
* @brief Generate the payload for properties get response.
*
* @param[in] serviceProp The service property information.
* @param[in] serviceNum The number of services.
* @return The payload string.
*/
char *IOTA_PropertiesGetResponsePayload(ST_IOTA_SERVICE_DATA_INFO serviceProp[], HW_INT serviceNum);

/**
* @brief Generate the payload for getting device shadow.
*
* @param[in] object_device_id The device ID.
* @param[in] service_id The service ID.
* @return The payload string.
*/
char *IOTA_GetDeviceShadowPayload(HW_CHAR *object_device_id, HW_CHAR *service_id);


/**
* @brief Update the status of sub-devices.
*
* @param[in] device_statuses Pointer to the device statuses structure.
* @param[in] deviceNum Number of devices.
* @return Pointer to the payload string.
*/
char *IOTA_UpdateSubDeviceStatusPayload(ST_IOTA_DEVICE_STATUSES *device_statuses, HW_INT deviceNum);

/**
* @brief Add sub-devices.
*
* @param[in] subDevicesInfo Pointer to the sub-devices information structure.
* @param[in] deviceNum Number of devices.
* @return Pointer to the payload string.
*/
char *IOTA_AddSubDevicePayload(ST_IOTA_SUB_DEVICE_INFO *subDevicesInfo, HW_INT deviceNum);

/**
* @brief Delete sub-devices.
*
* @param[in] delSubDevices Pointer to the sub-devices to be deleted structure.
* @param[in] deviceNum Number of devices.
* @return Pointer to the payload string.
*/
char *IOTA_DelSubDevicePayload(ST_IOTA_DEL_SUB_DEVICE *delSubDevices, HW_INT deviceNum);

/**
* @brief Report the OTA version information.
*
* @param[in] otaVersionInfo Pointer to the OTA version information structure.
* @return Pointer to the payload string.
*/
char *IOTA_OTAVersionReportPayload(ST_IOTA_OTA_VERSION_INFO otaVersionInfo);

/**
* @brief Report the OTA upgrade status information.
*
* @param[in] otaStatusInfo Pointer to the OTA upgrade status information structure.
* @return Pointer to the payload string.
*/
char *IOTA_OTAStatusReportPayload(ST_IOTA_UPGRADE_STATUS_INFO otaStatusInfo);

/**
 * @brief Get the file URL payload
 * @param file The file to download or upload
 * @param isup 1 for upload, 0 for download
 * @return The payload string, or NULL if failed
 */
char *IOTA_GetFileUrlPayload(const ST_IOTA_DOWNLOAD_FILE *file, int isup);

char *IOTA_GetLatestSoftBusInfoPayload(HW_CHAR *busId, HW_CHAR *eventId);

char *IOTA_ReportDeviceInfoPayload(ST_IOTA_DEVICE_INFO_REPORT *device_info_report);

char *IOTA_ReportDeviceLogPayload(HW_CHAR *type, HW_CHAR *content, HW_CHAR *timestamp);

char *IOTA_GetNTPTimePayload(void);
#endif
