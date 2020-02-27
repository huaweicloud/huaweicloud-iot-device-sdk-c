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

#ifndef BASE_H_
#define BASE_H_

//see also EN_MQTT_BASE_CALLBACK_SETTING in MqttBase.h
typedef enum enum_BASE_CONFIG {
	EN_BASE_CONFIG_USERNAME = 0,
	EN_BASE_CONFIG_PASSWORD = 1,
	EN_BASE_CONFIG_SERVER_ADDR = 5,
	EN_BASE_CONFIG_SERVER_PORT = 6,
	EN_BASE_CONFIG_AUTH_MODE = 9,
	EN_BASE_CONFIG_LOG_LOCAL_NUMBER = 10,
	EN_BASE_CONFIG_LOG_LEVEL = 11,
	EN_BASE_CONFIG_KEEP_ALIVE_TIME = 12,
	EN_BASE_CONFIG_CONNECT_TIMEOUT = 13,
	EN_BASE_CONFIG_RETRY_INTERVAL = 14,
	EN_BASE_CONFIG_RESET_SECRET_IN_PROGRESS = 15,
	EN_BASE_CONFIG_QOS = 16,
} ENUM_BASE_CONFIG;

#if defined(WIN32) || defined(WIN64) && defined(EXPORT_SERVICE)
#define _DLLEXPORT __declspec(dllexport)
#else
#define _DLLEXPORT
#endif

_DLLEXPORT int init(char *workPath);
_DLLEXPORT int SetConfig(int item, char *value);
_DLLEXPORT int destory(void);

#endif /*BASE_H_*/

