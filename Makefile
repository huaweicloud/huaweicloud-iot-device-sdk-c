CC = gcc
CFLAGS = -g -w -lrt -m64 -Wl,-z,relro,-z,now,-z,noexecstack -fno-strict-aliasing -fstack-protector-all -fno-omit-frame-pointer -pipe -Wall -fPIC -MD -MP -fno-common -freg-struct-return  -fno-inline -fno-exceptions -Wfloat-equal -Wshadow -Wformat=2 -Wextra -rdynamic -Wl,-z,relro,-z,noexecstack  -fstrength-reduce -fsigned-char -ffunction-sections -fdata-sections -Wpointer-arith -Wcast-qual -Waggregate-return -Winline -Wunreachable-code -Wcast-align -Wundef -Wredundant-decls  -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -pie -fPIE -s
#-D _SYS_LOG=1 -shared -fPIC
#-D Linux=1
CXXFLAGS = -O2 -g -Wall -fmessage-length=0 -lrt -m64 -Wl,-z,relro,-z,now,-z,noexecstack -fno-strict-aliasing -fno-omit-frame-pointer -pipe -Wall -fPIC -MD -MP -fno-common -freg-struct-return  -fno-inline -fno-exceptions -Wfloat-equal -Wshadow -Wformat=2 -Wextra -rdynamic -Wl,-z,relro,-z,noexecstack -fstack-protector-strong -fstrength-reduce -fno-builtin -fsigned-char -ffunction-sections -fdata-sections -Wpointer-arith -Wcast-qual -Waggregate-return -Winline -Wunreachable-code -Wcast-align -Wundef -Wredundant-decls  -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs

OUT_PATH=./out
HEADER_PATH = -I./include
LIB_PATH = -L./lib
SRC_PATH = ./src

SSH_OBJS = wss_client.o ssh_client.o
SYS_HAL_OBJS = sys_hal.o sys_hal_imp.o
DETECT_ANOMALY_OBJS = detect_anomaly.o
SOFT_BUS_OBJS = dconncaseone_interface.o soft_bus_datatrans.o soft_bus_init.o

CORE_OBJECTS = hmac_sha256.o mqtt_base.o log_util.o string_util.o cJSON.o json_util.o base.o callback_func.o login.o subscribe.o data_trans.o iota_init.o iota_login.o iota_datatrans.o iota_defaultCallback.o iota_payload.o
OBJS += $(foreach NAME,$(CORE_OBJECTS),$(OUT_PATH)/$(NAME))
OBJS += $(foreach NAME,$(SYS_HAL_OBJS),$(OUT_PATH)/$(NAME))
OBJS += $(foreach NAME,$(DETECT_ANOMALY_OBJS),$(OUT_PATH)/$(NAME))

