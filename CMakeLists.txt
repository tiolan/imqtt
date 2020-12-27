cmake_minimum_required(VERSION 3.0)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  # building not as subdir, so set some compile options
  project(
    IMqtt
    VERSION 0.1.0
    DESCRIPTION
      "An abstract interface wrapper written in C++ for MQTT libraries written in C"
    HOMEPAGE_URL https://github.com/tiolan/imqtt
    LANGUAGES CXX C)

  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

option(IMQTT_USE_MOSQ "use mosquitto as the mqtt lib" OFF)
option(IMQTT_USE_PAHO "use paho as the mqtt lib" OFF)
option(IMQTT_BUILD_SAMPLE "build the sample code" OFF)
option(IMQTT_INSTALL "install generated artifacts" OFF)
option(IMQTT_WITH_TLS "enable TLS configurations" OFF)
option(BUILD_SHARED_LIBS "build and link MQTT library as shared lib" OFF)

if(${IMQTT_USE_PAHO} AND ${IMQTT_USE_MOSQ})
  message(FATAL_ERROR "more than one MQTT lib at a time is not supported")
endif()

find_package(Threads REQUIRED)
if(${IMQTT_WITH_TLS})
  find_package(OpenSSL REQUIRED)
endif()
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(ExternalProject)

if(${BUILD_SHARED_LIBS})
  set(BUILD_STATIC_LIBS OFF)
  set(IMQTT_LINKAGE SHARED)
  if(MSVC)
    set(LIB_MQTT_EXT .dll)
  else()
    set(LIB_MQTT_EXT .so)
  endif()
else()
  set(BUILD_STATIC_LIBS ON)
  set(IMQTT_LINKAGE STATIC)
  if(MSVC)
    set(LIB_MQTT_EXT .lib)
  else()
    set(LIB_MQTT_EXT .a)
  endif()
endif()

if(${IMQTT_WITH_TLS})
  add_definitions(-DIMQTT_WITH_TLS)
endif()

if(${IMQTT_USE_MOSQ})
  add_definitions(-DIMQTT_USE_MOSQ)
  set(GIT_DEFAULT_TAG v2.0.4)
  set(GIT_REPO https://github.com/eclipse/mosquitto.git)
  set(EXTERNAL_PROJECT_NAME mosquitto_external)
  set(EXTERNAL_PROJECT_BUILD_ARGS
      -DWITH_STATIC_LIBRARIES:BOOL=${BUILD_STATIC_LIBS}
      -DDOCUMENTATION:BOOL=OFF
      -DWITH_THREADING:BOOL=ON
      -DWITH_CLIENTS:BOOL=OFF
      -DWITH_PLUGINS:BOOL=OFF
      -DWITH_APPS:BOOL=OFF
      -DWITH_PIC:BOOL=ON # TODO: Needed?
      -DWITH_TLS:BOOL=${IMQTT_WITH_TLS}
      -DWITH_BROKER:BOOL=OFF)
  if(${BUILD_STATIC_LIBS})
    set(LIB_MQTT_STATIC _static)
  endif()
  set(LIB_MQTT libmosquitto${LIB_MQTT_STATIC}${LIB_MQTT_EXT})
endif()

if(${IMQTT_USE_PAHO})
  add_definitions(-DIMQTT_USE_PAHO)
  set(GIT_DEFAULT_TAG v1.3.7)
  set(GIT_REPO https://github.com/eclipse/paho.mqtt.c.git)
  set(EXTERNAL_PROJECT_NAME paho_external)
  set(EXTERNAL_PROJECT_BUILD_ARGS
      -DPAHO_BUILD_STATIC:BOOL=${BUILD_STATIC_LIBS}
      -DPAHO_BUILD_SHARED:BOOL=${BUILD_SHARED_LIBS}
      -DPAHO_WITH_SSL:BOOL=${IMQTT_WITH_TLS}
      -DPAHO_HIGH_PERFORMANCE:BOOL=ON
      -DPAHO_ENABLE_TESTING:BOOL=OFF
      -DPAHO_ENABLE_CPACK:BOOL=OFF
      -DPAHO_BUILD_DOCUMENTATION:BOOL=OFF)
  if(${IMQTT_WITH_TLS})
    set(LIB_MQTT_SECURE s)
  endif()
  set(LIB_MQTT libpaho-mqtt3a${LIB_MQTT_SECURE}${LIB_MQTT_EXT})
endif()

if(NOT DEFINED MQTT_LIB_PATH)
  set(EXTERNAL_PROJECT ON)
  if(NOT DEFINED GIT_TAG)
    set(GIT_TAG ${GIT_DEFAULT_TAG})
  endif()
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

set(IMQTT_LIBRARY IMqttClient)
set(IMQTT_INTERFACE IMqttClientInterface)
add_subdirectory(src/MqttClient)

if(${IMQTT_BUILD_SAMPLE})
  add_subdirectory(src/Sample)
endif()

if(${IMQTT_INSTALL})
  install(
    TARGETS ${IMQTT_LIBRARY} ${IMQTT_INTERFACE}
    EXPORT ${PROJECT_NAME}_Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT Development
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime)
  if(${BUILD_SHARED_LIBS})
    install(
      DIRECTORY ${MQTT_LIB_PATH}/lib
      DESTINATION ${CMAKE_INSTALL_PREFIX}
      FILES_MATCHING
      PATTERN "${LIB_MQTT}*"
      PATTERN "pkgconfig" EXCLUDE)
  endif()
  write_basic_package_version_file(
    "${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

  configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

  install(
    EXPORT ${PROJECT_NAME}_Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

  install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
          DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)
endif()

ExternalProject_Add(
  imqttsample_external
  EXCLUDE_FROM_ALL ON
  DOWNLOAD_COMMAND ""
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/Sample
  CMAKE_ARGS -DCMAKE_PREFIX_PATH:PATH=${CMAKE_INSTALL_PREFIX}
             -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX})
