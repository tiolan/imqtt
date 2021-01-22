# IMqtt
IMqtt is a (by far not fully complete) C++ wrapper around existing MQTT libraries. It is desgined to use the MQTTv5 protocol only.

It provides an abstract C++ interface IMqttClient and hides the underlying MQTT library, such that it is exchangeable quite easy (depending on the functions used). It furthermore provides a C++ wrapper around the C interfaces of common MQTT libraries.
Target for now is to support [Paho](https://github.com/eclipse/paho.mqtt.c) (written in C) and [Mosquitto](https://github.com/eclipse/mosquitto) (written in C).
IMqtt is heavily based on callback interfaces. The user has to implement those callback interfaces and hand them over to an object of IMqttClient. Via those callbacks, messages and other status information are provided to the user.

In order to decouple the callbacks of the underlying MQTT library and the (potentially long-lasting) MQTT message processing done by the user, an optional FIFO-like IDispatchQueue is provided.

# API Reference
The API Reference can be found here: https://tiolan.github.io/imqtt/index.html

# Status
[![Build Status](https://www.travis-ci.com/tiolan/imqtt.svg?branch=master)](https://www.travis-ci.com/tiolan/imqtt)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=tiolan_imqtt&metric=alert_status)](https://sonarcloud.io/dashboard?id=tiolan_imqtt)

# Maturity
IMqtt is in development.
- It is not ready for production.
- There are no (automated) tests done yet and no testing-framework is setup currently.
- There are no releases done regularly (yet), take master as is.

In case the community is interested, the project might be driven further.

## Currently Supported:
- MQTTv5 user fields for publishing messages
- MQTTv5 correlationData field for publishing messages
- MQTTv5 payloadType field for publishing messages
- TLS support (details depend on used MQTT lib)
- Asynchronous interface with non-blocking calls
- Exponential backoff with randomized delay (details depend on used MQTT lib)
## Currently Not Supported:
- TLS-PSK
- Other than mentioned MQTTv5 fields
- MQTTv5 fields for messages other than publish
- Synchronous interface with blocking calls

# Building
## Example
~~~
mkdir build
cd build
cmake -DIMQTT_USE_MOSQ:BOOL=ON -DIMQTT_INSTALL:BOOL=ON -DCMAKE_INSTALL_PREFIX:PATH=../install -DIMQTT_BUILD_SAMPLE:BOOL=ON ..
make -j$(nproc) install
~~~
## CMake arguments for building IMqtt
| Argument                    | Description                                                                                                                                       | Default |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- | ------- |
| `IMQTT_USE_MOSQ:BOOL`       | When set, Mosquitto is used as MQTT library                                                                                                       | `OFF`   |
| `IMQTT_USE_PAHO:BOOL`       | When set, Paho is used as MQTT library                                                                                                            | `OFF`   |
| `IMQTT_WITH_TLS:BOOL`       | When set, TLS configuration options are provided and MQTT lib can be configured to establish TLS connections                                      | `OFF`   |
| `IMQTT_BUILD_SAMPLE:BOOL`   | When set, a sample app `imqttsample` is built as CMake subdirectory                                                                               | `OFF`   |
| `IMQTT_INSTALL:BOOL`        | When set, target `install` will install artifacts to `CMAKE_INSTALL_PREFIX`                                                                       | `OFF`   |
| `BUILD_SHARED_LIBS:BOOL`    | When set, IMQTT will be built as shared lib and also the MQTT lib will be linked as shared lib, else as static libs                               | `OFF`   |
| `LIB_MQTT_PATH:STRING`      | When set, MQTT library binaries will be used from this path, instead of being built as external CMake project                                     | -       |
| `GIT_TAG:STRING`            | When set and `LIB_MQTT_PATH` not set, CMake will clone MQTT library repos from github using provided git tag. When not set, a default tag is used | -       |
| `CMAKE_INSTALL_PREFIX:PATH` | When `IMQTT_INSTALL` is set, artifacts will be installed to this path                                                                             | -       |

# Usage
An example app can be found here: [Main.cpp](src/Sample/Main.cpp).
When building with `-DIMQTT_BUILD_SAMPLE:BOOL=ON` the provided sample is built as CMake subdirectory.
## CMake find_package
There is a CMake target `imqttsample_external` provided, that builds the example app using IMqtt with CMake `find_package` in this [CMakeLists.txt](src/Sample/CMakeLists.txt).

In order to make it find the CMake package IMqtt, the target `install` has to be built already.

## CMake subdirectory
Add Imqtt as CMake subdirectory using `add_subdirectory` and `target_link_libraries(${PROJECT_NAME} PRIVATE IMqttClient)` or `target_link_libraries(${PROJECT_NAME} PRIVATE IMqttClientInterface)` as shown here: [CMakeLists.txt](src/Sample/CMakeLists.txt).
