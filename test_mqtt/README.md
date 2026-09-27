# test_mqtt — MQTT 客户端

基于 **paho.mqtt.c** 实现的 MQTT 客户端，运行在 i.MX6ULL 开发板上。
连接公共测试服务器 `test.mosquitto.org`，实现 **LED 远程控制** 与 **芯片温度定时上报**。

## 功能

| 功能 | 说明 |
|------|------|
| 连接 Broker | `tcp://test.mosquitto.org:1883`，ClientID 固定为 `zr-arm` |
| 遗嘱消息 | 异常掉线时由 Broker 代发 `Unexpected disconnection` 到 `zr/will` |
| 上线通知 | 连接成功后向 `zr/will` 发布保留消息 `Online` |
| 订阅控制 | 订阅 `zr/led`，按 payload 控制板载 LED |
| 温度上报 | 每 30 秒读取 `/sys/class/thermal/thermal_zone0/temp` 发布到 `zr/temperature` |

### 主题设计

| 主题 | 方向 | Payload | 说明 |
|------|------|---------|------|
| `zr/will` | 发布 | `Online` / `Unexpected disconnection` | 上线状态与遗嘱 |
| `zr/led` | 订阅 | `0` / `1` / `2` | 熄灭 / 常亮 / 呼吸灯 |
| `zr/temperature` | 发布 | 如 `45000` | 芯片温度（毫摄氏度） |

## 目录结构

```text
test_mqtt/
├── CMakeLists.txt              # 构建脚本
├── cmake/
│   └── arm-linux-setup.cmake   # ARM 交叉编译工具链配置
├── mqttClient.c                # 客户端源码
└── MQTT笔记.md                 # MQTT 协议与 paho API 学习笔记
```

## 环境依赖

1. **交叉编译工具链**：`/opt/fsl-imx-x11/4.1.15-2.1.0`（`arm-poky-linux-gnueabi-gcc`，路径在 `cmake/arm-linux-setup.cmake` 中配置）
2. **paho.mqtt.c 库**：需先交叉编译并安装，本项目默认路径为
   `/home/zr-arm/tools/paho.mqtt.c-1.3.8/install`
   （包含 `include/MQTTClient.h` 和 `lib/libpaho-mqtt3c.so*`）

> 路径与自己的环境不一致时，请同时修改 `CMakeLists.txt` 中的 `target_include_directories`
> 和 `target_link_directories`。


## 编译

工程使用 CMake，**必须显式指定工具链文件**，否则会用主机 gcc：

```bash
cd test_mqtt
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-linux-setup.cmake \
      -DCMAKE_BUILD_TYPE=Release ..
make
```

产物：`build/bin/mqttClient`（ARM 32 位 ELF）。

```bash
file bin/mqttClient
# ELF 32-bit LSB executable, ARM, EABI5 ... interpreter /lib/ld-linux-armhf.so.3
```


正常输出：

```text
MQTT服务器连接成功!
```

## 测试方法

在 PC 上安装 mosquitto 客户端工具，即可远程验证：

```bash
# 订阅温度上报与上线状态
mosquitto_sub -h test.mosquitto.org -t 'zr/#' -v

# 控制 LED：0 熄灭 / 1 常亮 / 2 呼吸灯
mosquitto_pub -h test.mosquitto.org -t 'zr/led' -m '1'
mosquitto_pub -h test.mosquitto.org -t 'zr/led' -m '2'
mosquitto_pub -h test.mosquitto.org -t 'zr/led' -m '0'
```
