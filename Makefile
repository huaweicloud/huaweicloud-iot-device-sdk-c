CC = gcc
CFLAGS = -g -w -lrt -m64 -Wl,-z,relro,-z,now,-z,noexecstack -fno-strict-aliasing -fstack-protector-all -fno-omit-frame-pointer -pipe -Wall -fPIC -MD -MP -fno-common -freg-struct-return  -fno-inline -fno-exceptions -Wfloat-equal -Wshadow -Wformat=2 -Wextra -rdynamic -Wl,-z,relro,-z,noexecstack  -fstrength-reduce -fsigned-char -ffunction-sections -fdata-sections -Wpointer-arith -Wcast-qual -Waggregate-return -Winline -Wunreachable-code -Wcast-align -Wundef -Wredundant-decls  -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -pie -fPIE -s
# -D _SYS_LOG=1 -shared -fPIC
#-D Linux=1
CXXFLAGS = -O2 -g -Wall -fmessage-length=0 -lrt -m64 -Wl,-z,relro,-z,now,-z,noexecstack -fno-strict-aliasing -fno-omit-frame-pointer -pipe -Wall -fPIC -MD -MP -fno-common -freg-struct-return  -fno-inline -fno-exceptions -Wfloat-equal -Wshadow -Wformat=2 -Wextra -rdynamic -Wl,-z,relro,-z,noexecstack -fstack-protector-strong -fstrength-reduce -fno-builtin -fsigned-char -ffunction-sections -fdata-sections -Wpointer-arith -Wcast-qual -Waggregate-return -Winline -Wunreachable-code -Wcast-align -Wundef -Wredundant-decls  -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs

OBJS = hmac_sha256.o mqtt_base.o log_util.o string_util.o cJSON.o json_util.o base.o callback_func.o login.o subscribe.o data_trans.o iota_init.o iota_login.o iota_datatrans.o device_demo.o mqttv5_util.o
#generic_tcp_protocol.o gateway_server_demo.o
#bootstrap_demo.o
#$(warning "OS $(OS)")
#$(warning "OSTYPE $(OSTYPE)")

HEADER_PATH = -I./include
LIB_PATH = -L./lib
SRC_PATH = ./src

LIBS = $(LIB_PATH) -lpaho-mqtt3as -lssl -lcrypto -lz -lboundscheck
#$(LIB_PATH) -lHWMQTT
#$(LIB_PATH) -lpaho-mqtt3cs $(LIB_PATH)

ifeq ($(OS), Windows_NT)
	TARGET = MQTT_Demo.exe
else 
#	TARGET = libHWMQTT.so
	TARGET = MQTT_Demo.o
endif

$(TARGET):	$(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

##-----------base----------------##
hmac_sha256.o: $(SRC_PATH)/base/hmac_sha256.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/base/hmac_sha256.c -o hmac_sha256.o $(HEADER_PATH)/base/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH)

mqtt_base.o: $(SRC_PATH)/base/mqtt_base.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/base/mqtt_base.c -o mqtt_base.o $(HEADER_PATH)/base/ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/

##-----------util----------------##
log_util.o: $(SRC_PATH)/util/log_util.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/util/log_util.c -o log_util.o $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/
	
string_util.o: $(SRC_PATH)/util/string_util.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/util/string_util.c -o string_util.o $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/zlib/ $(HEADER_PATH)/third_party/libboundscheck/

mqttv5_util.o: $(SRC_PATH)/util/mqttv5_util.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/util/mqttv5_util.c -o mqttv5_util.o $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/zlib/

cJSON.o: $(SRC_PATH)/third_party/cjson/cJSON.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/third_party/cjson/cJSON.c -o cJSON.o $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/agentlite/
	
json_util.o: $(SRC_PATH)/util/json_util.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/util/json_util.c -o json_util.o $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/agentlite/

##-----------servcie----------------##
base.o: $(SRC_PATH)/service/base.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/service/base.c -o base.o $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)/agentlite/
	
callback_func.o: $(SRC_PATH)/service/callback_func.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/service/callback_func.c -o callback_func.o $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)
		
login.o: $(SRC_PATH)/service/login.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/service/login.c -o login.o $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)/agentlite/

subscribe.o: $(SRC_PATH)/service/subscribe.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/service/subscribe.c -o subscribe.o $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/
	
data_trans.o: $(SRC_PATH)/service/data_trans.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/service/data_trans.c -o data_trans.o $(HEADER_PATH)/service/ $(HEADER_PATH)/base/ $(HEADER_PATH)/util/ $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/third_party/cjson/
		

##-----------agentlite----------------##
iota_init.o: $(SRC_PATH)/agentlite/iota_init.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/agentlite/iota_init.c -o iota_init.o $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/

iota_login.o: $(SRC_PATH)/agentlite/iota_login.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/agentlite/iota_login.c -o iota_login.o $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/agentlite/
	
iota_datatrans.o: $(SRC_PATH)/agentlite/iota_datatrans.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/agentlite/iota_datatrans.c -o iota_datatrans.o $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/  $(HEADER_PATH)/third_party/libboundscheck/ $(HEADER_PATH) 

generic_tcp_protocol.o: $(SRC_PATH)/gateway_demo/generic_tcp_protocol.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/gateway_demo/generic_tcp_protocol.c -o generic_tcp_protocol.o $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/protocol/ $(HEADER_PATH)
	
gateway_server_demo.o: $(SRC_PATH)/gateway_demo/gateway_server_demo.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/gateway_demo/gateway_server_demo.c -o gateway_server_demo.o $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH)/protocol/ $(HEADER_PATH)

device_demo.o: $(SRC_PATH)/device_demo/device_demo.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/device_demo/device_demo.c -o device_demo.o $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH) 
	
bootstrap_demo.o: $(SRC_PATH)/bootstrap_demo/bootstrap_demo.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)/bootstrap_demo/bootstrap_demo.c -o bootstrap_demo.o $(HEADER_PATH)/agentlite/ $(HEADER_PATH)/service/ $(HEADER_PATH)/util/ $(HEADER_PATH)/third_party/cjson/ $(HEADER_PATH) 
		
all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) *.d