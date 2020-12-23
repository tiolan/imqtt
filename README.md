# IMqtt
IMqtt is a (by far not fully complete) C++ Wrapper around MQTT libraries, written in C.

The idea is to provide an abstract C++ interface to make the underlaying C MQTT library exchangeable quite easy.
Target for now is to support [Paho](https://github.com/eclipse/paho.mqtt.c) and [Mosquitto](https://github.com/eclipse/mosquitto).

# Building
## CMake arguments
| Argument | Description | Default
| - | - | -
| `USE_MOSQ:BOOL` | When set, Mosquitto is used as MQTT library | `OFF`
| `USE_PAHO:BOOL` | When set, Paho is used as MQTT library | `OFF`
| `BUILD_SAMPLE:BOOL` | When set, a sample app "imqttsample" is build | `OFF`
| `BUILD_SHARED_LIBS:BOOL` | When set, IMQTT will be build as shared lib, else as static lib | `OFF`
| `MQTT_LIB_PATH:STRING` | When set, MQTT library binaries will be used from this path, instead of being build as external CMake project | -
| `GIT_TAG:STRING` | When set and `MQTT_LIB_PATH` not set, CMake will clone MQTT library repos from github using provided git tag. When not set, a default tag is used | -
