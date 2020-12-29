# MqttClient interface
add_library(${IMQTT_INTERFACE} INTERFACE)
target_include_directories(
  ${IMQTT_INTERFACE}
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Interface>
            $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)

set(IMQTT_INTERFACE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface/IMqttClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface/IMqttClientCallbacks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface/IMqttMessage.h)

# target_sources(${IMQTT_INTERFACE} INTERFACE
# $<BUILD_INTERFACE:${IMQTT_INTERFACE_HEADERS}>)

# MqttClient library
if(${IMQTT_USE_MOSQ})
  list(APPEND CLIENT_SOURCES Mosquitto/MosquittoClient.cpp)
endif()

if(${IMQTT_USE_PAHO})
  list(APPEND CLIENT_SOURCES Paho/PahoClient.cpp)
endif()

list(APPEND CLIENT_SOURCES MqttMessage.cpp IMqttClient.cpp)

add_library(${IMQTT_LIBRARY} ${IMQTT_LINKAGE} ${CLIENT_SOURCES})
set_target_properties(${IMQTT_LIBRARY} PROPERTIES PUBLIC_HEADER
                                                  "${IMQTT_INTERFACE_HEADERS}")
if(${EXTERNAL_PROJECT})
  add_dependencies(${IMQTT_LIBRARY} ${EXTERNAL_PROJECT_NAME})
endif()

target_include_directories(${IMQTT_LIBRARY} PRIVATE ${LIB_MQTT_PATH}/include)

target_link_libraries(
  ${IMQTT_LIBRARY}
  # PUBLIC
  PUBLIC ${IMQTT_INTERFACE}
  # PRIVATE
  PRIVATE Threads::Threads ${LIB_MQTT})

if(${IMQTT_WITH_TLS})
  target_link_libraries(${IMQTT_LIBRARY} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(MSVC)
  # target_compile_options(${IMQTT_LIBRARY} PRIVATE /W4 /WX)
else()
  target_compile_options(${IMQTT_LIBRARY} PRIVATE -Wall -Wextra -Werror)
endif()
