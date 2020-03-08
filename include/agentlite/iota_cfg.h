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

#ifndef _IOTA_CFG_H_
#define _IOTA_CFG_H_

/**
 * enumeration values for item in IOTA_ConfigSetStr(HW_INT iItem, HW_CHAR *pValue)
 */
typedef enum {
	EN_IOTA_CFG_DEVICEID = 0,  //the value for this item is deviceId
	EN_IOTA_CFG_DEVICESECRET = 1,
	EN_IOTA_CFG_MQTT_ADDR = 2,
	EN_IOTA_CFG_MQTT_PORT = 3,
	EN_IOTA_CFG_AUTH_MODE = 4, //secret or cert mode;0 is secret mode, 1 is cert mode.
	EN_IOTA_CFG_LOG_LOCAL_NUMBER = 5, //take effect only when syslog is available
	EN_IOTA_CFG_LOG_LEVEL = 6, //take effect only when syslog is available
	EN_IOTA_CFG_KEEP_ALIVE_TIME = 7,
	EN_IOTA_CFG_CONNECT_TIMEOUT = 8,
	EN_IOTA_CFG_RETRY_INTERVAL = 9,
	EN_IOTA_CFG_RESET_SECRET_IN_PROGRESS = 10,
	EN_IOTA_CFG_QOS = 11,
	EN_IOTA_CFG_PRIVATE_KEY_PASSWORD = 12 //private key password in cert mode device
} EN_IOTA_CFG_TYPE;

typedef enum enum_IOTA_CFG_AUTH_MODE {
	EN_IOTA_CFG_AUTH_MODE_SECRET = 0,
	EN_IOTA_CFG_AUTH_MODE_CERT = 1,
} ENUM_IOTA_CFG_AUTH_MODE;

HW_API_FUNC HW_INT IOTA_ConfigSetStr(HW_INT iItem, HW_CHAR *pValue);
HW_API_FUNC HW_INT IOTA_ConfigSetUint(HW_INT iItem, HW_UINT uiValue);

#endif

