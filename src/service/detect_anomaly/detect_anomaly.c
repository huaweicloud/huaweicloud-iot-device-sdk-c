/*
 * Copyright (c) 2022-2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <dlfcn.h>
#include <unistd.h>
#include <utmpx.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <link.h>
#include <linux/limits.h>
#include "hw_type.h"
#include "iota_datatrans.h"
#include "iota_error_type.h"
#include "data_trans.h"
#include "log_util.h"
#include "string_util.h"
#include "securec.h"
#include "detect_anomaly.h"

#define SWITCH_ON 1
#define INT_TO_PERCENT 100
#define SERVICE_NUM 1

#define AUTH_LOG_FILE "/var/log/auth.log"
#define AUTH_LOG_MAX_LINE_LEN 256
// if anybody tried to login 5 times in 10s
#define LOGIN_BRUTE_FORCE_LOGIN_CNT 5
#define LOGIN_BRUTE_FORCE_LOGIN_INTERVAL 10
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_PER_LOOP 4
#define EVENT_BUF_LEN (EVENT_PER_LOOP * EVENT_SIZE)
// include null terminator
#define NET_INFO_IP_STR_MAX_LEN 256
// include  null terminator
#define IP_STR_MAX_LEN 40

typedef bool (*InotifyEventCallback)(const struct inotify_event *, va_list args);

typedef void (*ListDirCallback)(const char *, void *);

// you can add dir you want to scan
static const char *const g_defaultProtectedPaths[] = {
    // library paths
    // "/lib",
    // "/usr/lib",

    // binary paths
    // "/bin",
    // "/usr/bin",

    NULL,
};

typedef struct DetectLoginLocalArgs {
    off_t fileOffset;
} DetectLoginLocalArgs;

typedef struct DetectFileTamperArgs {
    int monitorFd;
} DetectFileTamperArgs;

typedef struct DetectLoginBruteForceCheckArgs {
    struct timeval loginFailedTimeRecords[LOGIN_BRUTE_FORCE_LOGIN_CNT];
    size_t oldestRecordIndex;
    off_t fileOffset;
} DetectLoginBruteForceCheckArgs;

static HW_BOOL g_reportMemoryTaskRunning = HW_FALSE;
static pthread_t g_reportMemoryTaskId;
static SecurityDetection g_securityDetection;

static DetectLoginLocalArgs g_detectloginLocalArgs;
static DetectFileTamperArgs g_fileTamperCheckArgs;
static DetectLoginBruteForceCheckArgs g_detectLoginBruteForceCheckArgs;

int Detect_GetShadowDetectAnomaly(void)
{
    return IOTA_GetDeviceShadow(SECURITY_DETECTION_CONFIG_REQUEST_ID, NULL, SECURITY_DETECTION_CONFIG, NULL);
}

static void Detect_ReportModuleInfo(const char *reportType, cJSON *content)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();
    cJSON *serviceEvent = cJSON_CreateObject();
    char *eventTimesStamp = GetEventTimesStamp();

    cJSON_AddStringToObject(serviceEvent, SERVICE_ID, LOG);
    cJSON_AddStringToObject(serviceEvent, EVENT_TYPE, SECURITY_LOG_REPORT);
    cJSON_AddStringToObject(serviceEvent, EVENT_TIME, eventTimesStamp);

    cJSON *paras = cJSON_CreateObject();
    cJSON_AddStringToObject(paras, TIMESTAMP, eventTimesStamp);
    cJSON_AddStringToObject(paras, TYPE, reportType);

    cJSON_AddItemReferenceToObject(paras, CONTENT, content);
    cJSON_AddItemToObject(serviceEvent, PARAS, paras);
    cJSON_AddItemToArray(services, serviceEvent);
    cJSON_AddItemToObject(root, SERVICES, services);

    char *payload = cJSON_Print(root);
    cJSON_Delete(root);
    MemFree(&eventTimesStamp);

    if (payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Detect_ReportModuleInfo with payload = %s failed\n", payload);
    } else {
        (void)EventUp(payload, NULL);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Detect_ReportModuleInfo with payload = %s\n", payload);
        MemFree(&payload);
    }
}

static void Detect_ReportMemoryInfo(cJSON *content)
{
    int isLeakAlarm = 0;
    MemoryInfo memoryInfo;
    memoryInfo.total = SysHalGetTotalMemory();
    memoryInfo.used = SysHalGetMemoryUsed();

    if (memoryInfo.used >= g_securityDetection.memoryThreshold * memoryInfo.total / INT_TO_PERCENT) {
        isLeakAlarm = 1;
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the memory usage exceeds threshold\n");
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the memory is enough, memoryUsed = %ld\n", memoryInfo.used);
    }
    memoryInfo.leakAlarm = isLeakAlarm;
    cJSON_AddNumberToObject(content, USED, memoryInfo.used);
    cJSON_AddNumberToObject(content, TOTAL, memoryInfo.total);
    cJSON_AddNumberToObject(content, LEAK_ALARM, memoryInfo.leakAlarm);
}

static void Detect_ReportPortInfo(cJSON *content)
{
    cJSON *used = NULL;
    ArrayInfo *arrayInfo = SysHalGetPortUsed();
    if (arrayInfo != NULL) {
        used = cJSON_CreateIntArray(arrayInfo->array, arrayInfo->arrayLen);
    }
    cJSON_AddItemToObject(content, USED, used);
    if (arrayInfo != NULL) {
        MemFree(&arrayInfo->array);   
        MemFree(&arrayInfo);
    }
}

static void Detect_ReportCpuUsageInfo(cJSON *content)
{
    int cpuAlarm = 0;
    CpuUsageInfo cpuUsageInfo;
    cpuUsageInfo.cpuUsage = SysHalGetCpuUsage();
    if (cpuUsageInfo.cpuUsage >= g_securityDetection.cpuUsageThreshold) {
        cpuAlarm = 1;
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the cpu usage exceeds threshold\n");
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the cpu is enough, cpuUsage = %ld\n", cpuUsageInfo.cpuUsage);
    }
    cpuUsageInfo.cpuAlarm = cpuAlarm;

    cJSON_AddNumberToObject(content, CPU_USAGE, cpuUsageInfo.cpuUsage);
    cJSON_AddNumberToObject(content, CPU_USAGE_ALARM, cpuUsageInfo.cpuAlarm);
}

static void Detect_ReportDiskSpaceInfo(cJSON *content)
{
    int diskAlarm = 0;
    DiskSpaceInfo diskSpaceInfo;
    diskSpaceInfo.diskSpaceTotal = SysHalGetTotalDiskSpace();
    diskSpaceInfo.diskSpaceUsed = SysHalGetDiskSpaceUsed();
    if (diskSpaceInfo.diskSpaceUsed >=
        g_securityDetection.diskSpaceThreshold * diskSpaceInfo.diskSpaceTotal / INT_TO_PERCENT) {
        diskAlarm = 1;
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the disk space used exceeds threshold\n");
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the disk space is enough, diskSpaceUsed = %ld\n", diskSpaceInfo.diskSpaceUsed);
    }
    diskSpaceInfo.diskSpaceAlarm = diskAlarm;

    cJSON_AddNumberToObject(content, DISK_SPACE_USED, diskSpaceInfo.diskSpaceUsed);
    cJSON_AddNumberToObject(content, DISK_SPACE_TOTAL, diskSpaceInfo.diskSpaceTotal);
    cJSON_AddNumberToObject(content, DISK_SPACE_ALARM, diskSpaceInfo.diskSpaceAlarm);
}

static void Detect_ReportBatteryInfo(cJSON *content)
{
    int batteryAlarm = 0;
    BatteryInfo batteryInfo;
    batteryInfo.batteryPct = SysHalGetBatteryPercentage();
    if (batteryInfo.batteryPct <= g_securityDetection.batteryPercentageThreshold) {
        batteryAlarm = 1;
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the battery is low \n");
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "the battery is enough, batteryPct = %ld\n", batteryInfo.batteryPct);
    }
    batteryInfo.batteryAlarm = batteryAlarm;

    cJSON_AddNumberToObject(content, BATTERY_PERCENTAGE, batteryInfo.batteryPct);
    cJSON_AddNumberToObject(content, BATTERY_PERCENTAGE_ALARM, batteryInfo.batteryAlarm);
}

static void ListDir(char *path, ListDirCallback callback, void *arg)
{
    callback(path, arg);
    DIR *dirObj = opendir(path);
    struct dirent *dirInfo = NULL;
    if (dirObj == NULL) {
        return;
    }
    while ((dirInfo = readdir(dirObj)) != NULL) {
        if ((strcmp(dirInfo->d_name, ".") == 0) || (strcmp(dirInfo->d_name, "..") == 0) ||
            (dirInfo->d_type != DT_DIR)) {
            continue;
        }
        size_t len = strlen(path);
        if ((strcat_s(path, PATH_MAX, "/") == EOK) && (strcat_s(path, PATH_MAX, dirInfo->d_name)) == EOK) {
            ListDir(path, callback, arg);
        }

        path[len] = '\0';
    }
    closedir(dirObj);
}

static bool InotifyReadEventsAndIterate(int monitorFd, InotifyEventCallback eventCallback, ...)
{
    ssize_t i = 0;
    va_list vaArgList;
    char buf[EVENT_BUF_LEN];
    bool ret = true;

    va_start(vaArgList, eventCallback);
    while (true) {
        ssize_t length = read(monitorFd, buf, EVENT_BUF_LEN);
        if (length < 0 && errno == EAGAIN) {
            PrintfLog(EN_LOG_LEVEL_INFO, "cannot read inotify events, try again later\n");
            ret = false;
            goto EXIT;
        }

        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buf[i];
            if (!eventCallback(event, vaArgList)) {
                ret = false;
                goto EXIT;
            }
            i += (ssize_t)(EVENT_SIZE + event->len);
        }
    }
EXIT:
    va_end(vaArgList);
    return ret;
}

/* *************************************************
 * local login
 * ************************************************ */
