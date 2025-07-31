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

#if defined (SECURITY_AWARENESS_ENABLE)

#include "report_anomaly.h"
#include "data_trans.h"
#include "iota_error_type.h"
#include "iota_datatrans.h"
#include "log_util.h"
#include "string_util.h"

static int IOTA_ReportAnomaly(const char *reportType, cJSON *content) {

    // 组装josn
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();
    cJSON *serviceEvent = cJSON_CreateObject();
    cJSON *paras = cJSON_CreateObject();
    char *eventTimesStamp = GetEventTimesStamp();
    
    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, LOG);
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, eventTimesStamp);

    cJSON_AddStringToObject(paras, TIMESTAMP, eventTimesStamp);
    cJSON_AddStringToObject(paras, TYPE, reportType);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, SECURITY_LOG_REPORT);

    cJSON_AddItemReferenceToObject(paras, CONTENT, content);
    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    MemFree(&eventTimesStamp);

    int ret = -1;
    if (payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_ReportAnomaly with payload = %s failed\n", payload);
        return -1;
    } else {
        ret = EventUp(payload, NULL);
        MemFree(&payload);
    }
    return ret;
}

int IOTA_AnomalyFileReport(ANOMALY_MALICIOUS_FILE maliciousFile[], int contentNum) {

    if (contentNum < 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_AnomalyFileReport contentNum < 1\n");
        return IOTA_FAILURE;
    }

    cJSON *content = cJSON_CreateObject();
    cJSON *usedDescribes = cJSON_CreateArray();
    for (int i = 0; i < contentNum; i++) {
        cJSON *usedDescribe = cJSON_CreateObject();
        cJSON_AddStringToObject(usedDescribe, "file", maliciousFile[i].file);
        cJSON_AddStringToObject(usedDescribe, DESC, maliciousFile[i].desc);
        cJSON_AddItemToArray(usedDescribes, usedDescribe);
    }
    cJSON_AddItemReferenceToObject(content, MALICIOUS_FILES, usedDescribes);
    int ret = IOTA_ReportAnomaly( FILE_REPORT, content);

    cJSON_Delete(usedDescribes);
    cJSON_Delete(content);
    return ret;
}

int IOTA_AnomalyCveValnerabilityReport(ANOMALY_CVE_VNLNERABILITY cveLoophole[], int contentNum) {
    if (contentNum < 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_AnomalyCveValnerabilityReport contentNum < 1\n");
        return IOTA_FAILURE;
    }

    cJSON *content = cJSON_CreateObject();
    cJSON *usedDescribes = cJSON_CreateArray();
    for (int i = 0; i < contentNum; i++) {
        cJSON *usedDescribe = cJSON_CreateObject();
        cJSON_AddStringToObject(usedDescribe, CVE_NUMBER, cveLoophole[i].cve_number);
        cJSON_AddStringToObject(usedDescribe, VNLNERABILITIES_NAME, cveLoophole[i].vulnerability_name);
        cJSON_AddStringToObject(usedDescribe, DESC, cveLoophole[i].desc);
        cJSON_AddItemToArray(usedDescribes, usedDescribe);
    }
    cJSON_AddItemReferenceToObject(content, VNLNERABILITIES, usedDescribes);
    int ret = IOTA_ReportAnomaly(CVE_VULNERABILITY_REPORT, content);

    cJSON_Delete(usedDescribes);
    cJSON_Delete(content);
    return ret;
}

	
int IOTA_AnomalyProtocolReport(ANOMALY_PROTOCOL unsafeProtocol[], int contentNum) {
    if (contentNum < 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_AnomalyProtocolReport contentNum < 1\n");
        return IOTA_FAILURE;
    }

    cJSON *content = cJSON_CreateObject();
    cJSON *usedDescribes = cJSON_CreateArray();
    for (int i = 0; i < contentNum; i++) {
        cJSON *usedDescribe = cJSON_CreateObject();
        cJSON_AddStringToObject(usedDescribe, PROTOCOL, unsafeProtocol[i].protocol);
        cJSON_AddStringToObject(usedDescribe, VERSION, unsafeProtocol[i].version);
        cJSON_AddStringToObject(usedDescribe, DESC, unsafeProtocol[i].desc);
        cJSON_AddItemToArray(usedDescribes, usedDescribe);
    }
    cJSON_AddItemReferenceToObject(content, USED_PROTOCOLS, usedDescribes);
    int ret = IOTA_ReportAnomaly(PROTOCOL_REPORT, content);

    cJSON_Delete(usedDescribes);
    cJSON_Delete(content);
    return ret;
}

int IOTA_AnomalyUnsafeFunctionReport(ANOMALY_UNSAFE_FUNCTION unsafeFunction[], int contentNum) {
    if (contentNum < 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_AnomalyUnsafeFunctionReport contentNum < 1\n");
        return IOTA_FAILURE;
    }
    cJSON *content = cJSON_CreateObject();
    cJSON *usedDescribes = cJSON_CreateArray();
    for (int i = 0; i < contentNum; i++) {
        cJSON *usedDescribe = cJSON_CreateObject();
        cJSON_AddStringToObject(usedDescribe, NAME, unsafeFunction[i].name);
        cJSON_AddStringToObject(usedDescribe, DESC, unsafeFunction[i].desc);
        cJSON_AddItemToArray(usedDescribes, usedDescribe);
    }
    cJSON_AddItemReferenceToObject(content, UNSAFE_FUNCTIONS, usedDescribes);
    int ret = IOTA_ReportAnomaly(UNSAFE_FUNCTION_REPORT, content);

    cJSON_Delete(usedDescribes);
    cJSON_Delete(content);
    return ret;
}

int IOTA_AnomalyAbnormalProcessReport(ANOMALY_ABNORMAL_PROCES abnormalProcess[], int contentNum) {
    if (contentNum < 1) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "IOTA_ReportAnomalyFileReport contentNum < 1\n");
        return IOTA_FAILURE;
    }
    cJSON *content = cJSON_CreateObject();
    cJSON *usedDescribes = cJSON_CreateArray();
     for (int i = 0; i < contentNum; i++) {
        cJSON *usedDescribe = cJSON_CreateObject();
        cJSON_AddStringToObject(usedDescribe, "pid", abnormalProcess[i].pid);
        cJSON_AddStringToObject(usedDescribe, "cmd", abnormalProcess[i].cmd);
        cJSON_AddStringToObject(usedDescribe, DESC, abnormalProcess[i].desc);
        cJSON_AddItemToArray(usedDescribes, usedDescribe);
    }
    cJSON_AddItemReferenceToObject(content, ABNORMAL_PROCESSES, usedDescribes);
    int ret = IOTA_ReportAnomaly(ABNORMAL_PROCESS_REPORT, content);

    cJSON_Delete(usedDescribes);
    cJSON_Delete(content);
    return ret;
}
#endif