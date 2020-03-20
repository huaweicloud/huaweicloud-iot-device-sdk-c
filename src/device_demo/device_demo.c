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

#include <stdio.h>
#include <signal.h>

#if defined(WIN32) || defined(WIN64)
#include "windows.h"
#endif

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include "hw_type.h"
#include "iota_init.h"
#include "iota_cfg.h"
#include "log_util.h"
#include "json_util.h"
#include "string_util.h"
#include "iota_login.h"
#include "iota_datatrans.h"
#include "string.h"
#include "cJSON.h"
#include "iota_error_type.h"

/* if you want to use syslog,you should do this:
 *
 * #include "syslog.h"
 * #define _SYS_LOG
 *
 * */

char *workPath = ".";
char *gatewayId = NULL;

char *serverIp_ = "iot-acc.cn-north-4.myhuaweicloud.com";
int port_ = 8883;

char *username_ = "XXXXX"; //deviceId
char *password_ = "XXXXX";

int disconnected_ = 0;

char *ota_version = NULL;
char *subDeviceId = "XXXX";
void Test_MessageReport(void);
void Test_PropertiesReport(void);
void Test_BatchPropertiesReport(void);
void Test_CommandResponse(char *requestId);
void Test_PropSetResponse(char *requestId);
void Test_PropGetResponse(char *requestId);
void Test_ReportOTAVersion(void);
void Test_ReportUpgradeStatus(int i, char *version);
void HandleAuthSuccess(void *context, int messageId, int code, char *message);
void HandleAuthFailure(void *context, int messageId, int code, char *message);
void HandleConnectionLost(void *context, int messageId, int code, char *message);
void HandleDisAuthSuccess(void *context, int messageId, int code, char *message);
void HandleDisAuthFailure(void *context, int messageId, int code, char *message);
void HandleSubscribesuccess(void *context, int messageId, int code, char *message);
void HandleSubscribeFailure(void *context, int messageId, int code, char *message);
void HandlePublishSuccess(void *context, int messageId, int code, char *message);
void HandlePublishFailure(void *context, int messageId, int code, char *message);
void HandleMessageDown(void *context, int messageId, int code, char *message);
void HandleUserTopicMessageDown(void *context, int messageId, int code, char *message, char *topicParas);
void HandleCommandRequest(void *context, int messageId, int code, char *message, char *requestId);
void HandlePropertiesSet(void *context, int messageId, int code, char *message, char *requestId);
void TimeSleep(int ms);
void HandlePropertiesGet(void *context, int messageId, int code, char *message, char *requestId);
void HandleDeviceShadowRsp(void *context, int messageId, int code, char *message, char *requestId);
void HandleEventsDown(void *context, int messageId, int code, char *message);
void MyPrintLog(int level, char *format, va_list args);
void SetAuthConfig(void);
void SetMyCallbacks(void);