static bool LocalLoginCheckUtmpxItem(struct utmpx *ut)
{
    bool localLoginFlag = false;
    struct in6_addr inetAddr;

    if ((ut->ut_type == USER_PROCESS) && (inet_pton(AF_INET, ut->ut_host, &inetAddr) == 0) &&
        (inet_pton(AF_INET6, ut->ut_host, &inetAddr) == 0)) {
        char *tmbuf = Timeval2Str(&ut->ut_tv);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "local login as user \"%s\", line \"%s\", at %s\n", ut->ut_user, ut->ut_line,
            tmbuf);
        MemFree(&tmbuf);
    }
    return localLoginFlag;
}

static bool LocalLoginCheckProcessLog(off_t *startOffset, cJSON *ret)
{
    FILE *f;
    struct stat st;
    struct utmpx utmpxItem;
    off_t currentFileSize;
    bool localLoginFlag = false;

    if (!(f = fopen(UTMPX_FILE, "r"))) {
        PrintfLog(EN_LOG_LEVEL_WARNING, "couldn't open %s\n", UTMPX_FILE);
        return false;
    }

    if (fstat(fileno(f), &st) == -1) {
        PrintfLog(EN_LOG_LEVEL_WARNING, "couldn't stat %s\n", UTMPX_FILE);
        goto exit;
    }

    if (st.st_size == *startOffset) {
        goto exit;
    }

    if (fseek(f, *startOffset, SEEK_SET) != (off_t)-1) {
        while (fread(&utmpxItem, sizeof(utmpxItem), 1, f) == 1) {
            localLoginFlag |= LocalLoginCheckUtmpxItem(&utmpxItem);
        }
    }

    currentFileSize = ftello(f);
    *startOffset = (currentFileSize != -1 && currentFileSize != *startOffset) ? currentFileSize : st.st_size;

exit:
    cJSON_AddNumberToObject(ret, "login_local_alarm", localLoginFlag);
    (void)fclose(f);
    return true;
}

