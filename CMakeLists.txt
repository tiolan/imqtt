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

find_package(Threads REQUIRED)

add_subdirectory(src/MqttClient)
add_subdirectory(src/Sample)