void TimeSleep(int ms) {
#if defined(WIN32) || defined(WIN64)
	Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

//------------------------------------------------------Test  data report---------------------------------------------------------------------

void Test_MessageReport() {
	//default topic
	int messageId = IOTA_MessageReport(NULL, "data123", "123", "hello", NULL);

    //user topic
//	int messageId = IOTA_MessageReport(NULL, "data123", "123", "hello", "devMsg");
	if (messageId != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_MessageReport() failed, messageId %d\n", messageId);
	}
}

void Test_PropertiesReport() {
	int serviceNum = 2; //reported services' totol count
	ST_IOTA_SERVICE_DATA_INFO services[serviceNum];

	//---------------the data of service1-------------------------------
	char *service1 = "{\"Load\":\"6\",\"ImbA_strVal\":\"7\"}";

	services[0].event_time = GetEventTimesStamp(); //if event_time is set to NULL, the time will be the iot-platform's time.
	services[0].service_id = "parameter";
	services[0].properties = service1;

	//---------------the data of service2-------------------------------
	char *service2 = "{\"PhV_phsA\":\"9\",\"PhV_phsB\":\"8\"}";

	services[1].event_time = NULL;
	services[1].service_id = "analog";
	services[1].properties = service2;

	int messageId = IOTA_PropertiesReport(services, serviceNum);
	if (messageId != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropertiesReport() failed, messageId %d\n", messageId);
	}

	MemFree(&services[0].event_time);
}

void Test_BatchPropertiesReport() {
	int deviceNum = 1;      //the number of sub devices
	ST_IOTA_DEVICE_DATA_INFO devices[deviceNum]; //Array of structures to be reported by sub devices
	int serviceList[deviceNum];  //Corresponding to the number of services to be reported for each sub device
	serviceList[0] = 2;       //device 1 reports two services
//	serviceList[1] = 1;		  //device 2 reports one service

	char *device1_service1 = "{\"Load\":\"1\",\"ImbA_strVal\":\"3\"}"; //service1要上报的属性数据，必须是json格式

	char *device1_service2 = "{\"PhV_phsA\":\"2\",\"PhV_phsB\":\"4\"}"; //service2要上报的属性数据，必须是json格式

	devices[0].device_id = subDeviceId;
	devices[0].services[0].event_time = GetEventTimesStamp();
	devices[0].services[0].service_id = "parameter";
	devices[0].services[0].properties = device1_service1;

	devices[0].services[1].event_time = GetEventTimesStamp();
	devices[0].services[1].service_id = "analog";
	devices[0].services[1].properties = device1_service2;

//	char *device2_service1 = "{\"AA\":\"2\",\"BB\":\"4\"}";
//	devices[1].device_id = "subDevices22222";
//	devices[1].services[0].event_time = "d2s1";
//	devices[1].services[0].service_id = "device2_service11111111";
//	devices[1].services[0].properties = device2_service1;

	int messageId = IOTA_BatchPropertiesReport(devices, deviceNum, serviceList);
	if (messageId != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_BatchPropertiesReport() failed, messageId %d\n", messageId);
	}

	MemFree(&devices[0].services[0].event_time);
	MemFree(&devices[0].services[1].event_time);

}

void Test_CommandResponse(char *requestId) {
	char *pcCommandRespense = "{\"SupWh\": \"aaa\"}"; // in service accumulator

	int result_code = 0;
	char *response_name = "cmdResponses";

	int messageId = IOTA_CommandResponse(requestId, result_code, response_name, pcCommandRespense);
	if (messageId != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_CommandResponse() failed, messageId %d\n", messageId);
	}

}

void Test_PropSetResponse(char *requestId) {
	int messageId = IOTA_PropertiesSetResponse(requestId, 0, "success");
	if (messageId != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropSetResponse() failed, messageId %d\n", messageId);
	}

}

void Test_PropGetResponse(char *requestId) {
	int serviceNum = 2;
	ST_IOTA_SERVICE_DATA_INFO serviceProp[serviceNum];

	char *property = "{\"Load\":\"5\",\"ImbA_strVal\":\"6\"}";

	serviceProp[0].event_time = GetEventTimesStamp();
	serviceProp[0].service_id = "parameter";
	serviceProp[0].properties = property;

	char *property2 = "{\"PhV_phsA\":\"2\",\"PhV_phsB\":\"4\"}";

	serviceProp[1].event_time = GetEventTimesStamp();
	serviceProp[1].service_id = "analog";
	serviceProp[1].properties = property2;

	int messageId = IOTA_PropertiesGetResponse(requestId, serviceProp, serviceNum);
	if (messageId != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_PropGetResponse() failed, messageId %d\n", messageId);
	}

	MemFree(&serviceProp[0].event_time);
	MemFree(&serviceProp[1].event_time);

}

void Test_ReportOTAVersion() {
	ST_IOTA_OTA_VERSION_INFO otaVersion;

	otaVersion.event_time = NULL;
	otaVersion.sw_version = "v1.0";
	otaVersion.fw_version = "v1.0";
	otaVersion.object_device_id = NULL;

	int messageId = IOTA_OTAVersionReport(otaVersion);
	if (messageId != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportOTAVersion() failed, messageId %d\n", messageId);
	}
}

void Test_ReportUpgradeStatus(int i, char *version) {
	ST_IOTA_UPGRADE_STATUS_INFO statusInfo;
	if (i == 0) {
		statusInfo.description = "success";
		statusInfo.progress = 100;
		statusInfo.result_code = 0;
		statusInfo.version = version;
	} else {
		statusInfo.description = "failed";
		statusInfo.result_code = 1;
		statusInfo.progress = 0;
		statusInfo.version = version;
	}

	statusInfo.event_time = NULL;
	statusInfo.object_device_id = NULL;

	int messageId = IOTA_OTAStatusReport(statusInfo);
	if (messageId != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: Test_ReportUpgradeStatus() failed, messageId %d\n", messageId);
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------------

void HandleAuthSuccess(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: handleConnectSuccess(), login success\n");
	disconnected_ = 0;
}

void HandleAuthFailure(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: handleConnectFailure() error, messageId %d, code %d, messsage %s\n", messageId, code, message);
	//judge if the network is available etc. and login again
	//...
	PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: handleConnectFailure() login again\n");
	int ret = IOTA_Connect();
	if (ret != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: HandleAuthFailure() error, login again failed, result %d\n", ret);
	}
}

void HandleConnectionLost(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: HandleConnectionLost() error, messageId %d, code %d, messsage %s\n", messageId, code, message);
	//judge if the network is available etc. and login again
	//...
	int ret = IOTA_Connect();
	if (ret != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: HandleConnectionLost() error, login again failed, result %d\n", ret);
	}
}

void HandleDisAuthSuccess(void *context, int messageId, int code, char *message) {
	disconnected_ = 1;

	printf("device_demo: handleLogoutSuccess, login again\n");
	printf("device_demo: HandleDisAuthSuccess(), messageId %d, code %d, messsage %s\n", messageId, code, message);
}

void HandleDisAuthFailure(void *context, int messageId, int code, char *message) {
	printf("device_demo: handleLogoutFailure, login again\n");
	int ret = IOTA_Connect();
	if (ret != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: HandleConnectionLost() error, login again failed, result %d\n", ret);
	}
	printf("device_demo: HandleDisAuthFailure(), messageId %d, code %d, messsage %s\n", messageId, code, message);
}

void HandleSubscribesuccess(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubscribesuccess() messageId %d\n", messageId);
}

void HandleSubscribeFailure(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_WARNING, "device_demo: HandleSubscribeFailure() warning, messageId %d, code %d, messsage %s\n", messageId, code, message);
}

void HandlePublishSuccess(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePublishSuccess() messageId %d\n", messageId);
}

void HandlePublishFailure(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_WARNING, "device_demo: HandlePublishFailure() warning, messageId %d, code %d, messsage %s\n", messageId, code, message);
}

//-------------------------------------------handle  message   arrived------------------------------------------------------------------------------

void HandleMessageDown(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), messageId %d, code %d, messsage %s\n", messageId, code, message);

	JSON *root = JSON_Parse(message);  //Convert string to JSON

	char *content = JSON_GetStringFromObject(root, "content", "-1");     //get value of content
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), content %s\n", content);

	char *object_device_id = JSON_GetStringFromObject(root, "object_device_id", "-1");     //get value of object_device_id
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), object_device_id %s\n", object_device_id);

	char *name = JSON_GetStringFromObject(root, "name", "-1");     //get value of name
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), name %s\n", name);

	char *id = JSON_GetStringFromObject(root, "id", "-1");        //get value of id
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleMessageDown(), id %s\n", id);

	JSON_Delete(root);

}

