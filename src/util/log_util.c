/* Copyright (c) <2020>, <Huawei Technologies Co., Ltd>
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
 *  */

#include "log_util.h"
#include "string_util.h"
#include <stdio.h>

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

void PrintfLog(int logLevel, char *_Format, ...)
{
    if (_Format == NULL) {
        return;
    }
    va_list args;
    va_start(args, _Format);

    char *format = NULL;
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
        default:
            prefix = "U ";
            break;
    }

    format = CombineStrings(2, prefix, _Format);

    if (PrintfLogCb) {
        PrintfLogCb(logLevel, format, args);
    }
    MemFree(&format);

    va_end(args);
}
