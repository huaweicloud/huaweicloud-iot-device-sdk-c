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

#include "stdio.h"
#include "signal.h"
#include "hw_type.h"
#include "iota_init.h"
#include "iota_cfg.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include "errno.h"
#include "iota_error_type.h"
#include "iota_login.h"
#include "iota_datatrans.h"
#include "cJSON.h"
#include <hw_type.h>
#include <log_util.h>
#include <json_util.h>
#include <string_util.h>
#include "generic_tcp_protocol.h"

char *gIoTPlatformIp    = "1.1.1.1";
int  gIoTPlatformPort   = 8883;
int  gClientSocket      = -1;
char *gUserName         = "xxx";  //deviceId
char *gPassWord         = "xxx";
int  gIoTPlatformStatus = DISCONNECTED;
char *gatewayId         = NULL;

void HandleCommandRequest(void *context, int messageId, int code, char *message, char *requestId);
void HandleEventsDown(void *context, int messageId, int code, char *message);
void HandleConnectSuccess(void *context, int messageId, int code, char *message);
void HandleConnectFailure(void *context, int messageId, int code, char *message);
void HandleConnectionLost(void *context, int messageId, int code, char *message);
void HandleDisconnectSuccess(void *context, int messageId, int code, char *message);
void HandleDisconnectFailure(void *context, int messageId, int code, char *message);
void HandleSubscribesuccess(void *context, int messageId, int code, char *message);
void HandleSubscribeFailure(void *context, int messageId, int code, char *message);
void HandleReportSuccess(void *context, int messageId, int code, char *message);
void HandleReportFailure(void *context, int messageId, int code, char *message);

void MyPrintLog(int level, char *format, va_list args);
void SetAuthConfig(void);
void SetMyCallbacks(void);

//----------------------------handle ack -------------------------------------------
void HandleConnectSuccess(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleConnectSuccess(), connect success\n");
	gIoTPlatformStatus = CONNECTED;
}

void HandleConnectFailure(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleConnectFailure(), messageId %d, code %d, messsage %s\n", messageId, code, message);
    
	//Check the cause of connect failure, and do what you want, Such as retrying
	int ret = IOTA_Connect();
	if (ret != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleConnectFailure(), connect again failed, result %d\n", ret);
    }
}

void HandleConnectionLost(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleConnectionLost() , messageId %d, code %d, messsage %s\n", messageId, code, message);
    gIoTPlatformStatus = DISCONNECTED;
    
	//Check the cause of lost, and do what you want, Such as reconnect 
	int ret = IOTA_Connect();
	if (ret != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleConnectionLost(), connect again failed, result %d\n", ret);
	}
}

void HandleDisconnectSuccess(void *context, int messageId, int code, char *message) {
    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleDisconnectSuccess, messageId %d, code %d, messsage %s\n", messageId, code, message);
    gIoTPlatformStatus = DISCONNECTED;
}

void HandleDisconnectFailure(void *context, int messageId, int code, char *message) {
    PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleDisconnectFailure(), messageId %d, code %d, messsage %s\n", messageId, code, message);

    int ret = IOTA_DisConnect();
    if (ret != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleDisconnectFailure(), disconnect again failed, result %d\n", ret);
	}
}

void HandleReportSuccess(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleReportSuccess() messageId %d\n", messageId);
}

void HandleReportFailure(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: HandleReportFailure() warning, messageId %d, code %d, messsage %s\n", messageId, code, message);
   //Check the cause of connect failure, and do what you want
   //To Do ......
}

//----------------------------------handle command or event arrive-----------------------------
void HandleCommandRequest(void *context, int messageId, int code, char *message, char *requestId) {
	PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleCommandRequest(), messageId %d, code %d, messsage %s, requestId %s\n", messageId, code, message, requestId);

    char *payload = EncodeCommandParas(messageId, code, message, requestId);
    if (payload != NULL) {   
        SendMessageToSubDevice(gClientSocket, payload);
        MemFree(&payload);
    }
    SendCommandRspToIoTPlatform(requestId); 
}

