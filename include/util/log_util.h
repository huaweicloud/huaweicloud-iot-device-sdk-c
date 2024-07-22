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

#ifndef LOG_UTIL_H
#define LOG_UTIL_H

#include <stdarg.h>
#define LOG_FILE_ENABLE                  0  // If equal to 1, output to file; if equal to 0, output to log
#define LOG_FILE_NAME                    "./client.log"  // Mqtt debug output file address

typedef void (*PRINTF_LOG_CALLBACK_HANDLER)(int level, char *format, va_list args);
void SetPrintfLogCallback(PRINTF_LOG_CALLBACK_HANDLER callback);
void PrintfLog(int logLevel, char *_Format, ...);

// this compiling macro _SYS_LOG can only be used in Linux system
#ifdef _SYS_LOG

#include "syslog.h"

void InitLogForLinux();
void SetLogLocalNumber(int logLocalNumber);
void SetLogLevel(int logLevel);
void DestoryLogForLinux();

typedef enum {
    EN_LOG_LEVEL_DEBUG = LOG_DEBUG,
    EN_LOG_LEVEL_INFO = LOG_INFO,
    EN_LOG_LEVEL_WARNING = LOG_WARNING,
    EN_LOG_LEVEL_ERROR = LOG_ERR,
} LOGLEVEL;

#else

typedef enum enum_LOG_LEVEL {
    EN_LOG_LEVEL_DEBUG,
    EN_LOG_LEVEL_INFO,
    EN_LOG_LEVEL_WARNING,
    EN_LOG_LEVEL_ERROR,
    // for MQTT debug printing
    EN_LOG_LEVEL_MQTT_MAXIMUM = 8,
    EN_LOG_LEVEL_MQTT_MEDIUM,
    EN_LOG_LEVEL_MQTT_MINIMUM,
    EN_LOG_LEVEL_MQTT_PROTOCOL,
    EN_LOG_LEVEL_MQTT_ERROR,
    EN_LOG_LEVEL_MQTT_SEVERE,
    EN_LOG_LEVEL_MQTT_FATAL,
} LOGLEVEL;
#endif

#endif /* LOG_UTIL_H */
