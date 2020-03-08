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

char *gJsonKeyOfServiceId[4]     = {"parameter", "analog", "discrete", "accumulator"};
char *gJsonKeyOfProperties[4][2] = {{"Load",     "ImbA_strVal"},
                                    {"PhV_phsA", "PhV_phsB"},
                                    {"Ind1",     "Ind2"},
                                    {"SupWh",    "SupVarh"}};

ST_IOTA_SERVICE_DATA_INFO* DecodeServiceProperty(char *payload) {

	//Decode the payload,service1/service2
	if (payload == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, payload is NULL\n");
		return NULL;
	}

    //Demo protocol definition:
    //First byte indicate service type, the value ranges from 1 to 4;  
    //Second byte indicate the value of properties1, the value ranges from 0 to 9;
    //Third byte indicate the value of properties2, the value ranges from 0 to 9;
	unsigned int serviceIndex = 0;
	char serviceId[2], firstPropertie[2], secondPropertie[2];
    serviceId[0] = *payload;
    serviceId[1] = '\0';
    firstPropertie[0] = *(payload+1);
    firstPropertie[1] = '\0';
    secondPropertie[0] = *(payload+2);
    secondPropertie[1] = '\0';
    serviceIndex = atoi(serviceId) - 1;

	if (serviceId >= 4) {
	    PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, service id = d%, is invalid\n", serviceId);
	    return NULL;        
	}

    //Combine the Service id
	char* jsonsServiceId = NULL;
	if (CopyStrValue(&jsonsServiceId, gJsonKeyOfServiceId[serviceIndex-1], StringLength(gJsonKeyOfServiceId[serviceIndex-1])) < 0){
		PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, copy string failed\n");
		return NULL;
	}

    //Combine the event time
	char* eventTime = GetEventTimesStamp();
	if (eventTime == NULL) {
	    PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, get system time failed\n");
        MemFree(&jsonsServiceId);
		return NULL;
	}

    //Combine the properties in Json String, Sample:{"Load":"1","ImbA_strVal":"2"}
	char* serviceProperties = CombineStrings(9, "{\"", gJsonKeyOfProperties[serviceIndex-1][0], "\":\"", firstPropertie, "\",\"",
		                                               gJsonKeyOfProperties[serviceIndex-1][1], "\":\"", secondPropertie, "\"}");
    if (serviceProperties == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, Combine Properties failed\n");
        MemFree(&jsonsServiceId);
        MemFree(&eventTime);
        return NULL;
    }

	ST_IOTA_SERVICE_DATA_INFO* serviceDataInfo = (ST_IOTA_SERVICE_DATA_INFO*)malloc(sizeof(ST_IOTA_SERVICE_DATA_INFO));
	if (serviceDataInfo == NULL) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: DecodeServiceProperty() error, malloc mem failed\n");
        MemFree(&jsonsServiceId);
        MemFree(&eventTime);
        MemFree(&serviceProperties);
		return NULL;
	}

	*serviceDataInfo->service_id = jsonsServiceId;
	*serviceDataInfo->event_time = eventTime;
	*serviceDataInfo->properties = serviceProperties;
	
	return serviceDataInfo;
    
}

char* EncodeCommandParas(int messageId, int code, char *message, char *requestId) {

    char *payload = NULL;
	JSON *root = JSON_Parse(message);  //Convert string to JSON

	char *object_device_id = JSON_GetStringFromObject(root, "object_device_id", "-1");  //get value of object_device_id
	PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: EncodeCommandParas(), object_device_id=%s\n", object_device_id);

	char *service_id = JSON_GetStringFromObject(root, "service_id", "-1");              //get value of service_id
	PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: EncodeCommandParas(), content=%s\n", service_id);

	char *command_name = JSON_GetStringFromObject(root, "command_name", "-1");          //get value of command_name
	PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: EncodeCommandParas(), name=%s\n", command_name);

	JSON *paras = JSON_GetObjectFromObject(root, "paras");                              //get value of data
	if (paras) {
        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: EncodeCommandParas(), para=%s\n", paras);
        CopyStrValue(&payload, paras, strlen(paras));
	}

	JSON_Delete(root);
	return payload;
}