static bool LocalLoginCheckInit(void)
{
    FILE *wtmpFile = fopen(WTMPX_FILE, "r");
    if (wtmpFile != NULL) {
        (void)fseek(wtmpFile, 0, SEEK_END);
        g_detectloginLocalArgs.fileOffset = ftello(wtmpFile);
        fclose(wtmpFile);
    }
    return true;
}

static void LocalLoginCheckGetReport(cJSON *content)
{
    LocalLoginCheckProcessLog(&g_detectloginLocalArgs.fileOffset, content);
}

/* *************************************************
 * file tamper
 * ************************************************ */
static bool FileTamperProcessINotifyEvent(const struct inotify_event *event, va_list args)
{
    const char *name = event->name;
    cJSON *alarm;
    alarm = va_arg(args, cJSON *);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "------------\n");

    PrintfLog(EN_LOG_LEVEL_DEBUG, "name: %s\n", name);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "mask: %d\n", event->mask);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "cookie: %d\n", event->cookie);
    if (event->mask & IN_OPEN) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Open event on %s\n", name);
    }
    if (event->mask & IN_CLOSE_WRITE) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Close write event on %s\n", name);
    }
    if (event->mask & IN_ATTRIB) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Metadata changed on %s\n", name);
    }
    if (event->mask & IN_ACCESS) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "File accessed %s\n", name);
    }
    if (event->mask & IN_MODIFY) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "File modified %s\n", name);
    }
    if (event->mask & IN_CLOSE_NOWRITE) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Unwritable file closed %s\n", name);
    }
    if (event->mask & IN_MOVED_FROM) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "File moved from %s\n", name);
    }
    if (event->mask & IN_MOVED_TO) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "File moved to %s\n", name);
    }
    if (event->mask & IN_CREATE) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Subfile created %s\n", name);
    }
    if (event->mask & IN_DELETE) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Subfile deleted %s\n", name);
    }
    if (event->mask & IN_DELETE_SELF) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Self deleted %s\n", name);
    }
    if (event->mask & IN_MOVE_SELF) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "Self moved %s\n", name);
    }
    cJSON_SetIntValue(alarm, 1);
    return true;
}