void HandleUserTopicMessageDown(void *context, int messageId, int code, char *message, char *topicParas) {
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleUserTopicMessageDown(), messageId %d, code %d, messsage %s, topicParas %s \n", messageId, code, message, topicParas);

	JSON *root = JSON_Parse(message);  //Convert string to JSON

	char *content = JSON_GetStringFromObject(root, "content", "-1");     //get value of content
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleUserTopicMessageDown(), content %s\n", content);

	char *object_device_id = JSON_GetStringFromObject(root, "object_device_id", "-1");     //get value of object_device_id
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleUserTopicMessageDown(), object_device_id %s\n", object_device_id);

	char *name = JSON_GetStringFromObject(root, "name", "-1");     //get value of name
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleUserTopicMessageDown(), name %s\n", name);

	char *id = JSON_GetStringFromObject(root, "id", "-1");        //get value of id
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleUserTopicMessageDown(), id %s\n", id);

	JSON_Delete(root);

}

void HandleCommandRequest(void *context, int messageId, int code, char *message, char *requestId) {
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), messageId %d, code %d, messsage %s, requestId %s\n", messageId, code, message, requestId);

	JSON *root = JSON_Parse(message);  //Convert string to JSON

	char *object_device_id = JSON_GetStringFromObject(root, "object_device_id", "-1");     //get value of object_device_id
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), object_device_id %s\n", object_device_id);

	char *service_id = JSON_GetStringFromObject(root, "service_id", "-1");     //get value of service_id
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), service_id %s\n", service_id);

	char *command_name = JSON_GetStringFromObject(root, "command_name", "-1");     //get value of command_name
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), command_name %s\n", command_name);

	JSON *paras = JSON_GetObjectFromObject(root, "paras");       //get value of data
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), id %s\n", paras);

	if (paras) {
		char *property = JSON_GetStringFromObject(paras, "Load", NULL);  //get value of Load defined in profile

		PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleCommandRequest(), Load %s\n", property);
	}

	Test_CommandResponse(requestId);     //command reponse

	JSON_Delete(root);

}