int CreateServerSocket(char* serverIp, int serverPort, int backlog) {

    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: CreateServerSocket()\n");
    struct sockaddr_in addr = { 0 };
	int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket <= 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: CreateServerSocket() create socket error.\n");
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(serverPort);
	addr.sin_addr.s_addr = inet_addr(serverIp);

	if (bind(server_socket, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: CreateServerSocket() bind error.\n");
        close(server_socket);
		return -1;
	}

	if (listen(server_socket, backlog) != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: CreateServerSocket() listen error.\n");
        close(server_socket);
		return -1;
	}

    return server_socket;
}

void TimeSleep(int ms) {
#if defined(WIN32) || defined(WIN64)
	Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void ProcessMessageFromClient(int  clientSocket) {

    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessMessageFromClient()\n");
    char recvbuf[MAX_MESSAGE_BUF_LEN] = { 0 };
    
	int  recvLen = recv(clientSocket, recvbuf, MAX_MESSAGE_BUF_LEN-1, 0);
	if (recvLen < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: ProcessMessageFromClient() recv error.\n");
        return;
	}
    recvbuf[recvLen] = "\0";

    ST_IOTA_SERVICE_DATA_INFO *payload = DecodeServiceProperty(recvbuf);
    SendReportToIoTPlatform(payload);
	MemFree(&payload);
    
    return;
}

void ProcessEventFromIoTPlatform(int messageId, int code, char *message) {

    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform()\n");
    
    // the demo of how to get the parameter
    JSON *root = JSON_Parse(message);
    
    char *object_device_id = JSON_GetStringFromObject(root, "object_device_id", "-1");          //get value of object_device_id
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), object_device_id %s\n", object_device_id);
    
    JSON *service = JSON_GetObjectFromObject(root, "services");                                 //get object of services
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), services %s\n", service);
    
    int dataSize = JSON_GetArraySize(service);                                                  //get size of services
    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), dataSize %d\n", dataSize);
    
    if (dataSize > 0) {
        JSON *serviceEvent = JSON_GetObjectFromArray(service, 0);                               //get object of ServiceEvent
        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), serviceEvent %s\n", serviceEvent);
        if (serviceEvent) {
            char *service_id = JSON_GetStringFromObject(serviceEvent, "service_id", NULL);      //get value of service_id
            PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), service_id %s\n", service_id);
    
            char *event_type = NULL; //To determine whether to add or delete a sub device
            event_type = JSON_GetStringFromObject(serviceEvent, "event_type", NULL);            //get value of event_type
            PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), event_type %s\n", event_type);
    
            char *event_time = JSON_GetStringFromObject(serviceEvent, "event_time", NULL);      //get value of event_time
            PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), event_time %s\n", event_time);
    
            JSON *paras = JSON_GetObjectFromObject(serviceEvent, "paras");                      //get object of paras
            PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), paras %s\n", paras);
    
            //sub device manager
            if (!strcmp(service_id, "$sub_device_manager")) {
    
                JSON *devices = JSON_GetObjectFromObject(paras, "devices");                     //get object of devices
                PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), devices %s\n", devices);
    
                int version = JSON_GetIntFromObject(paras, "version", -1);                      //get value of version
                PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), version %d\n", version);
    
                int devicesSize = JSON_GetArraySize(devices);                                   //get size of devicesSize
                PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), devicesSize %d\n", devicesSize);
    
                //add a sub device
                if (!strcmp(event_type, "add_sub_device_notify")) {
                    if (devicesSize > 0) {
                        JSON *deviceInfo = JSON_GetObjectFromArray(devices, 0);                 //get object of deviceInfo
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), deviceInfo %s\n", deviceInfo);
    
                        char *parent_device_id = JSON_GetStringFromObject(deviceInfo, "parent_device_id", NULL);    //get value of parent_device_id
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), parent_device_id %s\n", parent_device_id);
    
                        char *node_id = JSON_GetStringFromObject(deviceInfo, "node_id", NULL);    //get value of node_id
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), node_id %s\n", node_id);
    
                        char *subDeviceId = JSON_GetStringFromObject(deviceInfo, "device_id", NULL);    //get value of device_id
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), device_id %s\n", subDeviceId);
    
                        char *name = JSON_GetStringFromObject(deviceInfo, "name", NULL);    //get value of name
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), name %s\n", name);
    
                        char *description = JSON_GetStringFromObject(deviceInfo, "description", NULL);    //get value of description
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), description %s\n", description);
    
                        char *manufacturer_id = JSON_GetStringFromObject(deviceInfo, "manufacturer_id", NULL);    //get value of manufacturer_id
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), manufacturer_id %s\n", manufacturer_id);
    
                        char *model = JSON_GetStringFromObject(deviceInfo, "model", NULL);    //get value of model
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), model %s\n", model);
    
                        char *product_id = JSON_GetStringFromObject(deviceInfo, "product_id", NULL);    //get value of product_id
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), product_id %s\n", product_id);
    
                        char *fw_version = JSON_GetStringFromObject(deviceInfo, "fw_version", NULL);    //get value of fw_version
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), fw_version %s\n", fw_version);
    
                        char *sw_version = JSON_GetStringFromObject(deviceInfo, "sw_version", NULL);    //get value of sw_version
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), sw_version %s\n", sw_version);
    
                        char *status = JSON_GetStringFromObject(deviceInfo, "status", NULL);    //get value of status
                        PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: ProcessEventFromIoTPlatform(), status %s\n", status);
                    }
                }    
            }
        }
    }
    JSON_Delete(root);
}

void SendReportToIoTPlatform(ST_IOTA_SERVICE_DATA_INFO *payload) {

    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: SendReportToIoTPlatform()\n");
    if (payload == NULL) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: SendReportToIoTPlatform() failed, payload is NULL\n");
        return;
    }

    int messageId = IOTA_PropertiesReport(payload, 1);
	if (messageId != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: SendReportToIoTPlatform(), report properties failed, messageId %d\n", messageId);
	}
}

void SendMessageToSubDevice(int clientSocket, char *payload) {

    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: SendMessageToSubDevice()\n");
	if(send(clientSocket, payload, strlen(payload), 0) == -1) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: SendMessageToSubDevice() send error.\n");
	}
}

void SendCommandRspToIoTPlatform(char *requestId) {

    PrintfLog(EN_LOG_LEVEL_INFO, "geteway_demo: SendCommandRspToIoTPlatform()\n");
	char *pcCommandRespense = "{\"SupWh\": \"aaa\"}";      // in service accumulator
	int result_code = 0;
	char *response_name = "cmdResponses";

	int messageId = IOTA_CommandResponse(requestId, result_code, response_name, pcCommandRespense);
	if (messageId != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "geteway_demo: SendCommandRspToIoTPlatform() failed, messageId %d\n", messageId);
	}
}