static int WatchForFileTamper(int monitorFd, const char *filePath)
{
    PrintfLog(EN_LOG_LEVEL_DEBUG, "watch %s ", filePath);
    int wd = inotify_add_watch(monitorFd, filePath,
        IN_DELETE | IN_CREATE | IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVE);
    if (wd == -1) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "failed\n");
    } else {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "success\n");
    }
    return wd;
}

static void FileTamperWatchListedDir(const char *filePath, void *arg)
{
    int monitorFd = *((int *)arg);
    WatchForFileTamper(monitorFd, filePath);
}

static int FileTamperWatchLibs(struct dl_phdr_info *info, size_t size, void *arg)
{
    (void)size;
    const char *dlpiName = info->dlpi_name;
    int monitorFd = *((int *)arg);
    if ((strstr(info->dlpi_name, "linux-vdso.so.1")) || (strlen(info->dlpi_name) == 0)) {
        return 0;
    }
    PrintfLog(EN_LOG_LEVEL_DEBUG, "Library  %s\n", dlpiName);
    WatchForFileTamper(monitorFd, dlpiName);
    return 0;
}

static bool FileTamperCheckInit(void)
{
    int monitorFd = inotify_init();
    g_fileTamperCheckArgs.monitorFd = monitorFd;

    if (fcntl(monitorFd, F_SETFL, fcntl(monitorFd, F_GETFL) | O_NONBLOCK)) {
        PrintfLog(EN_LOG_LEVEL_WARNING, "fcntl error\n");
        goto EXIT;
    }

    dl_iterate_phdr(FileTamperWatchLibs, &monitorFd);
    int p;
    for (p = 0; g_defaultProtectedPaths[p] != NULL; ++p) {
        char path[PATH_MAX];
        if (strcpy_s(path, PATH_MAX, g_defaultProtectedPaths[p]) != EOK) {
            continue;
        }

        ListDir(path, FileTamperWatchListedDir, &monitorFd);
    }
    return true;
EXIT:
    close(monitorFd);
    return NULL;
}

static void FileTamperCheckGetReport(cJSON *content)
{
    cJSON *alarm = cJSON_AddNumberToObject(content, "file_tamper_alarm", 0);
    InotifyReadEventsAndIterate(g_fileTamperCheckArgs.monitorFd, FileTamperProcessINotifyEvent, alarm);
}

static void FileTamperCheckDestroy(void)
{
    close(g_fileTamperCheckArgs.monitorFd);
}

/* *************************************************
    login brute force
************************************************* */

