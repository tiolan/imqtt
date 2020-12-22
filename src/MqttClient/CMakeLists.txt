# MqttClient interface
add_library(MqttClientInterface INTERFACE)
target_include_directories(MqttClientInterface
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Interface)

# MqttClient module

if(${USE_MOSQ})
  set(LIB_MQTT libmosquitto)
  set(HEADER_MQTT ${CMAKE_SOURCE_DIR}/thirdparty/mosquitto/lib)
  list(APPEND CLIENT_SOURCES Mosquitto/MosquittoClient.cpp)
endif()

if(${USE_PAHO})
  set(LIB_MQTT paho-mqtt3a)
  set(HEADER_MQTT ${CMAKE_SOURCE_DIR}/thirdparty/paho/src)
  list(APPEND CLIENT_SOURCES Paho/PahoClient.cpp)
endif()

list(APPEND CLIENT_SOURCES MqttMessage.cpp)

add_library(MqttClientLibrary ${CLIENT_SOURCES})

target_link_libraries(
  MqttClientLibrary
  # PUBLIC
  PUBLIC MqttClientInterface
  # PRIVATE
  PRIVATE Threads::Threads ${LIB_MQTT})

target_include_directories(MqttClientLibrary PRIVATE ${HEADER_MQTT})

if(MSVC)
  target_compile_options(MqttClientLibrary PRIVATE /W4 /WX)
else()
  target_compile_options(MqttClientLibrary PRIVATE -Wall -Wextra # -pedantic
                                                   -Werror)
endif()