void HandleEventsDown(void *context, int messageId, int code, char *message) {

    PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: HandleEventsDown(), messageId %d, code %d, messsage %s\n", messageId, code, message);
    ProcessEventFromIoTPlatform(messageId, code, message);
}

//------------------------------------------------------------------------------------

void MyPrintLog(int level, char *format, va_list args) {
	vprintf(format, args);
	/*if you want to printf log in system log files,you can do this:
	 *
	 * vsyslog(level, format, args);
	 * */
}

void SetAuthConfig() {
	IOTA_ConfigSetStr(EN_IOTA_CFG_MQTT_ADDR, gIoTPlatformIp);
	IOTA_ConfigSetUint(EN_IOTA_CFG_MQTT_PORT, gIoTPlatformPort);
	IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICEID, gUserName);
	IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICESECRET, gPassWord);
	IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);

#ifdef _SYS_LOG
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LOCAL_NUMBER, LOG_LOCAL7);
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LEVEL, LOG_INFO);
#endif
}

void SetMyCallbacks() {
	IOTA_SetCallback(EN_IOTA_CALLBACK_CONNECT_SUCCESS, HandleConnectSuccess);
	IOTA_SetCallback(EN_IOTA_CALLBACK_CONNECT_FAILURE, HandleConnectFailure);

	IOTA_SetCallback(EN_IOTA_CALLBACK_DISCONNECT_SUCCESS, HandleDisconnectSuccess);
	IOTA_SetCallback(EN_IOTA_CALLBACK_DISCONNECT_FAILURE, HandleDisconnectFailure);
	IOTA_SetCallback(EN_IOTA_CALLBACK_CONNECTION_LOST, HandleConnectionLost);

	IOTA_SetCallback(EN_IOTA_CALLBACK_PUBLISH_SUCCESS, HandleReportSuccess);
	IOTA_SetCallback(EN_IOTA_CALLBACK_PUBLISH_FAILURE, HandleReportFailure);

	IOTA_SetCallbackWithTopic(EN_IOTA_CALLBACK_COMMAND_REQUEST, HandleCommandRequest);
	IOTA_SetCallback(EN_IOTA_CALLBACK_EVENT_DOWN, HandleEventsDown);
}

int main(int argc, char **argv) {
#if defined(_DEBUG)
	setvbuf(stdout, NULL, _IONBF, 0); //in order to make the console log printed immediately at debug mode
#endif

	IOTA_SetPrintLogCallback(MyPrintLog);

	printf("gateway_demo: start test ===================>\n");

	if (IOTA_Init(WORK_PATH) < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "gateway_demo: call IOTA_Init() error, init failed\n");
		return -1;
	}

	SetAuthConfig();
	SetMyCallbacks();

	//Connect to HuaweiCloud IoT service, do what you want in function HandleConnectSuccess
	int ret = IOTA_Connect();
	if (ret != 0) {
		printf("gateway_demo: connect to IoT platform error, result %d\n", ret);
        return -1;
	}

	TimeSleep(1500);

    //Create TCP Service listen
    int server_socket = CreateServerSocket(TCP_SERVER_IP, TCP_SERVER_PORT, BACK_LOG);
    if (server_socket == -1) {
        return -1;
    }

    //Accept client connect, and process the message report from client
    int  count = 0;
    struct sockaddr_in addr_client = { 0 };
    socklen_t addrLen = sizeof(addr_client);
	while (count < 10000) {
		int clientSocket = accept(server_socket, (struct sockaddr*) &addr_client, &addrLen);
		if (clientSocket <= 0) {
			PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: accept client connect error.\n");
			//continue;
		}

        if (gClientSocket != clientSocket) {
            PrintfLog(EN_LOG_LEVEL_INFO, "gateway_demo: get a new client socket=%d\n", clientSocket);
            close(gClientSocket);
            gClientSocket = clientSocket;
        }

        ProcessMessageFromClient(gClientSocket);
		TimeSleep(1500);
		count++;
	}

    close(gClientSocket);
    close(server_socket);
    
	printf("gateway_demo: test has ended ===================>\n");

	return 0;
}