static bool LoginBruteForceProcessCheckCriticalFreq(size_t oldestRecordIndex, struct timeval *loginFailedTimeRecords)
{
    size_t lastRecordIndex = 0;
    bool ret = false;
    if (oldestRecordIndex == 0) {
        lastRecordIndex = LOGIN_BRUTE_FORCE_LOGIN_CNT - 1;
    } else {
        lastRecordIndex = (oldestRecordIndex - 1) % LOGIN_BRUTE_FORCE_LOGIN_CNT;
    }
    struct timeval *oldestRecord = loginFailedTimeRecords + oldestRecordIndex;
    struct timeval *lastRecord = loginFailedTimeRecords + lastRecordIndex;
    double oldestRecordTime = (double)oldestRecord->tv_sec + (double)oldestRecord->tv_usec / (1000 * 1000);
    double lastRecordTime = (double)lastRecord->tv_sec + (double)lastRecord->tv_usec / (1000 * 1000);
    PrintfLog(EN_LOG_LEVEL_DEBUG, "%lu %lu, %lf %lf\n", lastRecordIndex, oldestRecordIndex, lastRecordTime,
        oldestRecordTime);
    if (lastRecordTime - oldestRecordTime < LOGIN_BRUTE_FORCE_LOGIN_INTERVAL) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "brute force login detected\n");
        ret = true;
    }
    PrintfLog(EN_LOG_LEVEL_DEBUG, "login time records:");
    size_t i;
    for (i = 0; i < LOGIN_BRUTE_FORCE_LOGIN_CNT; ++i) {
        struct timeval *tv = loginFailedTimeRecords + (oldestRecordIndex + i) % LOGIN_BRUTE_FORCE_LOGIN_CNT;
        char *tmbuf = Timeval2Str(tv);
        PrintfLog(EN_LOG_LEVEL_DEBUG, "%s\t", tmbuf);
        MemFree(&tmbuf);
    }
    putchar('\n');
    return ret;
}

static bool LoginBruteForceCheckInit(void)
{
    g_detectLoginBruteForceCheckArgs.oldestRecordIndex = 0;

    FILE *authLogFile = fopen(AUTH_LOG_FILE, "r");
    if (authLogFile == NULL) {
        PrintfLog(EN_LOG_LEVEL_WARNING, "couldn't open auth log file\n");
        goto EXIT;
    }
    (void)fseek(authLogFile, 0, SEEK_END);
    g_detectLoginBruteForceCheckArgs.fileOffset = ftello(authLogFile);
    (void)fclose(authLogFile);

    int i;
    for (i = 0; i < LOGIN_BRUTE_FORCE_LOGIN_CNT; ++i) {
        g_detectLoginBruteForceCheckArgs.loginFailedTimeRecords[i].tv_sec = 0;
        g_detectLoginBruteForceCheckArgs.loginFailedTimeRecords[i].tv_usec = 0;
    }
    return true;
EXIT:
    return false;
}

static void LoginBruteForceReport(cJSON *content)
{
    off_t *startOffset = &g_detectLoginBruteForceCheckArgs.fileOffset;
    struct timeval *loginFailedTimeRecords = g_detectLoginBruteForceCheckArgs.loginFailedTimeRecords;
    size_t *oldestRecordIndex = &g_detectLoginBruteForceCheckArgs.oldestRecordIndex;
    FILE *f;
    char logLine[AUTH_LOG_MAX_LINE_LEN];
    struct stat st;
    off_t currentFileSize;
    bool hasBruteForceLogin = false;

    if (!(f = fopen(AUTH_LOG_FILE, "r"))) {
        PrintfLog(EN_LOG_LEVEL_WARNING, "couldn't open %s", UTMPX_FILE);
        goto exit;
    }

    if (fstat(fileno(f), &st) == -1) {
        PrintfLog(EN_LOG_LEVEL_WARNING, "couldn't stat %s", UTMPX_FILE);
        goto exit;
    }

    if (st.st_size == *startOffset) {
        goto exit;
    }

    if (fseek(f, *startOffset, SEEK_SET) != (off_t)-1) {
        char *c = NULL;
        while ((c = fgets(logLine, AUTH_LOG_MAX_LINE_LEN, f)) != NULL) {
            int msgCount = 1;
            if (strstr(logLine, "authentication failure") == NULL) {
                continue;
            }
            if (strstr(logLine, "sudo") != NULL) {
                continue; // ignore sudo
            }
            // repeated message?
            const char *repeatedTimeMessage = strstr(logLine, "message repeated ");
            if ((repeatedTimeMessage != NULL) &&
                (sscanf_s(repeatedTimeMessage, "message repeated %d times", &msgCount) > 0)) {
                PrintfLog(EN_LOG_LEVEL_DEBUG, "can't get message repeated %d times", &msgCount);
            }
            struct timeval tv;

            gettimeofday(&tv, NULL);
            int i;
            for (i = 0; i < msgCount; i++) {
                loginFailedTimeRecords[(*oldestRecordIndex)++] = tv;
                *oldestRecordIndex %= LOGIN_BRUTE_FORCE_LOGIN_CNT;
            }
            PrintfLog(EN_LOG_LEVEL_DEBUG, "%s", logLine);
            hasBruteForceLogin |= LoginBruteForceProcessCheckCriticalFreq(*oldestRecordIndex, loginFailedTimeRecords);
        }
    }

    currentFileSize = ftello(f);
    *startOffset = (currentFileSize != -1 && currentFileSize != *startOffset) ? currentFileSize : st.st_size;

exit:
    if (f != NULL) {
        (void)fclose(f);
    }
    cJSON_AddNumberToObject(content, "login_brute_force_alarm", hasBruteForceLogin);
}