void HandlePropertiesSet(void *context, int messageId, int code, char *message, char *requestId) {
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), messageId %d, code %d, messsage %s, requestId %s\n", messageId, code, message, requestId);

	JSON *root = JSON_Parse(message);  //Convert string to JSON

	char *object_device_id = JSON_GetStringFromObject(root, "object_device_id", "-1");     //get value of object_device_id
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), object_device_id %s\n", object_device_id);

	JSON *services = JSON_GetObjectFromObject(root, "services");                            //get  services array
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), services %s\n", services);

	int dataSize = JSON_GetArraySize(services);                                            //get length of services array
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), dataSize %d\n", dataSize);

	if (dataSize > 0) {
		JSON *service = JSON_GetObjectFromArray(services, 0);                //only get the first one to demonstrate
		PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleSubDeviceMessageDown(), service %s\n", service);
		if (service) {
			char *service_id = JSON_GetStringFromObject(service, "service_id", NULL);

			PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), service_id %s\n", service_id);

			JSON *properties = JSON_GetObjectFromObject(service, "properties");

			char *Load = JSON_GetStringFromObject(properties, "Load", NULL);
			PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesSet(), Load %s\n", Load);
		}
	}

	Test_PropSetResponse(requestId);  //command response

	JSON_Delete(root);
}

void HandlePropertiesGet(void *context, int messageId, int code, char *message, char *requestId) {
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesGet(), messageId %d, code %d, messsage %s, requestId %s\n", messageId, code, message, requestId);

	JSON *root = JSON_Parse(message);

	char *object_device_id = JSON_GetStringFromObject(root, "object_device_id", "-1");     //get value of object_device_id
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesGet(), object_device_id %s\n", object_device_id);

	char *service_id = JSON_GetStringFromObject(root, "service_id", "-1");     //get value of service_id
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandlePropertiesGet(), service_id %s\n", service_id);

	Test_PropGetResponse(requestId);  //command response

	JSON_Delete(root);
}

