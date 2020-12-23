cmake_minimum_required(VERSION 3.0)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # building not as subdir, so set some compile options
  project(imqtt LANGUAGES CXX C)

  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

option(USE_MOSQ "use mosquitto as the mqtt lib" OFF)
option(USE_PAHO "use paho as the mqtt lib" OFF)
option(BUILD_SAMPLE "build the sample code" OFF)
option(BUILD_SHARED_LIBS "build shared lib" OFF)

if(${USE_PAHO} AND ${USE_MOSQ})
  message(FATAL_ERROR "more than one MQTT lib at a time is not supported")
endif()

find_package(Threads REQUIRED)

if(${USE_MOSQ})
  add_definitions(-DUSE_MOSQ)
  set(GIT_DEFAULT_TAG v2.0.4)
  set(GIT_REPO https://github.com/eclipse/mosquitto.git)
  set(EXTERNAL_PROJECT_NAME mosquitto_external)
  set(EXTERNAL_PROJECT_BUILD_ARGS
      -DWITH_STATIC_LIBRARIES:BOOL=ON
      -DDOCUMENTATION:BOOL=OFF
      -DWITH_THREADING:BOOL=ON
      -DWITH_CLIENTS:BOOL=OFF
      -DWITH_PLUGINS:BOOL=OFF
      -DWITH_APPS:BOOL=OFF
      -DWITH_PIC:BOOL=ON
      -DWITH_TLS:BOOL=OFF # For now
      -DWITH_BROKER:BOOL=OFF)
endif()

if(${USE_PAHO})
  add_definitions(-DUSE_PAHO)
  set(GIT_DEFAULT_TAG v1.3.7)
  set(GIT_REPO https://github.com/eclipse/paho.mqtt.c.git)
  set(EXTERNAL_PROJECT_NAME paho_external)
  set(EXTERNAL_PROJECT_BUILD_ARGS
      -DPAHO_BUILD_SHARED:BOOL=ON
      -DPAHO_BUILD_STATIC:BOOL=ON
      -DPAHO_WITH_SSL:BOOL=ON
      -DPAHO_HIGH_PERFORMANCE:BOOL=ON
      -DPAHO_ENABLE_TESTING:BOOL=OFF
      -DPAHO_ENABLE_CPACK:BOOL=OFF
      -DPAHO_BUILD_DOCUMENTATION:BOOL=OFF)
endif()

if(NOT DEFINED MQTT_LIB_PATH)
  set(EXTERNAL_PROJECT ON)
  if(NOT DEFINED GIT_TAG)
    set(GIT_TAG ${GIT_DEFAULT_TAG})
  endif()
  include(ExternalProject)
  ExternalProject_Add(
    ${EXTERNAL_PROJECT_NAME}
    GIT_REPOSITORY ${GIT_REPO}
    GIT_TAG ${GIT_TAG}
    GIT_CONFIG advice.detachedHead=false
    CMAKE_ARGS ${EXTERNAL_PROJECT_BUILD_ARGS}
               -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>)

  ExternalProject_Get_Property(${EXTERNAL_PROJECT_NAME} INSTALL_DIR)
  set(MQTT_LIB_PATH ${INSTALL_DIR})
endif()

message(STATUS "Searching MQTT lib in: ${MQTT_LIB_PATH}")
link_directories(${MQTT_LIB_PATH}/lib)

add_subdirectory(src/MqttClient)

if(${BUILD_SAMPLE})
  add_subdirectory(src/Sample)
endif()