/* *************************************************
    malicous IPs
************************************************* */
// for ip only, only supports up to 2 bytes
static uint16_t Bytes2Int(const char *str, int len)
{
    uint16_t ret = 0;
    int i;
    for (i = 0; i < len; ++i) {
        ret = ret * 16 + str[i] - '0';
    }
    return ret;
}

static bool NetWorkInfoLineToIPString(const char *lineIn, bool isIpv6, char *ip, int bufferLen)
{
    int offset = 0;
    int ret = 0;
    uint32_t ipDigit = 0; // 32bits to accommodate both ipv4 and ipv6
    bool notAllZero = false;
    int ipDigitCnt = isIpv6 ? 8 : 4;
    int ipNibbleCnt = isIpv6 ? 4 : 2;
    char ipDigitSplitter = isIpv6 ? ':' : '.';

    /*
    sl local_address rem_address
    1069: 00000000:14E9 00000000:0000
    */
    const char *line = lineIn;
    line = strstr(line, ":") + 1; // sl
    line = strstr(line, ":") + 1; // local_address
    line = strstr(line, " ") + 1; // rem_address

    int i;
    for (i = 0; i < ipDigitCnt; ++i) {
        ipDigit = Bytes2Int(line, ipNibbleCnt);
        notAllZero = notAllZero || (ipDigit != 0);
        ret = sprintf_s(ip + offset, bufferLen - offset, "%u%c", ipDigit, ipDigitSplitter);
        if (ret == -1) {
            PrintfLog(EN_LOG_LEVEL_WARNING, "can't parse ip address\n");
            return false;
        }
        offset += ret;
    }
    ip[offset - 1] = '\0';
    return notAllZero;
}

static void MaliciousIPCheckGetReport(cJSON *content)
{
    static const char *infoFiles[] = {
        "/proc/net/tcp",
        "/proc/net/tcp6",
        "/proc/net/udp",
        "/proc/net/udp6",
    };
    char ipStr[IP_STR_MAX_LEN]; // 39 bytes to accommodate both ipv4 and ipv6
    char line[NET_INFO_IP_STR_MAX_LEN];
    cJSON *IpArray = cJSON_AddArrayToObject(content, "used_ips");
    cJSON *oneIp = NULL;
    bool duplicatedNewIp = false;

    size_t i;
    for (i = 0; i < sizeof(infoFiles) / sizeof(infoFiles[0]); i++) {
        FILE *f = fopen(infoFiles[i], "r");
        if (f == NULL) {
            PrintfLog(EN_LOG_LEVEL_WARNING, "can't get tcp info\n");
            continue;
        }
        int lineNo = 0;
        while (true) {
            char *c = fgets(line, NET_INFO_IP_STR_MAX_LEN, f);
            if (c == NULL) {
                break;
            }
            // skip first line
            if (lineNo++ == 0) {
                continue;
            }

            bool isIpv6 = (strstr(infoFiles[i], "6") != NULL);
            if (!NetWorkInfoLineToIPString(line, isIpv6, ipStr, IP_STR_MAX_LEN)) {
                continue;
            }
            duplicatedNewIp = false;
            cJSON_ArrayForEach(oneIp, IpArray)
            {
                if (strcmp(cJSON_GetStringValue(oneIp), ipStr) == 0) {
                    duplicatedNewIp = true;
                    break;
                }
            }
            if (duplicatedNewIp == true) {
                continue;
            }
            cJSON_AddItemToArray(IpArray, cJSON_CreateString(ipStr));
        }
        (void)fclose(f);
    }
}