void HandleDeviceShadowRsp(void *context, int messageId, int code, char *message, char *requestId) {
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleDeviceShadowRsp(), messageId %d, code %d, messsage %s, requestId %s\n", messageId, code, message, requestId);
	//Start analyzing data, please refer to function HandleEventsDown
}

void HandleEventsDown(void *context, int messageId, int code, char *message) {
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), messageId %d, code %d, messsage %s\n", messageId, code, message);

	// the demo of how to get the parameter
	JSON *root = JSON_Parse(message);

	char *object_device_id = JSON_GetStringFromObject(root, "object_device_id", "-1");           //get value of object_device_id
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), object_device_id %s\n", object_device_id);

	JSON *service = JSON_GetObjectFromObject(root, "services");                                 //get object of services
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), services %s\n", service);

	int dataSize = JSON_GetArraySize(service);                                                 //get size of services
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), dataSize %d\n", dataSize);

	if (dataSize > 0) {
		JSON *serviceEvent = JSON_GetObjectFromArray(service, 0);                              //get object of ServiceEvent
		PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), serviceEvent %s\n", serviceEvent);
		if (serviceEvent) {
			char *service_id = JSON_GetStringFromObject(serviceEvent, "service_id", NULL);    //get value of service_id
			PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), service_id %s\n", service_id);

			char *event_type = NULL; //To determine whether to add or delete a sub device
			event_type = JSON_GetStringFromObject(serviceEvent, "event_type", NULL);    //get value of event_type
			PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), event_type %s\n", event_type);

			char *event_time = JSON_GetStringFromObject(serviceEvent, "event_time", NULL);    //get value of event_time
			PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), event_time %s\n", event_time);

			JSON *paras = JSON_GetObjectFromObject(serviceEvent, "paras");                              //get object of paras
			PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), paras %s\n", paras);

			//sub device manager
			if (!strcmp(service_id, "$sub_device_manager")) {

				JSON *devices = JSON_GetObjectFromObject(paras, "devices");                                 //get object of devices
				PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), devices %s\n", devices);

				int version = JSON_GetIntFromObject(paras, "version", -1);                             //get value of version
				PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), version %d\n", version);

				int devicesSize = JSON_GetArraySize(devices);                                                 //get size of devicesSize
				PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), devicesSize %d\n", devicesSize);

				//add a sub device
				if (!strcmp(event_type, "add_sub_device_notify")) {

					if (devicesSize > 0) {
						JSON *deviceInfo = JSON_GetObjectFromArray(devices, 0);                                 //get object of deviceInfo
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), deviceInfo %s\n", deviceInfo);

						char *parent_device_id = JSON_GetStringFromObject(deviceInfo, "parent_device_id", NULL);    //get value of parent_device_id
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), parent_device_id %s\n", parent_device_id);

						char *node_id = JSON_GetStringFromObject(deviceInfo, "node_id", NULL);    //get value of node_id
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), node_id %s\n", node_id);

						subDeviceId = JSON_GetStringFromObject(deviceInfo, "device_id", NULL);    //get value of device_id
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), device_id %s\n", subDeviceId);

						char *name = JSON_GetStringFromObject(deviceInfo, "name", NULL);    //get value of name
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), name %s\n", name);

						char *description = JSON_GetStringFromObject(deviceInfo, "description", NULL);    //get value of description
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), description %s\n", description);

						char *manufacturer_id = JSON_GetStringFromObject(deviceInfo, "manufacturer_id", NULL);    //get value of manufacturer_id
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), manufacturer_id %s\n", manufacturer_id);

						char *model = JSON_GetStringFromObject(deviceInfo, "model", NULL);    //get value of model
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), model %s\n", model);

						char *product_id = JSON_GetStringFromObject(deviceInfo, "product_id", NULL);    //get value of product_id
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), product_id %s\n", product_id);

						char *fw_version = JSON_GetStringFromObject(deviceInfo, "fw_version", NULL);    //get value of fw_version
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), fw_version %s\n", fw_version);

						char *sw_version = JSON_GetStringFromObject(deviceInfo, "sw_version", NULL);    //get value of sw_version
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), sw_version %s\n", sw_version);

						char *status = JSON_GetStringFromObject(deviceInfo, "status", NULL);    //get value of status
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), status %s\n", status);

						//command response
						Test_BatchPropertiesReport();
					}

				}

				//delete a sub device
				if (!strcmp(event_type, "delete_sub_device_notify")) {

					if (devicesSize > 0) {
						JSON *deviceInfo = JSON_GetObjectFromArray(devices, 0);                                 //get object of deviceInfo
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), deviceInfo %s\n", deviceInfo);

						char *parent_device_id = JSON_GetStringFromObject(deviceInfo, "parent_device_id", NULL);    //get value of parent_device_id
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), parent_device_id %s\n", parent_device_id);

						char *node_id = JSON_GetStringFromObject(deviceInfo, "node_id", NULL);    //get value of node_id
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), node_id %s\n", node_id);

						subDeviceId = JSON_GetStringFromObject(deviceInfo, "device_id", NULL);    //get value of device_id
						PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), device_id %s\n", subDeviceId);

					}

				}

			}

			//OTA
			if (!strcmp(service_id, "$ota")) {

				if (!strcmp(event_type, "version_query")) {
					//report OTA version
					Test_ReportOTAVersion();

				}

				//firmware_upgrade or software_upgrade
				if ((!strcmp(event_type, "firmware_upgrade")) || (!strcmp(event_type, "software_upgrade"))) {
					ota_version = JSON_GetStringFromObject(paras, "version", NULL);    //get value of version
					PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), version %s\n", ota_version);

					char *url = JSON_GetStringFromObject(paras, "url", NULL);    //get value of url
					PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), url %s\n", url);

					int file_size = JSON_GetIntFromObject(paras, "file_size", -1);    //get value of file_size
					PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), file_size %d\n", file_size);

					char *access_token = JSON_GetStringFromObject(paras, "access_token", NULL);    //get value of access_token
					PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), access_token %s\n", access_token);

					int expires = JSON_GetIntFromObject(paras, "expires", -1);    //get value of expires
					PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), expires %d\n", expires);

					char *sign = JSON_GetStringFromObject(paras, "sign", NULL);    //get value of sign
					PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: HandleEventsDown(), sign %s\n", sign);

					//start to receive packages and firmware_upgrade or software_upgrade

					if (IOTA_GetOTAPackages(url, access_token, 1000) == 0) {
						usleep(3000 * 1000);
						//report successful upgrade status
						Test_ReportUpgradeStatus(0, ota_version);
					} else {
						//report failed status
						Test_ReportUpgradeStatus(-1, ota_version);
					}

				}

			}

		}
	}

	JSON_Delete(root);
}

