add_executable(imqttsample Main.cpp)

target_link_libraries(
  imqttsample
  PRIVATE MqttClientLibrary
          # Not really needed as part of MqttClientLibrary, but here for demo:
          MqttClientInterface)
