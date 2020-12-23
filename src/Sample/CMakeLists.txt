list(APPEND SOURCES Main.cpp)
project(imqttsample)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.0)
  # this shows how to use IMqtt as external project with find package

  include(GNUInstallDirs)

  # -DCMAKE_PREFIX_PATH:PATH is set from outside

  find_package(Threads REQUIRED)
  find_package(IMqtt CONFIG REQUIRED)

  link_directories(${CMAKE_PREFIX_PATH}/lib)

  add_executable(${PROJECT_NAME} ${SOURCES})
  target_link_libraries(
    ${PROJECT_NAME} IMqtt::IMqttClient
    # IMqtt::IMqttClientInterface - in case only the abstract interface would be
    # needed without actually linking the library
  )

else()
  # this shows how to use IMqtt as CMake subdirectory
  add_executable(${PROJECT_NAME} ${SOURCES})
  target_link_libraries(
    ${PROJECT_NAME} IMqttClient
    # IMqttClientInterface - in case only the abstract interface would be needed
    # without actually linking the library
  )
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                        COMPONENT Runtime)