//----------------------------------------------------------------------------------------------------------------------------------------------

void SetAuthConfig() {
	IOTA_ConfigSetStr(EN_IOTA_CFG_MQTT_ADDR, serverIp_);
	IOTA_ConfigSetUint(EN_IOTA_CFG_MQTT_PORT, port_);
	IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICEID, username_);
	IOTA_ConfigSetStr(EN_IOTA_CFG_DEVICESECRET, password_);
	IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_SECRET);
	/**
	 * Configuration is required in certificate mode:
	 *
	 * IOTA_ConfigSetUint(EN_IOTA_CFG_AUTH_MODE, EN_IOTA_CFG_AUTH_MODE_CERT);
	 * IOTA_ConfigSetUint(EN_IOTA_CFG_PRIVATE_KEY_PASSWORD, "yourPassword");
	 *
	 */

#ifdef _SYS_LOG
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LOCAL_NUMBER, LOG_LOCAL7);
    IOTA_ConfigSetUint(EN_IOTA_CFG_LOG_LEVEL, LOG_INFO);
#endif
}

void SetMyCallbacks() {
	IOTA_SetCallback(EN_IOTA_CALLBACK_CONNECT_SUCCESS, HandleAuthSuccess);
	IOTA_SetCallback(EN_IOTA_CALLBACK_CONNECT_FAILURE, HandleAuthFailure);

	IOTA_SetCallback(EN_IOTA_CALLBACK_DISCONNECT_SUCCESS, HandleDisAuthSuccess);
	IOTA_SetCallback(EN_IOTA_CALLBACK_DISCONNECT_FAILURE, HandleDisAuthFailure);
	IOTA_SetCallback(EN_IOTA_CALLBACK_CONNECTION_LOST, HandleConnectionLost);

	IOTA_SetCallback(EN_IOTA_CALLBACK_SUBSCRIBE_SUCCESS, HandleSubscribesuccess);
	IOTA_SetCallback(EN_IOTA_CALLBACK_SUBSCRIBE_FAILURE, HandleSubscribeFailure);

	IOTA_SetCallback(EN_IOTA_CALLBACK_PUBLISH_SUCCESS, HandlePublishSuccess);
	IOTA_SetCallback(EN_IOTA_CALLBACK_PUBLISH_FAILURE, HandlePublishFailure);

	IOTA_SetCallback(EN_IOTA_CALLBACK_MESSAGE_DOWN, HandleMessageDown);
	IOTA_SetCallbackWithTopic(EN_IOTA_CALLBACK_COMMAND_REQUEST, HandleCommandRequest);
	IOTA_SetCallbackWithTopic(EN_IOTA_CALLBACK_PROPERTIES_SET, HandlePropertiesSet);
	IOTA_SetCallbackWithTopic(EN_IOTA_CALLBACK_PROPERTIES_GET, HandlePropertiesGet);
	IOTA_SetCallback(EN_IOTA_CALLBACK_EVENT_DOWN, HandleEventsDown);
	IOTA_SetCallbackWithTopic(EN_IOTA_CALLBACK_USER_TOPIC, HandleUserTopicMessageDown);
	IOTA_SetCallbackWithTopic(EN_IOTA_CALLBACK_DEVICE_SHADOW, HandleDeviceShadowRsp);
}