typedef struct SituationAwarenessTasks {
    bool check;
    bool newCheck;
    const char *checkName;
    const char *reportType;
    // for getting info
    bool (*init)(void);
    void (*getReport)(cJSON *content);
    void (*destroy)(void);
} SituationAwarenessTasks;

static SituationAwarenessTasks g_mTasks[] = {
    {
        .checkName = MEMORY_CHECK,
        .reportType = MEMORY_REPORT,
        .init = NULL,
        .getReport = &Detect_ReportMemoryInfo,
        .destroy = NULL,
    },
    {
        .checkName = PORT_CHECK,
        .reportType = PORT_REPORT,
        .init = NULL,
        .getReport = &Detect_ReportPortInfo,
        .destroy = NULL,
    },
    {
        .checkName = CPU_USAGE_CHECK,
        .reportType = CPU_USAGE_REPORT,
        .init = NULL,
        .getReport = &Detect_ReportCpuUsageInfo,
        .destroy = NULL,
    },
    {
        .checkName = DISK_SPACE_CHECK,
        .reportType = DISK_SPACE_REPORT,
        .init = NULL,
        .getReport = &Detect_ReportDiskSpaceInfo,
        .destroy = NULL,
    },
    {
        .checkName = BATTERY_PERCENTAGE_CHECK,
        .reportType = BATTERY_REPORT,
        .init = NULL,
        .getReport = &Detect_ReportBatteryInfo,
        .destroy = NULL,
    },
    {
        .checkName = LOGIN_LOCAL_CHECK,
        .reportType = LOGIN_LOCAL_REPORT,
        .init = &LocalLoginCheckInit,
        .getReport = LocalLoginCheckGetReport,
        .destroy = NULL
    },
    {
        .checkName = FILE_TAMPER_CHECK,
        .reportType = FILE_TAMPER_REPORT,
        .init = &FileTamperCheckInit,
        .getReport = FileTamperCheckGetReport,
        .destroy = FileTamperCheckDestroy
    },
    {
        .checkName = LOGIN_BRUTE_FORCE_CHECK,
        .reportType = LOGIN_BRUTE_FORCE_REPORT,
        .init = &LoginBruteForceCheckInit,
        .getReport = LoginBruteForceReport,
        .destroy = NULL
    },
    {
        .checkName = MALICIOUS_IP_CHECK,
        .reportType = IP_REPORT,
        .init = NULL,
        .getReport = MaliciousIPCheckGetReport,
        .destroy = NULL
    },
};

static void *Detect_ReportDetectionInfoEntry(void *args)
{
    while (1) {
        size_t i;
        for (i = 0; i < sizeof(g_mTasks) / sizeof(g_mTasks[0]); i++) {
            if (g_mTasks[i].check && g_mTasks[i].getReport) {
                cJSON *content = cJSON_CreateObject();
                g_mTasks[i].getReport(content);
                Detect_ReportModuleInfo(g_mTasks[i].reportType, content);
                cJSON_Delete(content);
            }
        }
        usleep(DETECT_REPORT_FREQUENCY * 1000 * 1000);
    }
    return args; // for pthread interface
}

