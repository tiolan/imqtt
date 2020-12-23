# MqttClient interface
add_library(MqttClientInterface INTERFACE)
target_include_directories(MqttClientInterface
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Interface)

# MqttClient library
if(${USE_MOSQ})
  if(${BUILD_SHARED_LIBS})
    set(LIB_MQTT mosquitto_static)
  else()
    set(LIB_MQTT mosquitto)
  endif()
  list(APPEND CLIENT_SOURCES Mosquitto/MosquittoClient.cpp)
endif()

if(${USE_PAHO})
  set(LIB_MQTT paho-mqtt3a)
  list(APPEND CLIENT_SOURCES Paho/PahoClient.cpp)
endif()

list(APPEND CLIENT_SOURCES MqttMessage.cpp)

if(${BUILD_SHARED_LIBS})
  set(SHARED_LIB SHARED)
endif()

add_library(MqttClientLibrary ${SHARED_LIB} ${CLIENT_SOURCES})

if(${EXTERNAL_PROJECT})
  add_dependencies(MqttClientLibrary ${EXTERNAL_PROJECT_NAME})
endif()

target_include_directories(MqttClientLibrary PRIVATE ${MQTT_LIB_PATH}/include)

target_link_libraries(
  MqttClientLibrary
  # PUBLIC
  PUBLIC MqttClientInterface
  # PRIVATE
  PRIVATE Threads::Threads ${LIB_MQTT})

if(MSVC)
  # target_compile_options(MqttClientLibrary PRIVATE /W4 /WX)
else()
  target_compile_options(MqttClientLibrary PRIVATE -Wall -Wextra -Werror)
endif()
