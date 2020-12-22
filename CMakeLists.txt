cmake_minimum_required(VERSION 3.9)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # building not as subdir, so set some compile options
  project(imqtt LANGUAGES CXX C)

  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

option(USE_MOSQ "use mosquitto as the mqtt lib" OFF)
option(USE_PAHO "use paho as the mqtt lib" OFF)
option(BUILD_SAMPLE "build the sample code" ON)

if(${USE_PAHO} AND ${USE_MOSQ})
  message(FATAL_ERROR "more than one MQTT lib at a time is not supported")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Threads REQUIRED)

if(${USE_MOSQ})
  include(buildMosquitto)
  add_definitions(-DUSE_MOSQ)
endif()

if(${USE_PAHO})
  include(buildPaho)
  add_definitions(-DUSE_PAHO)
endif()

add_subdirectory(src/MqttClient)
if(${BUILD_SAMPLE})
  add_subdirectory(src/Sample)
endif()
