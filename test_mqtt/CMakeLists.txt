
cmake_minimum_required(VERSION 2.8.12)
project(MQTTClient C)
message(STATUS "CMake version: " ${CMAKE_VERSION})
message(STATUS "CMake system name: " ${CMAKE_SYSTEM_NAME})
message(STATUS "CMake system processor: " ${CMAKE_SYSTEM_PROCESSOR})

# 设置可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# 定义可执行文件目标
add_executable(mqttClient mqttClient.c)

# 指定MQTT客户端库头文件路径、库路径以及链接库
# MQTT头文件搜索路径
target_include_directories(mqttClient PRIVATE /home/zr-arm/tools/paho.mqtt.c-1.3.8/install/include)# MQTT库文件搜索路径
target_link_directories(mqttClient PRIVATE /home/zr-arm/tools/paho.mqtt.c-1.3.8/install/lib)	
# MQTT链接库 libpaho-mqtt3c.so
target_link_libraries(mqttClient PRIVATE paho-mqtt3c)	



