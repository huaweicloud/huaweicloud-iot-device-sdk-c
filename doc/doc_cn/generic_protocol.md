目前平台只支持MQTT/HTTP/LWM2M等标准协议接入，如果设备是其他协议（统称为第三方协议），我们推荐使用网关来完成协议转换，将第三方协议转成MQTT协议，从而实现泛协议接入华为物联网平台。原理请参考：<https://support.huaweicloud.com/bestpractice-iothub/iot_bp_0009.html>。
  SDK提供了一个泛协议接入（tcp）的demo，可进入src/gateway_demo文件夹下，其中：
  1. gateway_server_demo.c为泛协议网关demo，主要负责接收设备数据解码并上报到平台，同时可实现命令下发编码并发送到设备。

  2. generic_tcp_protocol.c为tcp协议自定义编解码的规则，这里实现了收到平台下行各种数据自定义编码后发送给设备，以及设备的数据自定义解码后发送到平台。

  3. gateway_client_demo.c为tcp设备（模拟客户端），主要负责上报数据与接收命令，上报的数据根据generic_tcp_protocol.c中定义的解码规则，通过gateway_server_demo.c上报到物联网平台；同时，物联网平台下发的命令通过generic_tcp_protocol.c中定义的编码规则，通过gateway_server_demo.c下发到设备。

  体验步骤：


  1. 将主目录下的产品模型（TTU_0916ca7d2bcd4ec98b2614062550b1c6_TTU01.zip）上传到物联网平台（可参考步骤3.4），并注册设备，保存设备ID和密钥。

  2. 将保存的设备ID和密钥更新到gateway_server_demo.c中（跟device_demo同样的步骤）。

  3. 将主目录下Makefile文件里OBJS中的device_demo.o注释掉，同时将generic_tcp_protocol.o gateway_server_demo.o放开。
    ![](./generic.PNG)
  4. 编译

    make

  5. 编译完成后导入lib

     export LD_LIBRARY_PATH=./lib/   

  6. 运行网关

     ./MQTT_Demo

  7. 进入到src/gateway_demo文件夹下，编译客户端

     gcc -o client.o gateway_client_demo.c

  8. 运行客户端

   ./client.o

  9、上报数据

     运行客户端后，可在客户端控制台输入123，在网关控制台可以看到解码后的数据格式打印，同时在平台能看到上报的数据。可参考generic_tcp_protocol.c中的解码规则（DecodeServiceProperty函数，客户端上报的第一位为service，第二位为该service下的属性1的值，第三位为该service下的属性2的值。命令可参考EncodeCommandParas函数。）。