void MyPrintLog(int level, char *format, va_list args) {
	vprintf(format, args);
	/*if you want to printf log in system log files,you can do this:
	 *
	 * vsyslog(level, format, args);
	 * */
}

int main(int argc, char **argv) {
#if defined(_DEBUG)
	setvbuf(stdout, NULL, _IONBF, 0); //in order to make the console log printed immediately at debug mode
#endif

	IOTA_SetPrintLogCallback(MyPrintLog);
	PrintfLog(EN_LOG_LEVEL_INFO, "device_demo: start test ===================>\n");

	if (IOTA_Init(workPath) < 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: IOTA_Init() error, init failed\n");
		return 1;
	}

	SetAuthConfig();
	SetMyCallbacks();

	//see handleLoginSuccess and handleLoginFailure for login result
	int ret = IOTA_Connect();
	if (ret != 0) {
		PrintfLog(EN_LOG_LEVEL_ERROR, "device_demo: IOTA_Auth() error, Auth failed, result %d\n", ret);
	}

	TimeSleep(10500);
	int count = 0;
	while (count < 10000) {

		//message up
		Test_MessageReport();

		//properties report
		Test_PropertiesReport();

		//batchProperties report
		Test_BatchPropertiesReport();

		//command response
		Test_CommandResponse("1005");

		TimeSleep(1500);

		//propSetResponse
		Test_PropSetResponse("1006");

		TimeSleep(1500);

		//propSetResponse
		Test_PropGetResponse("1007");

		TimeSleep(5500);

		IOTA_SubscribeUserTopic("devMsg");

		//get device shadow
		IOTA_GetDeviceShadow("1232", NULL, NULL);

		count++;
	}

	while (1) {
		TimeSleep(50);
	}

	return 0;
}

