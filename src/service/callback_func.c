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

#include "string_util.h"
#include "log_util.h"
#include "mqtt_base.h"
#include "base.h"
#include "callback_func.h"
#include "subscribe.h"
#include "string.h"
#include "iota_error_type.h"
#include "json_util.h"
#include "cJSON.h"
#include "iota_datatrans.h"

EVENT_CALLBACK_HANDLER onEventDown;
CMD_CALLBACK_HANDLER onCmd;
CMD_CALLBACK_HANDLER_V3 onCmdV3;
PROTOCOL_CALLBACK_HANDLER onConnSuccess;
MESSAGE_CALLBACK_HANDLER onMessage;
PROP_SET_CALLBACK_HANDLER onPropertiesSet;
PROP_GET_CALLBACK_HANDLER onPropertiesGet;
SHADOW_GET_CALLBACK_HANDLER onDeviceShadow;
USER_TOPIC_MSG_CALLBACK_HANDLER onUserTopicMessage;
BOOTSTRAP_CALLBACK_HANDLER onBootstrap;

void OnLoginSuccess(EN_IOTA_MQTT_PROTOCOL_RSP *rsp) {
	SubscribeAll();
	if (onConnSuccess) {
		(onConnSuccess)(rsp);
	}
}

void OnMessageArrived(void *context, int token, int code, const char *topic, char *message) {

	if (StringLength(StrInStr(topic, BOOTSTRAP_DOWN)) > 0) {
		EN_IOTA_MQTT_PROTOCOL_RSP *bootstrap_msg  = (EN_IOTA_MQTT_PROTOCOL_RSP*)malloc(sizeof(EN_IOTA_MQTT_PROTOCOL_RSP));
		if (bootstrap_msg == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "OnMessageArrived(): there is not enough memory here.\n");
			return;
		}
		bootstrap_msg->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
		if (bootstrap_msg->mqtt_msg_info == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "OnMessageArrived(): there is not enough memory here.\n");
			free(bootstrap_msg);
			return;
		}
		bootstrap_msg->mqtt_msg_info->context = context;
		bootstrap_msg->mqtt_msg_info->messageId = token;
		bootstrap_msg->mqtt_msg_info->code = code;

		if (onBootstrap) {
			(onBootstrap)(bootstrap_msg);
		}

		MemFree(&bootstrap_msg->mqtt_msg_info);
		MemFree(&bootstrap_msg);

		return;
	}



	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_MESSAGEDOWN)) > 0) {
		EN_IOTA_MESSAGE *msg  = (EN_IOTA_MESSAGE*)malloc(sizeof(EN_IOTA_MESSAGE));
		if (msg == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "OnMessageArrived(): there is not enough memory here.\n");
			return;
		}
		msg->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
		if (msg->mqtt_msg_info == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "OnMessageArrived(): there is not enough memory here.\n");
			free(msg);
			return;
		}
		msg->mqtt_msg_info->context = context;
		msg->mqtt_msg_info->messageId = token;
		msg->mqtt_msg_info->code = code;

		JSON *root = JSON_Parse(message);

		char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");
		msg->object_device_id = object_device_id;

		char *name = JSON_GetStringFromObject(root, NAME, "-1");
		msg->name = name;

		char *id = JSON_GetStringFromObject(root, ID, "-1");
		msg->id = id;

		char *content = JSON_GetStringFromObject(root, CONTENT, "-1");
		msg->content = content;

		if (onMessage) {
			(onMessage)(msg);
		}

		JSON_Delete(root);
		MemFree(&msg->mqtt_msg_info);
		MemFree(&msg);
		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_PREFIX_V3)) > 0) {
		EN_IOTA_COMMAND_V3 *command_v3  = (EN_IOTA_COMMAND*)malloc(sizeof(EN_IOTA_COMMAND));
		if (command_v3 == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "HandleEventsDown(): there is not enough memory here.\n");
			return;
		}
		command_v3->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
		if (command_v3->mqtt_msg_info == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "HandleEventsDown(): there is not enough memory here.\n");
			free(command_v3);
			return;
		}
		command_v3->mqtt_msg_info->context = context;
		command_v3->mqtt_msg_info->messageId = token;
		command_v3->mqtt_msg_info->code = code;

		command_v3->msgType = CLOUD_REQ;

		JSON *root = JSON_Parse(message);

		char *serviceId = JSON_GetStringFromObject(root, SERVICE_ID_V3, "-1");           //get value of serviceId
		command_v3->serviceId = serviceId;

		char *cmd = JSON_GetStringFromObject(root, CMD, "-1");     //get value of cmd
		command_v3->cmd = cmd;

		int mid = JSON_GetIntFromObject(root, MID, -1);     //get value of mid
		command_v3->mid = mid;

		JSON *paras = JSON_GetObjectFromObject(root, PARAS);       //get value of paras

		char *paras_obj = cJSON_Print(paras);
		command_v3->paras = paras_obj;

		if (onCmdV3) {
			(onCmdV3)(command_v3);
		}

		JSON_Delete(root);
		MemFree(&paras_obj);
		MemFree(&command_v3->mqtt_msg_info);
		MemFree(&command_v3);

		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_COMMAND)) > 0) {
		char *requestId_value = strstr(topic, "=");
		char *request_id = strstr(requestId_value + 1, "");

		EN_IOTA_COMMAND *command  = (EN_IOTA_COMMAND*)malloc(sizeof(EN_IOTA_COMMAND));
		if (command == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			return;
		}

		command->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
		if (command->mqtt_msg_info == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			free(command);
			return;
		}
		command->mqtt_msg_info->context = context;
		command->mqtt_msg_info->messageId = token;
		command->mqtt_msg_info->code = code;

		command->request_id = request_id;

		JSON *root = JSON_Parse(message);

		char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");           //get value of object_device_id
		command->object_device_id = object_device_id;

		char *service_id = JSON_GetStringFromObject(root, SERVICE_ID, "-1");     //get value of service_id
		command->service_id = service_id;

		char *command_name = JSON_GetStringFromObject(root, COMMAND_NAME, "-1");     //get value of command_name
		command->command_name = command_name;

		JSON *paras = JSON_GetObjectFromObject(root, PARAS);       //get value of data

		char *paras_obj = cJSON_Print(paras);
		command->paras = paras_obj;

		if (onCmd) {
			(onCmd)(command);
		}

		JSON_Delete(root);
		MemFree(&paras_obj);
		MemFree(&command->mqtt_msg_info);
		MemFree(&command);

		return;

	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_SET)) > 0) {
		char *requestId_value = strstr(topic, "=");
		char *request_id = strstr(requestId_value + 1, "");

		EN_IOTA_PROPERTY_SET *prop_set = (EN_IOTA_PROPERTY_SET*)malloc(sizeof(EN_IOTA_PROPERTY_SET));
		if (prop_set == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			return;
		}

		prop_set->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
		if (prop_set->mqtt_msg_info == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			free(prop_set);
			return;
		}
		prop_set->mqtt_msg_info->context = context;
		prop_set->mqtt_msg_info->messageId = token;
		prop_set->mqtt_msg_info->code = code;

		prop_set->request_id = request_id;

		JSON *root = JSON_Parse(message);

		char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");     //get value of object_device_id
		prop_set->object_device_id = object_device_id;

		JSON *services = JSON_GetObjectFromObject(root, SERVICES);                            //get  services array

		int services_count = JSON_GetArraySize(services);                                            //get length of services array
		prop_set->services_count = services_count;


		if (services_count >= MAX_SERVICE_COUNT) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "the number of service exceeds.\n");
			JSON_Delete(root);
			free(prop_set->mqtt_msg_info);
			free(prop_set);
			return;
		}

		prop_set->services = (EN_IOTA_SERVICE_PROPERTY*)malloc(sizeof(EN_IOTA_SERVICE_PROPERTY) * services_count);
		if (prop_set->services == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			JSON_Delete(root);
			free(prop_set->mqtt_msg_info);
			free(prop_set);
			return;
		}

		int i = 0;
		char *prop[MAX_SERVICE_COUNT]= {0};
		while (services_count > 0) {
			JSON *service = JSON_GetObjectFromArray(services, i);
			if (service) {
				char *service_id = JSON_GetStringFromObject(service, SERVICE_ID, NULL);
				prop_set->services[i].service_id = service_id;

				JSON *properties = JSON_GetObjectFromObject(service, PROPERTIES);
				prop[i] = cJSON_Print(properties);
				prop_set->services[i].properties = prop[i];

			}

			i++;
			services_count--;
		}

		if (onPropertiesSet) {
			(onPropertiesSet)(prop_set);
		}

		JSON_Delete(root);
		int j;
		for (j = 0;j < i;j++) {
			MemFree(&prop[j]);
		}
		MemFree(&prop);
		MemFree(&prop_set->services);
		MemFree(&prop_set->mqtt_msg_info);
		MemFree(&prop_set);

		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_GET)) > 0) {
		char *requestId_value = strstr(topic, "=");
		char *request_id = strstr(requestId_value + 1, "");

		EN_IOTA_PROPERTY_GET *prop_get = (EN_IOTA_PROPERTY_GET*)malloc(sizeof(EN_IOTA_PROPERTY_GET));
		if (prop_get == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			return;
		}

		prop_get->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
		if (prop_get->mqtt_msg_info == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			free(prop_get);
			return;
		}
		prop_get->mqtt_msg_info->context = context;
		prop_get->mqtt_msg_info->messageId = token;
		prop_get->mqtt_msg_info->code = code;

		prop_get->request_id = request_id;

		JSON *root = JSON_Parse(message);

		char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");     //get value of object_device_id
		prop_get->object_device_id = object_device_id;

		char *service_id = JSON_GetStringFromObject(root, SERVICE_ID, "-1");     //get value of object_device_id
		prop_get->service_id = service_id;

		if (onPropertiesGet) {
			(onPropertiesGet)(prop_get);
		}

		JSON_Delete(root);
		MemFree(&prop_get->mqtt_msg_info);
		MemFree(&prop_get);
		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_PROP_RSP)) > 0) {
		char *requestId_value = strstr(topic, "=");
		char *request_id = strstr(requestId_value + 1, "");

		EN_IOTA_DEVICE_SHADOW *device_shadow = (EN_IOTA_DEVICE_SHADOW*)malloc(sizeof(EN_IOTA_DEVICE_SHADOW));
		if (device_shadow == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			return;
		}

		device_shadow->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
		if (device_shadow->mqtt_msg_info == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			free(device_shadow);
			return;
		}
		device_shadow->mqtt_msg_info->context = context;
		device_shadow->mqtt_msg_info->messageId = token;
		device_shadow->mqtt_msg_info->code = code;

		device_shadow->request_id = request_id;

		JSON *root = JSON_Parse(message);

		char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");
		device_shadow->object_device_id = object_device_id;

		JSON *shadow = JSON_GetObjectFromObject(root, SHADOW);
		int shadow_data_count = 0;
		if (shadow != NULL) {
			shadow_data_count = JSON_GetArraySize(shadow);
			device_shadow->shadow_data_count = shadow_data_count;
		}

		if (shadow_data_count >= MAX_SERVICE_COUNT) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "the number of shadow service exceeds.\n");
			JSON_Delete(root);
			free(device_shadow->mqtt_msg_info);
			free(device_shadow);
			return;
		}

		device_shadow->shadow = (EN_IOTA_SHADOW_DATA*)malloc(sizeof(EN_IOTA_SHADOW_DATA) * shadow_data_count);
		if (device_shadow->shadow == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			JSON_Delete(root);
			free(device_shadow->mqtt_msg_info);
			free(device_shadow);
			return;
		}
		int i = 0;
		char *desired_str[MAX_SERVICE_COUNT]= {0};
		char *reported_str[MAX_SERVICE_COUNT]= {0};
		while (shadow_data_count > 0) {
			JSON *shadow_data = JSON_GetObjectFromArray(shadow, i);

			if (shadow_data) {
				char *service_id = JSON_GetStringFromObject(shadow_data, SERVICE_ID, "-1");
				device_shadow->shadow[i].service_id = service_id;

				JSON *desired = JSON_GetObjectFromObject(shadow_data, DESIRED);
				if (desired) {
					char *desired_event_time = JSON_GetStringFromObject(desired, EVENT_TIME, "-1");
					device_shadow->shadow[i].desired_event_time = desired_event_time;
					JSON *desired_properties = JSON_GetObjectFromObject(desired, PROPERTIES);
					desired_str[i] = cJSON_Print(desired_properties);
					device_shadow->shadow[i].desired_properties = desired_str[i];
				}

				JSON *reported = JSON_GetObjectFromObject(shadow_data, REPORTED);
				if (reported) {
					char *reported_event_time = JSON_GetStringFromObject(reported, EVENT_TIME, "-1");
					device_shadow->shadow[i].desired_event_time = reported_event_time;
					JSON *reported_properties = JSON_GetObjectFromObject(reported, PROPERTIES);
					reported_str[i] = cJSON_Print(reported_properties);
					device_shadow->shadow[i].reported_properties = reported_str[i];
				}

				int version = JSON_GetIntFromObject(shadow_data, VERSION, -1);
				device_shadow->shadow[i].version = version;

			}

			i++;
			shadow_data_count--;

		}

		if (onDeviceShadow) {
			(onDeviceShadow)(device_shadow);
		}
		JSON_Delete(root);
		int j;
		for (j = 0;j < i;j++) {
			MemFree(&desired_str[j]);
			MemFree(&reported_str[j]);
		}

		MemFree(&device_shadow->shadow);
		MemFree(&device_shadow->mqtt_msg_info);
		MemFree(&device_shadow);
		return;
	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_USER)) > 0) {
		char *topic_paras_value = strstr(topic, "user/");
		char *topic_paras = strstr(topic_paras_value + 5, "");


		EN_IOTA_USER_TOPIC_MESSAGE *user_topic_msg  = (EN_IOTA_USER_TOPIC_MESSAGE*)malloc(sizeof(EN_IOTA_USER_TOPIC_MESSAGE));
		if (user_topic_msg == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "HandleEventsDown(): there is not enough memory here.\n");
			return;
		}
		user_topic_msg->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
		if (user_topic_msg->mqtt_msg_info == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "HandleEventsDown(): there is not enough memory here.\n");
			free(user_topic_msg);
			return;
		}
		user_topic_msg->mqtt_msg_info->context = context;
		user_topic_msg->mqtt_msg_info->messageId = token;
		user_topic_msg->mqtt_msg_info->code = code;

		user_topic_msg->topic_para = topic_paras;

		JSON *root = JSON_Parse(message);

		char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");
		user_topic_msg->object_device_id = object_device_id;

		char *name = JSON_GetStringFromObject(root, NAME, "-1");
		user_topic_msg->name = name;

		char *id = JSON_GetStringFromObject(root, ID, "-1");
		user_topic_msg->id = id;

		char *content = JSON_GetStringFromObject(root, CONTENT, "-1");
		user_topic_msg->content = content;

		if (onUserTopicMessage) {
			(onUserTopicMessage)(user_topic_msg);
		}

		JSON_Delete(root);
		MemFree(&user_topic_msg->mqtt_msg_info);
		MemFree(&user_topic_msg);
		return;

	}

	if (StringLength(StrInStr(topic, TOPIC_SUFFIX_EVENT_DOWN)) > 0) {

		EN_IOTA_EVENT *event = (EN_IOTA_EVENT*)malloc(sizeof(EN_IOTA_EVENT));
		if (event == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			return;
		}

		event->mqtt_msg_info = (EN_IOTA_MQTT_MSG_INFO*)malloc(sizeof(EN_IOTA_MQTT_MSG_INFO));
		if (event->mqtt_msg_info == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			free(event);
			return;
		}
		event->mqtt_msg_info->context = context;
		event->mqtt_msg_info->messageId = token;
		event->mqtt_msg_info->code = code;


		JSON *root = JSON_Parse(message);

		char *object_device_id = JSON_GetStringFromObject(root, OBJECT_DEVICE_ID, "-1");           //get value of object_device_id
		event->object_device_id = object_device_id;

		JSON *services = JSON_GetObjectFromObject(root, SERVICES);                                 //get object of services

		int services_count = 0;
		services_count = JSON_GetArraySize(services);                                                 //get size of services

		if (services_count > MAX_EVENT_COUNT) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "messageArrivaled: services_count is too large.\n"); //you can increase the MAX_EVENT_COUNT in iota_init.h
			JSON_Delete(root);
			MemFree(&event);
			return;
		}

		event->services_count = services_count;
		event->services = (EN_IOTA_SERVICE_EVENT*)malloc(sizeof(EN_IOTA_SERVICE_EVENT) * services_count);
		if (event->services == NULL) {
			PrintfLog(EN_LOG_LEVEL_ERROR, "there is not enough memory here.\n");
			free(event->mqtt_msg_info);
			free(event);
			return;
		}

		int services_count_copy = services_count;  // for releasing the services

		int i = 0;
		while(services_count > 0) {
			JSON *serviceEvent = JSON_GetObjectFromArray(services, i);                              //get object of ServiceEvent
			if (serviceEvent) {
				char *service_id = JSON_GetStringFromObject(serviceEvent, SERVICE_ID, NULL);    //get value of service_id
				event->services[i].servie_id = EN_IOTA_EVENT_ERROR;  //init service id

				char *event_type = NULL; //To determine whether to add or delete a sub device
				event_type = JSON_GetStringFromObject(serviceEvent, EVENT_TYPE, NULL);    //get value of event_type
				event->services[i].event_type = EN_IOTA_EVENT_TYPE_ERROR; //init event type

				char *event_time = JSON_GetStringFromObject(serviceEvent, EVENT_TIME, NULL);    //get value of event_time
				event->services[i].event_time = event_time;

				JSON *paras = JSON_GetObjectFromObject(serviceEvent, PARAS);                    //get object of paras

				//sub device manager
				if (!strcmp(service_id, SUB_DEVICE_MANAGER)) {

					event->services[i].servie_id = EN_IOTA_EVENT_SUB_DEVICE_MANAGER;
					event->services[i].paras = (EN_IOTA_DEVICE_PARAS*)malloc(sizeof(EN_IOTA_DEVICE_PARAS));
					if (event->services[i].paras == NULL) {
						PrintfLog(EN_LOG_LEVEL_ERROR, "HandleEventsDown(): there is not enough memory here.\n");
						free(event->services);
						free(event->mqtt_msg_info);
						free(event);
						return;
					}

					JSON *devices = JSON_GetObjectFromObject(paras, DEVICES);                                 //get object of devices

					int devices_count = JSON_GetArraySize(devices);                                                 //get size of devicesSize

					if (devices_count > MAX_EVENT_DEVICE_COUNT) {
						PrintfLog(EN_LOG_LEVEL_ERROR, "messageArrivaled: HandleEventsDown(), devices_count is too large.\n"); //you can increase the MAX_EVENT_DEVICE_COUNT in iota_init.h
						JSON_Delete(root);
						break;
					}

					event->services[i].paras->devices = (EN_IOTA_DEVICE_INFO*)malloc(sizeof(EN_IOTA_DEVICE_INFO) * devices_count);
					if (event->services[i].paras == NULL) {
						PrintfLog(EN_LOG_LEVEL_ERROR, "HandleEventsDown(): there is not enough memory here.\n");
						while(i >= 0) {
							free(event->services[i].paras);
							i--;
						}
						free(event->services);
						free(event->mqtt_msg_info);
						free(event);
						return;
					}

					event->services[i].paras->devices_count = devices_count;

					long long version = getLLongValueFromStr(message, VERSION_JSON);
					event->services[i].paras->version = version;
					int j = 0;

					//add a sub device
					if (!strcmp(event_type, ADD_SUB_DEVICE_NOTIFY)) {
						event->services[i].event_type = EN_IOTA_EVENT_ADD_SUB_DEVICE_NOTIFY;
						while(devices_count > 0) {
							JSON *deviceInfo = JSON_GetObjectFromArray(devices, j);                                 //get object of deviceInfo

							char *parent_device_id = JSON_GetStringFromObject(deviceInfo, PARENT_DEVICE_ID, NULL);    //get value of parent_device_id
							event->services[i].paras->devices[j].parent_device_id = parent_device_id;

							char *node_id = JSON_GetStringFromObject(deviceInfo, NODE_ID, NULL);    //get value of node_id
							event->services[i].paras->devices[j].node_id = node_id;

							char *device_id = JSON_GetStringFromObject(deviceInfo, DEVICE_ID, NULL);    //get value of device_id
							event->services[i].paras->devices[j].device_id = device_id;

							char *name = JSON_GetStringFromObject(deviceInfo, NAME, NULL);    //get value of name
							event->services[i].paras->devices[j].name = name;

							char *description = JSON_GetStringFromObject(deviceInfo, DESCRIPTION, NULL);    //get value of description
							event->services[i].paras->devices[j].description = description;

							char *manufacturer_id = JSON_GetStringFromObject(deviceInfo, MANUFACTURER_ID, NULL);    //get value of manufacturer_id
							event->services[i].paras->devices[j].manufacturer_id = manufacturer_id;

							char *model = JSON_GetStringFromObject(deviceInfo, MODEL, NULL);    //get value of model
							event->services[i].paras->devices[j].model = model;

							char *product_id = JSON_GetStringFromObject(deviceInfo, PRODUCT_ID, NULL);    //get value of product_id
							event->services[i].paras->devices[j].product_id = product_id;

							char *fw_version = JSON_GetStringFromObject(deviceInfo, FW_VERSION, NULL);    //get value of fw_version
							event->services[i].paras->devices[j].fw_version = fw_version;

							char *sw_version = JSON_GetStringFromObject(deviceInfo, SW_VERSION, NULL);    //get value of sw_version
							event->services[i].paras->devices[j].sw_version = sw_version;

							char *status = JSON_GetStringFromObject(deviceInfo, STATUS, NULL);    //get value of status
							event->services[i].paras->devices[j].status = status;

							j++;
							devices_count--;
						}
					}

					//delete a sub device
					if (!strcmp(event_type, DELETE_SUB_DEVICE_NOTIFY)) {
						event->services[i].event_type = EN_IOTA_EVENT_DELETE_SUB_DEVICE_NOTIFY;
						while(devices_count > 0) {
							JSON *deviceInfo = JSON_GetObjectFromArray(devices, j);                                 //get object of deviceInfo

							char *parent_device_id = JSON_GetStringFromObject(deviceInfo, PARENT_DEVICE_ID, NULL);    //get value of parent_device_id
							event->services[i].paras->devices[j].parent_device_id = parent_device_id;

							char *node_id = JSON_GetStringFromObject(deviceInfo, NODE_ID, NULL);    //get value of node_id
							event->services[i].paras->devices[j].node_id = node_id;

							char *device_id = JSON_GetStringFromObject(deviceInfo, DEVICE_ID, NULL);    //get value of device_id
							event->services[i].paras->devices[j].device_id = device_id;

							j++;
							devices_count--;

						}
					}
				}

				//OTA
				if (!strcmp(service_id, OTA)) {

					event->services[i].servie_id = EN_IOTA_EVENT_OTA;
					event->services[i].ota_paras = (EN_IOTA_OTA_PARAS*)malloc(sizeof(EN_IOTA_OTA_PARAS));
					if (event->services[i].ota_paras == NULL) {
						PrintfLog(EN_LOG_LEVEL_ERROR, "HandleEventsDown(): there is not enough memory here.\n");
						free(event->services);
						free(event->mqtt_msg_info);
						free(event);
						return;
					}

					if (!strcmp(event_type, VERSION_QUERY)){
						event->services[i].event_type = EN_IOTA_EVENT_VERSION_QUERY;
					} else if (!strcmp(event_type, FIRMWARE_UPGRADE)) {
						event->services[i].event_type = EN_IOTA_EVENT_FIRMWARE_UPGRADE;
					} else if (!strcmp(event_type, SOFTWARE_UPGRADE)) {
						event->services[i].event_type = EN_IOTA_EVENT_SOFTWARE_UPGRADE;
					}

					//firmware_upgrade or software_upgrade
					if (event->services[i].event_type == EN_IOTA_EVENT_FIRMWARE_UPGRADE || event->services[i].event_type == EN_IOTA_EVENT_SOFTWARE_UPGRADE) {
						char *version = JSON_GetStringFromObject(paras, VERSION, NULL);    //get value of version
						event->services[i].ota_paras->version = version;

						char *url = JSON_GetStringFromObject(paras, URL, NULL);    //get value of url
						event->services[i].ota_paras->url = url;

						int file_size = JSON_GetIntFromObject(paras, FILE_SIZE, -1);    //get value of file_size
						event->services[i].ota_paras->file_size = file_size;

						char *access_token = JSON_GetStringFromObject(paras, ACCESS_TOKEN, NULL);    //get value of access_token
						event->services[i].ota_paras->access_token = access_token;

						int expires = JSON_GetIntFromObject(paras, EXPIRES, -1);    //get value of expires
						event->services[i].ota_paras->expires = expires;

						char *sign = JSON_GetStringFromObject(paras, SIGN, NULL);    //get value of sign
						event->services[i].ota_paras->sign = sign;

					}

				}

				//NTP
				if (!strcmp(service_id, SDK_TIME)) {
					event->services[i].servie_id = EN_IOTA_EVENT_TIME_SYNC;
					event->services[i].ntp_paras = (EN_IOTA_NTP_PARAS*)malloc(sizeof(EN_IOTA_NTP_PARAS));

					if (!strcmp(event_type, TIME_SYNC_RSP)){
						event->services[i].event_type = EN_IOTA_EVENT_GET_TIME_SYNC_RESPONSE;

						long long device_send_time = getLLongValueFromStr(message, DEVICE_SEND_TIME_JSON);
						long long server_recv_time = getLLongValueFromStr(message, SERVER_RECV_TIME_JSON);
						long long server_send_time = getLLongValueFromStr(message, SERVER_SEND_TIME_JSON);
						event->services[i].ntp_paras->device_real_time = (server_recv_time + server_send_time + getTime() - device_send_time) / 2;
					}

				}

			}

			i++;
			services_count--;
		}

		if (onEventDown) {
			(onEventDown)(event);
		}

		JSON_Delete(root);

		if (event != NULL) {
			int m;
			for (m = 0;m < services_count_copy;m++) {
				if (event->services[m].servie_id == EN_IOTA_EVENT_SUB_DEVICE_MANAGER) {
					MemFree(&event->services[m].paras->devices);
					MemFree(&event->services[m].paras);
				} else if (event->services[m].servie_id == EN_IOTA_EVENT_OTA) {
					MemFree(&event->services[m].ota_paras);
				} else if (event->services[m].servie_id == EN_IOTA_EVENT_TIME_SYNC) {
					MemFree(&event->services[m].ntp_paras);
				}
			}

			MemFree(&event->services);
			MemFree(&event->mqtt_msg_info);
			MemFree(&event);
		}

		return;
	}
}