static void Detect_ReportDetectionInfo(void)
{
    int securityConfig = 0;

    size_t i;
    for (i = 0; i < sizeof(g_mTasks) / sizeof(g_mTasks[0]); i++) {
        securityConfig = securityConfig || g_mTasks[i].newCheck;
        if (g_mTasks[i].newCheck && !g_mTasks[i].check && g_mTasks[i].init) {
            g_mTasks[i].init();
        }
        if (!g_mTasks[i].newCheck && g_mTasks[i].check && g_mTasks[i].destroy) {
            g_mTasks[i].destroy();
        }
        g_mTasks[i].check = g_mTasks[i].newCheck;
    }

    if ((securityConfig == SWITCH_ON) && !g_reportMemoryTaskRunning) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "start dectection thread\n");
        pthread_create(&g_reportMemoryTaskId, NULL, Detect_ReportDetectionInfoEntry, NULL);
        g_reportMemoryTaskRunning = HW_TRUE;
    } else if ((securityConfig != SWITCH_ON) && g_reportMemoryTaskRunning) {
        PrintfLog(EN_LOG_LEVEL_DEBUG, "stop dectection thread\n");
        (void)pthread_cancel(g_reportMemoryTaskId);
        pthread_join(g_reportMemoryTaskId, NULL);
        g_reportMemoryTaskRunning = HW_FALSE;
    }
}

static void Detect_ReportShadowDesired(SecurityDetection securityDetection)
{
    cJSON *properties = cJSON_CreateObject();
    ST_IOTA_SERVICE_DATA_INFO services[1];

    cJSON_AddNumberToObject(properties, MEMORY_THRESHOLD, securityDetection.memoryThreshold);
    cJSON_AddNumberToObject(properties, CPU_USAGE_THRESHOLD, securityDetection.cpuUsageThreshold);
    cJSON_AddNumberToObject(properties, DISK_SPACE_THRESHOLD, securityDetection.diskSpaceThreshold);
    cJSON_AddNumberToObject(properties, BATTERY_PERCENTAGE_THRESHOLD, securityDetection.batteryPercentageThreshold);
    size_t i;
    for (i = 0; i < sizeof(g_mTasks) / sizeof(g_mTasks[0]); i++) {
        cJSON_AddNumberToObject(properties, g_mTasks[i].checkName, g_mTasks[i].newCheck);
    }
    char *service = cJSON_Print(properties);
    cJSON_Delete(properties);

    services[0].event_time = NULL;
    services[0].service_id = SECURITY_DETECTION_CONFIG;
    services[0].properties = service;
    int messageId = IOTA_PropertiesReport(services, SERVICE_NUM, 0, NULL);
    if (messageId != 0) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "Detect_ReportShadowDesired: IoTA_PropertiesReport() failed, messageId %d\n",
            messageId);
    }
    MemFree(&service);
}

void Detect_ParseShadowGetOrPropertiesSet(char *propertiesDown)
{
    cJSON *properties = cJSON_Parse(propertiesDown);
    if (properties == NULL) {
        return;
    }

    cJSON *memoryThreshold = cJSON_GetObjectItem(properties, MEMORY_THRESHOLD);
    if (memoryThreshold != NULL) {
        g_securityDetection.memoryThreshold = (int)cJSON_GetNumberValue(memoryThreshold);
    }

    cJSON *cpuUsageThreshold = cJSON_GetObjectItem(properties, CPU_USAGE_THRESHOLD);
    if (cpuUsageThreshold != NULL) {
        g_securityDetection.cpuUsageThreshold = (int)cJSON_GetNumberValue(cpuUsageThreshold);
    }

    cJSON *diskSpaceThreshold = cJSON_GetObjectItem(properties, DISK_SPACE_THRESHOLD);
    if (diskSpaceThreshold != NULL) {
        g_securityDetection.diskSpaceThreshold = (int)cJSON_GetNumberValue(diskSpaceThreshold);
    }

    cJSON *batteryPctThreshold = cJSON_GetObjectItem(properties, BATTERY_PERCENTAGE_THRESHOLD);
    if (batteryPctThreshold != NULL) {
        g_securityDetection.batteryPercentageThreshold = (int)cJSON_GetNumberValue(batteryPctThreshold);
    }

    size_t i;
    for (i = 0; i < sizeof(g_mTasks) / sizeof(g_mTasks[0]); i++) {
        cJSON *check = cJSON_GetObjectItem(properties, g_mTasks[i].checkName);
        if (check != NULL) {
            g_mTasks[i].newCheck = (int)cJSON_GetNumberValue(check);
        }
    }

    Detect_ReportDetectionInfo();
    Detect_ReportShadowDesired(g_securityDetection);

    cJSON_Delete(properties);
    return;
}