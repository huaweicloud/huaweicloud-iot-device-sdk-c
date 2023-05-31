CC = gcc
CFLAGS = -g -w -lrt -m64 -Wl,-z,relro,-z,now,-z,noexecstack -fno-strict-aliasing -fstack-protector-all -fno-omit-frame-pointer -pipe -Wall -fPIC -MD -MP -fno-common -freg-struct-return  -fno-inline -fno-exceptions -Wfloat-equal -Wshadow -Wformat=2 -Wextra -rdynamic -Wl,-z,relro,-z,noexecstack  -fstrength-reduce -fsigned-char -ffunction-sections -fdata-sections -Wpointer-arith -Wcast-qual -Waggregate-return -Winline -Wunreachable-code -Wcast-align -Wundef -Wredundant-decls  -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -pie -fPIE -s
# -D _SYS_LOG=1 -shared -fPIC
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

CORE_OBJECTS = hmac_sha256.o mqtt_base.o log_util.o string_util.o cJSON.o json_util.o base.o callback_func.o login.o subscribe.o data_trans.o iota_init.o iota_login.o iota_datatrans.o  mqttv5_util.o
OBJS += $(foreach NAME,$(CORE_OBJECTS),$(OUT_PATH)/$(NAME))
OBJS += $(foreach NAME,$(SYS_HAL_OBJS),$(OUT_PATH)/$(NAME))
OBJS += $(foreach NAME,$(DETECT_ANOMALY_OBJS),$(OUT_PATH)/$(NAME))

DEVICE_RULE_FILENAME = ${wildcard $(SRC_PATH)/service/device_rule/*.c} 
DEVICE_RULE_OBJS := $(DEVICE_RULE_FILENAME:%.c=$(OUT_PATH)/%.o)
OBJS += $(DEVICE_RULE_OBJS)
#generic_tcp_protocol.o gateway_server_demo.o
#bootstrap_demo.o
#$(warning "OS $(OS)")
#$(warning "OSTYPE $(OSTYPE)")


LIBS = $(LIB_PATH) -lpaho-mqtt3as -lssl -lcrypto -lz -lboundscheck -lpthread
#$(LIB_PATH) -lHWMQTT
#$(LIB_PATH) -lpaho-mqtt3cs $(LIB_PATH)

#SOFT_BUS_OPTION2 := 1
ifdef SOFT_BUS_OPTION2
CFLAGS += -DSOFT_BUS_OPTION2=1
OBJS += $(foreach NAME,$(SOFT_BUS_OBJS),$(OUT_PATH)/$(NAME))
endif

#SSH_SWITCH :=1
ifdef SSH_SWITCH
CFLAGS += -DSSH_SWITCH=1
OBJS += $(foreach NAME,$(SSH_OBJS),$(OUT_PATH)/$(NAME))
LIBS += -lnopoll -lssh
endif

# enable device rule compilation
DEVIC_ERULE_ENALBE:=y
CONFIG_ENALBE_DEVICE_RULE_FILE_STORAGE:=y
ifeq ($(DEVIC_ERULE_ENALBE),y)
CFLAGS += -DDEVIC_ERULE_ENALBE=1
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

##-----------agentlite----------------##
$(OUT_PATH)/iota_init.o: $(SRC_PATH)/agentlite/iota_init.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/service/device_rule/ $(HEADER_PATH)/third_party/cjson $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)/third_party/cjson/

$(OUT_PATH)/iota_login.o: $(SRC_PATH)/agentlite/iota_login.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/agentlite/
	
$(OUT_PATH)/iota_datatrans.o: $(SRC_PATH)/agentlite/iota_datatrans.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/  $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)  $(HEADER_PATH)/service/device_rule/

$(OUT_PATH)/generic_tcp_protocol.o: $(SRC_PATH)/gateway_demo/generic_tcp_protocol.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/protocol/ $(HEADER_PATH)
	
$(OUT_PATH)/gateway_server_demo.o: $(SRC_PATH)/gateway_demo/gateway_server_demo.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/protocol/ $(HEADER_PATH)

$(OUT_PATH)/device_demo.o: $(SRC_PATH)/device_demo/device_demo.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)/service/device_rule/   $(HEADER_PATH) $(HEADER_PATH)/tunnel/ $(HEADER_PATH)/nopoll
	
$(OUT_PATH)/bootstrap_demo.o: $(SRC_PATH)/bootstrap_demo/bootstrap_demo.c
	$(CC) $(CFLAGS) -c $< -o $@ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH) 

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
 
##--------------sysHal--------------##
SYS_HAL_SRC = $(SRC_PATH)/service/sys_hal
SYS_HAL_HEADER_PATH = ./include/service/sys_hal
SYS_HAL_INC = -I$(SYS_HAL_HEADER_PATH) $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)/agentlite $(HEADER_PATH)/util/ $(HEADER_PATH)/service/

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

##-----------device rule-------------##
DEVICE_RULE_HEADER_PATH = ./include/service/device_rule/
DEVICE_RULE_INC = -I$(DEVICE_RULE_HEADER_PATH)  $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)/agentlite $(HEADER_PATH)/util/ $(HEADER_PATH)/service/ $(HEADER_PATH)/base/

$(DEVICE_RULE_OBJS):$(OUT_PATH)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(DEVICE_RULE_INC)

.PHONY: all
all:	$(TARGET)


$(OBJ_DIRS):
	mkdir -p $@

clean:
	rm -f $(TARGET)
	rm -rf ./out

