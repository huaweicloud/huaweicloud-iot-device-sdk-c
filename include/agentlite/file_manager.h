#ifndef _FILE_MANAGER_H_
#define _FILE_MANAGER_H_

#include <openssl/ossl_typ.h>
#include "cJSON.h"
#include "hw_type.h"

#define FILE_IP                     "121.36.121.227"
#define FILE_PORT 					 443


typedef struct {
	HW_CHAR *file_name;
	HW_INT file_size;
	HW_CHAR *file_hash_code;
	HW_CHAR *object_device_id;
    HW_CHAR *event_type;
}ST_FILE_MANA_INFO_REPORT;

typedef struct {
	HW_CHAR *object_name;
	HW_INT result_code;
	HW_INT status_code;
	HW_CHAR *event_type;
    HW_CHAR *status_description;
    HW_CHAR *object_device_id;
}ST_FILE_MANA_RES_REPORT;


#define FILE_HTTP_GET				"GET "
#define FILE_HTTP_PUT				"PUT "
#define FILE_HEADER_LENGTH			1x000
#define FILE_IP_URI_LEN				1500
#define OTA_HTTP_HOST				"Host: " 
#define FILE_CONTENT_TYPE			"Content-Type: text/plain\r\n"
#define FILE_CONNECTION				"Connection: keep-alive\r\n"
#define FILE_CONTENT_LENGTH			"Content-Length: "
#define FILE_CONTENT_DISPOSITION	"form-data;name=\"%s\";filname=\"%s\"\r\n"
#define FILE_HTTP_VERSION			" HTTP/1.1\r\n"
#define FILE_LINEFEED				"\r\n"
#define FILE_CRLF					"\r\n\r\n"
#define DOUBLE_OBLIQUE_LINE			"//"
#define SINGLE_SLANT				"/"
#define COLON						":"

#define BUFSIZE						4096
#define HTTP_STATUS_LENGTH			3
#define HTTP_CONTENT_LEN			10
#define OTA_TIMEOUT_MIN_LENGTH		300
#define HTTP_OK						"200"
#define SERVICE_ID					"service_id"
#define OBJECT_DEVICE_ID			"object_device_id"
#define UPLOAD_RESULT_REPORT		"upload_result_report"
#define DOWN_RESULT_REPORT			"download_result_report"
#define EVENT_TYPE					"event_type"
#define EVENT_TIME					"event_time"
#define FILE_NAME_L					"file_name"
#define HASH_CODE					"hash_code"
#define SIZE						"size"
#define OBJECT_NAME					"object_name"
#define RESULT_CODE					"result_code"
#define STATUS_CODE					"status_code"
#define STATUS_DESCRIPTION			"status_description"
#define PARAS						"paras"
#define SERVICES					"services"
#define FILE_SERVICE_ID				"$file_manager"
#define FILE_EVENT_TYPE				"get_upload_url"
#define FILE_EVENT_DOWN				"get_download_url"
#define FILE_EVENT_TYPE_RES			"get_upload_url_response"
#define FILE_EVENT_DOWN_RES			"get_download_url_response"
#define FILE_ATTRIBUTES				"file_attributes"
#define FILE_HTTP_RESPONSE_VERSION	"HTTP/1.1 "

#define FILE_SUCCESS					0
#define FILE_FAILURE					-1
#define FILE_PARAMETER_EMPTY 			-101
#define FILE_RESOURCE_NOT_AVAILABLE		-102
#define FILE_INITIALIZATION_REPEATED	-103
#define FILE_WRITE_FAILED				-104
#define FILE_READ_FAILED				-105
#define FILE_SSL_CONNECT_FAILED			-106
#define FILE_MQTT_CONNECT_EXISTED		-107
#define FILE_CERTIFICATE_NOT_FOUND		-108
#define FILE_MQTT_DISCONNECT_FAILED		-109
#define FILE_PARSE_JSON_FAILED			-110
#define FILE_PARAMETER_ERROR			-111
#define FILE_END						1
#define FILE_OPEN_ERR       			-112

HW_API_FUNC HW_INT FILE_Upload(HW_CHAR *url,HW_CHAR *data ,HW_INT timeout);
HW_API_FUNC HW_INT FiLE_Download(HW_CHAR *url,HW_CHAR *data ,HW_INT timeout);
HW_API_FUNC HW_INT FILE_ReportFile(ST_FILE_MANA_INFO_REPORT *device_info_report, void *context);
static void FILE_FileResponse(int result, HW_CHAR *filePath, HW_INT readOrwrite);
#endif


