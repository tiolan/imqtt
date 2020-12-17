# MqttClient interface
add_library(MqttClientInterface INTERFACE)
target_include_directories(MqttClientInterface
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/interface)

# MqttClient module
add_library(MqttClientLibrary MosquittoClient.cpp MqttMessage.cpp)

if(${USE_MOSQ})
  # TODO: also allow the other (static) libs
  set(LIB_MOSQ mosquitto)
endif()

if(${USE_PAHO})
  set(LIB_PAHO "")
endif()

target_link_libraries(
  MqttClientLibrary
  # PUBLIC
  PUBLIC MqttClientInterface
  # PRIVATE
  PRIVATE Threads::Threads ${LIB_MOSQ} ${LIB_PAHO})