void SetLogCallback(LOG_CALLBACK_HANDLER logCallbackHandler) {
    SetPrintfLogCallback(logCallbackHandler);
}

int SetEventCallback(EVENT_CALLBACK_HANDLER pfnCallbackHandler) {
	onEventDown = pfnCallbackHandler;
	return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetCmdCallback(CMD_CALLBACK_HANDLER pfnCallbackHandler) {
	onCmd = pfnCallbackHandler;
	return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetCmdCallbackV3(CMD_CALLBACK_HANDLER_V3 pfnCallbackHandler) {
	onCmdV3 = pfnCallbackHandler;
	return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetProtocolCallback(int item, PROTOCOL_CALLBACK_HANDLER pfnCallbackHandler) {
	switch (item) {
		case EN_CALLBACK_CONNECT_SUCCESS:
			onConnSuccess = pfnCallbackHandler;
			return MqttBase_SetCallback(item, OnLoginSuccess);
		default:
			return MqttBase_SetCallback(item, pfnCallbackHandler);
	}
}

int SetMessageCallback(MESSAGE_CALLBACK_HANDLER pfnCallbackHandler) {
	onMessage = pfnCallbackHandler;
	return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetPropSetCallback(PROP_SET_CALLBACK_HANDLER pfnCallbackHandler) {
	onPropertiesSet = pfnCallbackHandler;
	return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetPropGetCallback(PROP_GET_CALLBACK_HANDLER pfnCallbackHandler) {
	onPropertiesGet = pfnCallbackHandler;
	return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetShadowGetCallback(SHADOW_GET_CALLBACK_HANDLER pfnCallbackHandler) {
	onDeviceShadow = pfnCallbackHandler;
	return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetUserTopicMsgCallback(USER_TOPIC_MSG_CALLBACK_HANDLER pfnCallbackHandler) {
	onUserTopicMessage = pfnCallbackHandler;
	return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

int SetBootstrapCallback(BOOTSTRAP_CALLBACK_HANDLER pfnCallbackHandler) {
	onBootstrap = pfnCallbackHandler;
	return MqttBase_SetCallbackWithTopic(EN_CALLBACK_COMMAND_ARRIVED, OnMessageArrived);
}

