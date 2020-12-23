# IMqtt
IMqtt is a (by far not fully complete) C++ Wrapper around MQTT libraries, written in C.

The idea is to provide an abstract C++ interface to make the underlaying C MQTT library exchangeable quite easy.
Target for now is to support [Paho](https://github.com/eclipse/paho.mqtt.c) and [Mosquitto](https://github.com/eclipse/mosquitto).

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
| `IMQTT_BUILD_SAMPLE:BOOL`   | When set, a sample app `imqttsample` is built as CMake subdirectory                                                                               | `OFF`   |
| `IMQTT_INSTALL:BOOL`        | When set, target `install` will install artifacts to `CMAKE_INSTALL_PREFIX`                                                                       | `OFF`   |
| `BUILD_SHARED_LIBS:BOOL`    | When set, IMQTT will be built as shared lib and also the MQTT lib will be linked as shared lib, else as static libs                               | `OFF`   |
| `MQTT_LIB_PATH:STRING`      | When set, MQTT library binaries will be used from this path, instead of being built as external CMake project                                     | -       |
| `GIT_TAG:STRING`            | When set and `MQTT_LIB_PATH` not set, CMake will clone MQTT library repos from github using provided git tag. When not set, a default tag is used | -       |
| `CMAKE_INSTALL_PREFIX:PATH` | When `IMQTT_INSTALL` is set, artifacts will be installed to this path                                                                             | -       |

# Usage
An example app can be found here: [Main.cpp](src/Sample/Main.cpp)
## CMake find_package
There is a CMake target `imqttsample_external` provided, that builds the example app using IMqtt with CMake `find_package` in this [CMakeLists.txt](src/Sample/CMakeLists.txt).
## CMake subdirectory
When building with `-DIMQTT_BUILD_SAMPLE:BOOL=ON` the provided sample is built as CMake subdirectory, see [CMakeLists.txt](src/Sample/CMakeLists.txt).
In order to make it find the CMake package IMqtt, the target install has to be built already.

# Maturity
IMqtt is in development. It is not ready for production. There are no tests done yet and no testing-framework is setup currently.