/*
 * Copyright (c) 2024-2025 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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
#ifndef HW_REPORT_ANOMALY_H
#define HW_REPORT_ANOMALY_H

#if defined (SECURITY_AWARENESS_ENABLE)
#include "hw_type.h"

typedef struct {
    char *name;
    char *desc;
} ANOMALY_UNSAFE_FUNCTION;

typedef struct {
    char *protocol;
    char *version;
    char *desc;
} ANOMALY_PROTOCOL;

typedef struct {
    char *cve_number;
    char *vulnerability_name;
    char *desc;
} ANOMALY_CVE_VNLNERABILITY;

typedef struct {
    char *file;
    char *desc;
} ANOMALY_MALICIOUS_FILE;

typedef struct {
    char *pid;
    char *cmd;
    char *desc;
} ANOMALY_ABNORMAL_PROCES;

/**
 * @brief 恶意文件检测上报数据
 * 
 * @param maliciousFile: 恶意文件异常上报数据
 * @param contentNum: 上报个数
 * 
 * @return int 0:success, otherwise failed
 */
int IOTA_AnomalyFileReport(ANOMALY_MALICIOUS_FILE maliciousFile[], int contentNum);

/**
 * @brief CVE漏洞库检测上报数据
 * 
 * @param cveLoophole: CVE漏洞库检测上报数据
 * @param contentNum: 上报个数
 * 
 * @return int 0:success, otherwise failed
 */
int IOTA_AnomalyCveValnerabilityReport(ANOMALY_CVE_VNLNERABILITY cveLoophole[], int contentNum);

/**
 * @brief 不安全协议检测上报数据
 * 
 * @param unsafeProtocol: 不安全协议检测上报数据
 * @param contentNum: 上报个数
 * 
 * @return int 0:success, otherwise failed
 */
int IOTA_AnomalyProtocolReport(ANOMALY_PROTOCOL unsafeProtocol[], int contentNum);

/**
 * @brief 不安全功能检测上报数据
 * 
 * @param unsafeFunction: 不安全功能检测上报数据
 * @param contentNum: 上报个数
 * 
 * @return int 0:success, otherwise failed
 */
int IOTA_AnomalyUnsafeFunctionReport(ANOMALY_UNSAFE_FUNCTION unsafeFunction[], int contentNum);

/**
 * @brief 进程异常检测上报数据
 * 
 * @param abnormalProcess: 进程异常检测上报数据
 * @param contentNum: 上报个数
 * 
 * @return int 0:success, otherwise failed
 */
int IOTA_AnomalyAbnormalProcessReport(ANOMALY_ABNORMAL_PROCES abnormalProcess[], int contentNum);

#endif /* REPORT_ANOMALY */
#endif /* HW_REPORT_ANOMALY_H */