DEVICE_RULE_FILENAME = ${wildcard $(SRC_PATH)/service/device_rule/*.c} 
DEVICE_RULE_OBJS := $(DEVICE_RULE_FILENAME:%.c=$(OUT_PATH)/%.o)
OBJS += $(DEVICE_RULE_OBJS)
#$(warning "OS $(OS)")
#$(warning "OSTYPE $(OSTYPE)")


LIBS = $(LIB_PATH) -lpaho-mqtt3as -lssl -lcrypto -lz -lboundscheck -lpthread -lcurl -lm
#$(LIB_PATH) -lHWMQTT
#$(LIB_PATH) -lpaho-mqtt3cs $(LIB_PATH)

#SOFT_BUS_OPTION2 := 1
ifdef SOFT_BUS_OPTION2
CFLAGS += -DSOFT_BUS_OPTION2=1
OBJS += $(foreach NAME,$(SOFT_BUS_OBJS),$(OUT_PATH)/$(NAME))
endif

GLOBAL_VAR_CONFIG := 1
ifdef GLOBAL_VAR_CONFIG
CFLAGS += -DGLOBAL_VAR_CONFIG=1
endif

BRIDGE_SWITCH := 1
ifdef BRIDGE_SWITCH
BRIDGE_OBJS = iota_bridge.o bridge_topic_data.o
OBJS += $(foreach NAME,$(BRIDGE_OBJS),$(OUT_PATH)/$(NAME))
endif

#SSH_SWITCH := 1
ifdef SSH_SWITCH
CFLAGS += -DSSH_SWITCH=1
OBJS += $(foreach NAME,$(SSH_OBJS),$(OUT_PATH)/$(NAME))
LIBS += -lnopoll -lssh
endif

#MQTTV5 := 1
ifdef MQTTV5
CFLAGS += -DMQTTV5
OBJS +=  $(OUT_PATH)/mqttv5_util.o
endif

#SECURITY_AWARENESS_ENABLE := 1
ifdef SECURITY_AWARENESS_ENABLE
CFLAGS += -DSECURITY_AWARENESS_ENABLE
OBJS +=  $(OUT_PATH)/report_anomaly.o
endif

# enable device rule compilation
#DEVICE_RULE_ENALBE := y
#CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE := y
ifeq ($(DEVICE_RULE_ENALBE),y)
CFLAGS += -DDEVICE_RULE_ENALBE=1
ifeq ($(CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE),y)
CFLAGS += -DCONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE=1
endif
endif

OBJ_DIRS += $(dir $(OBJS))
OBJ_DIRS := $(sort $(OBJ_DIRS))

ifeq ($(OS), Windows_NT)
	TARGET = MQTT_Demo.exe
else 
#	TARGET = libHWMQTT.so
	TARGET = MQTT_Demo
endif

$(TARGET): $(OBJ_DIRS) $(OUT_PATH)/device_demo.o $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OUT_PATH)/device_demo.o $(OBJS)  $(LIBS)

gateway_server_test: $(OBJ_DIRS) $(OUT_PATH)/generic_tcp_protocol.o $(OUT_PATH)/gateway_server_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/generic_tcp_protocol.o $(OUT_PATH)/gateway_server_test.o $(OBJS) $(LIBS)

gateway_client_test: $(OBJ_DIRS) $(OUT_PATH)/generic_tcp_protocol.o $(OUT_PATH)/gateway_client_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/generic_tcp_protocol.o $(OUT_PATH)/gateway_client_test.o $(OBJS) $(LIBS)

bridge_server_test: $(OBJ_DIRS) $(OUT_PATH)/bridge_tcp_protocol.o $(OUT_PATH)/bridge_server_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/bridge_tcp_protocol.o $(OUT_PATH)/bridge_server_test.o $(OBJS) $(LIBS)

bridge_client_test: $(OBJ_DIRS) $(OUT_PATH)/bridge_tcp_protocol.o $(OUT_PATH)/bridge_client_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/bridge_tcp_protocol.o $(OUT_PATH)/bridge_client_test.o $(OBJS) $(LIBS)

bootstrap_test: $(OBJ_DIRS) $(OUT_PATH)/bootstrap.o $(OUT_PATH)/bootstrap_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/bootstrap.o  $(OUT_PATH)/bootstrap_test.o $(OBJS)  $(LIBS)

basic_test: $(OBJ_DIRS) $(OUT_PATH)/basic_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/basic_test.o $(OBJS)  $(LIBS)
	
bootstrap_groups_test: $(OBJ_DIRS) $(OUT_PATH)/bootstrap.o $(OUT_PATH)/bootstrap_groups_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/bootstrap.o $(OUT_PATH)/bootstrap_groups_test.o $(OBJS)  $(LIBS)

mqttV5_test: $(OBJ_DIRS) $(OUT_PATH)/mqttV5_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/mqttV5_test.o $(OBJS)  $(LIBS)

message_test: $(OBJ_DIRS) $(OUT_PATH)/message_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/message_test.o $(OBJS)  $(LIBS)

properties_test: $(OBJ_DIRS) $(OUT_PATH)/properties_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/properties_test.o $(OBJS)  $(LIBS)

command_test: $(OBJ_DIRS) $(OUT_PATH)/command_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/command_test.o $(OBJS)  $(LIBS)

shadow_test: $(OBJ_DIRS) $(OUT_PATH)/shadow_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/shadow_test.o $(OBJS)  $(LIBS)

time_sync_test: $(OBJ_DIRS) $(OUT_PATH)/time_sync_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/time_sync_test.o $(OBJS)  $(LIBS)

ota_test: $(OBJ_DIRS) $(OUT_PATH)/ota_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/ota_test.o $(OBJS)  $(LIBS)

report_device_info_test: $(OBJ_DIRS) $(OUT_PATH)/report_device_info_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/report_device_info_test.o $(OBJS)  $(LIBS)
	
log_report_test: $(OBJ_DIRS) $(OUT_PATH)/log_report_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/log_report_test.o $(OBJS)  $(LIBS)

file_up_down_test: $(OBJ_DIRS) $(OUT_PATH)/file_up_down_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/file_up_down_test.o $(OBJS)  $(LIBS)

device_rule_test: $(OBJ_DIRS) $(OUT_PATH)/device_rule_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/device_rule_test.o $(OBJS)  $(LIBS)
	
sys_hal_test: $(OBJ_DIRS) $(OUT_PATH)/sys_hal_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/sys_hal_test.o $(OBJS)  $(LIBS)

device_config_test: $(OBJ_DIRS) $(OUT_PATH)/device_config_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/device_config_test.o $(OBJS)  $(LIBS)

remote_login_test: $(OBJ_DIRS) $(OUT_PATH)/remote_login_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/remote_login_test.o $(OBJS)  $(LIBS)

reconnection_test: $(OBJ_DIRS) $(OUT_PATH)/reconnection_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/reconnection_test.o $(OBJS)  $(LIBS)

soft_bus_test: $(OBJ_DIRS) $(OUT_PATH)/soft_bus_test.o $(OUT_PATH)/soft_bus_data_process.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/soft_bus_test.o $(OUT_PATH)/soft_bus_data_process.o $(OBJS)  $(LIBS)

report_anomaly_test: $(OBJ_DIRS) $(OUT_PATH)/report_anomaly_test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OUT_PATH)/report_anomaly_test.o $(OBJS)  $(LIBS)
	
##-----------base----------------##
$(OUT_PATH)/hmac_sha256.o: $(SRC_PATH)/base/hmac_sha256.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/base/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)

$(OUT_PATH)/mqtt_base.o: $(SRC_PATH)/base/mqtt_base.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/base/ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/libboundscheck/

##-----------util----------------##
$(OUT_PATH)/log_util.o: $(SRC_PATH)/util/log_util.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/
	
$(OUT_PATH)/string_util.o: $(SRC_PATH)/util/string_util.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/zlib/ $(HEADER_PATH)/third_party/libboundscheck/

$(OUT_PATH)/mqttv5_util.o: $(SRC_PATH)/util/mqttv5_util.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/zlib/

$(OUT_PATH)/cJSON.o: $(SRC_PATH)/third_party/cjson/cJSON.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/agentlite/
	
$(OUT_PATH)/json_util.o: $(SRC_PATH)/util/json_util.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/agentlite/

##-----------servcie----------------##
$(OUT_PATH)/base.o: $(SRC_PATH)/service/base.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/
	
$(OUT_PATH)/callback_func.o: $(SRC_PATH)/service/callback_func.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH) $(HEADER_PATH)/service/device_rule/ $(HEADER_PATH)/service/detect_anomaly/ $(HEADER_PATH)/service/sys_hal/ $(HEADER_PATH)/third_party/libboundscheck/
		
$(OUT_PATH)/login.o: $(SRC_PATH)/service/login.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/

$(OUT_PATH)/subscribe.o: $(SRC_PATH)/service/subscribe.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/cjson/
	
$(OUT_PATH)/data_trans.o: $(SRC_PATH)/service/data_trans.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/service/device_rule/ $(HEADER_PATH)/third_party/libboundscheck/

$(OUT_PATH)/bridge_topic_data.o: $(SRC_PATH)/service/bridge/bridge_topic_data.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/service/ $(HEADER_PATH)/base/  $(HEADER_PATH)/service/bridge/ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/service/device_rule/ $(HEADER_PATH)/third_party/libboundscheck/


##-----------agentlite----------------##
$(OUT_PATH)/iota_init.o: $(SRC_PATH)/agentlite/iota_init.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/service/device_rule/ $(HEADER_PATH)/third_party/cjson $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)/third_party/cjson/

$(OUT_PATH)/iota_login.o: $(SRC_PATH)/agentlite/iota_login.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/agentlite/
	
$(OUT_PATH)/iota_datatrans.o: $(SRC_PATH)/agentlite/iota_datatrans.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/  $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)  $(HEADER_PATH)/service/device_rule/

$(OUT_PATH)/iota_defaultCallback.o: $(SRC_PATH)/agentlite/iota_defaultCallback.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/  $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)  $(HEADER_PATH)/service/device_rule/ $(HEADER_PATH)/tunnel/  $(HEADER_PATH)/nopoll/

$(OUT_PATH)/iota_payload.o: $(SRC_PATH)/agentlite/iota_payload.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/  $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH) $(HEADER_PATH)/service/device_rule/
$(OUT_PATH)/iota_bridge.o: $(SRC_PATH)/agentlite/iota_bridge.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/bridge/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/  $(HEADER_PATH)/base $(HEADER_PATH)/service/  $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)  

#-----------demo-------------##
$(OUT_PATH)/generic_tcp_protocol.o: $(SRC_PATH)/../demos/gateway_demo/generic_tcp_protocol.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ -I./demos/gateway_demo/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)
	
$(OUT_PATH)/gateway_server_test.o: $(SRC_PATH)/../demos/gateway_demo/gateway_server_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ -I./demos/gateway_demo/ $(HEADER_PATH)/third_party/cjson/  $(HEADER_PATH) 

$(OUT_PATH)/gateway_client_test.o: $(SRC_PATH)/../demos/gateway_demo/gateway_client_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/util/ $(HEADER_PATH)

$(OUT_PATH)/device_demo.o: $(SRC_PATH)/device_demo/device_demo.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)/service/device_rule/  $(HEADER_PATH) $(HEADER_PATH)/tunnel/ $(HEADER_PATH)/nopoll
	
##--------------bridge--------------##
$(OUT_PATH)/bridge_tcp_protocol.o: $(SRC_PATH)/../demos/bridge_demo/bridge_tcp_protocol.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ -I./demos/bridge_demo/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)
	
$(OUT_PATH)/bridge_server_test.o: $(SRC_PATH)/../demos/bridge_demo/bridge_server_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ -I./demos/bridge_demo/ $(HEADER_PATH)/third_party/cjson/  $(HEADER_PATH) 

$(OUT_PATH)/bridge_client_test.o: $(SRC_PATH)/../demos/bridge_demo/bridge_client_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/util/ $(HEADER_PATH)

##--------------tunnel--------------##
$(OUT_PATH)/wss_client.o: $(SRC_PATH)/tunnel/wss_client.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/nopoll/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/util/ $(HEADER_PATH)/tunnel/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH) $(HEADER_PATH)/third_party/libboundscheck/
$(OUT_PATH)/ssh_client.o: $(SRC_PATH)/tunnel/ssh_client.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/nopoll/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/util/ $(HEADER_PATH)/tunnel/ $(HEADER_PATH)/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/libssh/ $(HEADER_PATH)/third_party/libboundscheck/

##-----------soft bus----------------##
$(OUT_PATH)/soft_bus_datatrans.o: $(SRC_PATH)/service/soft_bus_datatrans.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/base/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/util/ $(HEADER_PATH)/service/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/soft_bus_init.o: $(SRC_PATH)/service/soft_bus_init.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/base/ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)

$(OUT_PATH)/dconncaseone_interface.o: $(SRC_PATH)/dconncaseone_interface.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/

##--------------bootstrap test----------------##
$(OUT_PATH)/bootstrap.o: $(SRC_PATH)/../demos/bootstrap_demo/bootstrap.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ -I./demos/bootstrap_demo/ $(HEADER_PATH)

$(OUT_PATH)/bootstrap_test.o: $(SRC_PATH)/../demos/bootstrap_demo/bootstrap_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ -I./demos/bootstrap_demo/ $(HEADER_PATH)

$(OUT_PATH)/bootstrap_groups_test.o: $(SRC_PATH)/../demos/bootstrap_demo/bootstrap_groups_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/base/ -I./demos/bootstrap_demo/ $(HEADER_PATH)

##--------------device test----------------##
$(OUT_PATH)/basic_test.o: $(SRC_PATH)/../demos/device_demo/basic_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/mqttV5_test.o: $(SRC_PATH)/../demos/device_demo/mqttV5_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/message_test.o: $(SRC_PATH)/../demos/device_demo/message_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/properties_test.o: $(SRC_PATH)/../demos/device_demo/properties_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/command_test.o: $(SRC_PATH)/../demos/device_demo/command_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/shadow_test.o: $(SRC_PATH)/../demos/device_demo/shadow_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/time_sync_test.o: $(SRC_PATH)/../demos/device_demo/time_sync_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/ota_test.o: $(SRC_PATH)/../demos/device_demo/ota_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/report_device_info_test.o: $(SRC_PATH)/../demos/device_demo/report_device_info_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/report_anomaly_test.o: $(SRC_PATH)/../demos/device_demo/report_anomaly_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/service/detect_anomaly/  $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/log_report_test.o: $(SRC_PATH)/../demos/device_demo/log_report_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/file_up_down_test.o: $(SRC_PATH)/../demos/device_demo/file_up_down_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/device_rule_test.o: $(SRC_PATH)/../demos/device_demo/device_rule_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)

$(OUT_PATH)/sys_hal_test.o: $(SRC_PATH)/../demos/device_demo/sys_hal_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/service/sys_hal $(HEADER_PATH)

$(OUT_PATH)/device_config_test.o: $(SRC_PATH)/../demos/device_demo/device_config_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/service/sys_hal $(HEADER_PATH)

$(OUT_PATH)/remote_login_test.o: $(SRC_PATH)/../demos/device_demo/remote_login_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/service/sys_hal $(HEADER_PATH)/tunnel/ $(HEADER_PATH)/nopoll $(HEADER_PATH)

$(OUT_PATH)/reconnection_test.o: $(SRC_PATH)/../demos/device_demo/reconnection_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/service/sys_hal $(HEADER_PATH)

##--------------softBus demo ----------------##
ifdef SOFT_BUS_OPTION2
$(OUT_PATH)/soft_bus_test.o: $(SRC_PATH)/../demos/softBus_demo/soft_bus_test.c
	$(CC) $(CFLAGS) -c $< -o $@ $(SRC_PATH)/../demos/softBus_demo/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/service/sys_hal $(HEADER_PATH)

$(OUT_PATH)/soft_bus_data_process.o: $(SRC_PATH)/../demos/softBus_demo/soft_bus_data_process.c
	$(CC) $(CFLAGS) -c $< -o $@ $(SRC_PATH)/../demos/softBus_demo/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/service/sys_hal $(HEADER_PATH)
endif
##--------------sysHal--------------##
SYS_HAL_SRC = $(SRC_PATH)/service/sys_hal
SYS_HAL_HEADER_PATH = ./include/service/sys_hal
SYS_HAL_INC = -I$(SYS_HAL_HEADER_PATH) $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)/agentlite $(HEADER_PATH)/util/ $(HEADER_PATH)/service

$(OUT_PATH)/sys_hal_imp.o: $(SYS_HAL_SRC)/sys_hal_imp.c
	$(CC) $(CFLAGS) -c $< -o $@ $(SYS_HAL_INC)
$(OUT_PATH)/sys_hal.o: $(SYS_HAL_SRC)/sys_hal.c
	$(CC) $(CFLAGS) -c $< -o $@ $(SYS_HAL_INC)

##--------anomaly detection---------##
DETECT_ANOMOLY_SRC = $(SRC_PATH)/service/detect_anomaly
DETECT_ANOMOLY_HEADER_PATH = ./include/service/detect_anomaly
DETECT_ANOMOLY_INC = -I$(DETECT_ANOMOLY_HEADER_PATH) $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)/agentlite $(HEADER_PATH)/util/ $(HEADER_PATH)/service/ $(HEADER_PATH)/service/sys_hal/

$(OUT_PATH)/detect_anomaly.o: $(DETECT_ANOMOLY_SRC)/detect_anomaly.c $(OUT_PATH)/sys_hal_imp.o $(OUT_PATH)/sys_hal.o
	$(CC) $(CFLAGS) -c $< -o $@ $(DETECT_ANOMOLY_INC) $(HEADER_PATH)

$(OUT_PATH)/report_anomaly.o: $(DETECT_ANOMOLY_SRC)/report_anomaly.c $(OUT_PATH)/sys_hal_imp.o $(OUT_PATH)/sys_hal.o
	$(CC) $(CFLAGS) -c $< -o $@ $(DETECT_ANOMOLY_INC) $(HEADER_PATH)

##-----------device rule-------------##
DEVICE_RULE_HEADER_PATH = ./include/service/device_rule/
DEVICE_RULE_INC = -I$(DEVICE_RULE_HEADER_PATH)  $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)/agentlite $(HEADER_PATH)/util/ $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)

$(DEVICE_RULE_OBJS):$(OUT_PATH)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(DEVICE_RULE_INC)

BOOTSTREP_TEST = bootstrap_test bootstrap_groups_test
DEVICE_TEST = basic_test mqttV5_test message_test properties_test command_test shadow_test time_sync_test ota_test report_device_info_test report_anomaly_test log_report_test file_up_down_test device_rule_test sys_hal_test device_config_test remote_login_test reconnection_test
GATEWAY_TEST = gateway_client_test gateway_server_test
BRIDGE_TEST = bridge_client_test bridge_server_test
SOFT_BUS_TEST = 
ifdef SOFT_BUS_OPTION2
SOFT_BUS_TEST += soft_bus_test
endif

.PHONY: all
all:	$(TARGET) $(BOOTSTREP_TEST) $(DEVICE_TEST) $(BRIDGE_TEST) $(GATEWAY_TEST) $(SOFT_BUS_TEST)


.PHONY: bootstrap_demo
bootstrap_demo: $(BOOTSTREP_TEST)

.PHONY: device_demo
device_demo: $(DEVICE_TEST)

.PHONY: gateway_demo
gateway_demo: $(GATEWAY_TEST)

.PHONY: bridge_demo
bridge_demo: $(BRIDGE_TEST)

.PHONY: softBus_demo
softBus_demo: $(SOFT_BUS_TEST)

.PHONY: test
test: $(TARGET) $(BOOTSTREP_TEST) $(DEVICE_TEST) $(GATEWAY_TEST) $(SOFT_BUS_TEST)

$(OBJ_DIRS):
	mkdir -p $@

clean:
	rm -f $(TARGET) $(BOOTSTREP_TEST) $(DEVICE_TEST) $(GATEWAY_TEST) $(BRIDGE_TEST) $(SOFT_BUS_TEST)
	rm -f bootstrap_demo
	rm -rf ./out

