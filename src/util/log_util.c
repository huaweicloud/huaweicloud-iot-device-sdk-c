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
#include <limits.h>
#include "string_util.h"
#include "log_util.h"

PRINTF_LOG_CALLBACK_HANDLER PrintfLogCb = NULL;
void SetPrintfLogCallback(PRINTF_LOG_CALLBACK_HANDLER callback)
{
    PrintfLogCb = callback;
}

#ifdef _SYS_LOG
#include "syslog.h"

void InitLogForLinux()
{
    openlog("OC_MQTT_LOG ", LOG_PID | LOG_CONS, LOG_USER);
}

void SetLogLocalNumber(int logLocalNumber)
{
    closelog();
    openlog("OC_MQTT_LOG ", LOG_PID | LOG_CONS, logLocalNumber);
}

void SetLogLevel(int logLevel)
{
    setlogmask(LOG_UPTO(logLevel));
}

void DestoryLogForLinux()
{
    closelog();
}

#endif

#if LOG_FILE_ENABLE
int g_fileSign = 1;
FILE *fp = NULL;
int FileWrite(char *format, va_list args)
{
    if (format == NULL) {
        return -1;
    }

    int ret = 0;
    if (fp == NULL) {
        char fileWay[PATH_MAX] = {0};
        realpath(LOG_FILE_NAME, fileWay);
        
        fp = fopen(fileWay, "a+");
        if (fp == NULL) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "ERROR FileWrite() fopen err! fileWay = %s\n", fileWay);
            return -1;
        }
        PrintfLog(EN_LOG_LEVEL_INFO, "FileWrite() fileName = %s\n", fileWay);
    }
    
    ret = vfprintf(fp, format, args);
    if (ret == EOF) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "FileWrite() fputs err!\n");
    }
    (void)fflush(fp);
    return ret;
}
#endif

void PrintfLog(int logLevel, char *_Format, ...)
{
    if (_Format == NULL) {
        return;
    }
    va_list args;
    va_start(args, _Format);

    char *format = NULL;
    char *tempStr = NULL;
    char *prefix = NULL;
    switch (logLevel) {
        case EN_LOG_LEVEL_DEBUG:
            prefix = "DEBUG ";
            break;
        case EN_LOG_LEVEL_INFO:
            prefix = "INFO ";
            break;
        case EN_LOG_LEVEL_WARNING:
            prefix = "WARNING ";
            break;
        case EN_LOG_LEVEL_ERROR:
            prefix = "ERROR ";
            break;
        case EN_LOG_LEVEL_MQTT_MAXIMUM:
            prefix = "MQTT_MAXIMUM ";
            break;
        case EN_LOG_LEVEL_MQTT_MEDIUM:
            prefix = "MQTT_MEDIUM ";
            break;
        case EN_LOG_LEVEL_MQTT_MINIMUM:
            prefix = "MQTT_MINIMUM ";
            break;
        case EN_LOG_LEVEL_MQTT_PROTOCOL:
            prefix = "MQTT_PROTOCOL ";
            break;
        case EN_LOG_LEVEL_MQTT_ERROR:
            prefix = "MQTT_ERROR ";
            break;
        case EN_LOG_LEVEL_MQTT_SEVERE:
            prefix = "MQTT_SEVERE ";
            break;
        case EN_LOG_LEVEL_MQTT_FATAL:
            prefix = "MQTT_FATAL ";
            break;
        default:
            prefix = "U ";
            break;
    }

    tempStr = CombineStrings(2, prefix, _Format);
    if (logLevel < EN_LOG_LEVEL_MQTT_MAXIMUM || logLevel > EN_LOG_LEVEL_MQTT_FATAL) {
        format = PrependLocalTimeWithMs(tempStr);
        MemFree(&tempStr);
    } else {
        format = tempStr;
    }

#if LOG_FILE_ENABLE
    if (g_fileSign > 0) {
        g_fileSign = FileWrite(format, args);
        va_start(args, _Format);
    }
#endif

    if (PrintfLogCb) {
        PrintfLogCb(logLevel, format == NULL ? _Format : format, args);
    }
    MemFree(&format);

    va_end(args);
}
