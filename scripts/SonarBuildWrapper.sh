#!/bin/bash
set -e

NUMBER_OF_PROCESSORS=$(nproc --all)

PAHO_BUILD_DIR=build-paho
mkdir ${PAHO_BUILD_DIR}
cd ${PAHO_BUILD_DIR}
cmake -DIMQTT_USE_PAHO:BOOL=ON -DIMQTT_BUILD_SAMPLE:BOOL=ON -DIMQTT_INSTALL:BOOL=OFF -DIMQTT_BUILD_DOC:BOOL=OFF -DBUILD_SHARED_LIBS=OFF ..
cmake --build . -j ${NUMBER_OF_PROCESSORS}
cd ..

MOSQ_BUILD_DIR=build-mosq
mkdir ${MOSQ_BUILD_DIR}
cd ${MOSQ_BUILD_DIR}
cmake -DIMQTT_USE_MOSQ:BOOL=ON -DIMQTT_BUILD_SAMPLE:BOOL=ON -DIMQTT_INSTALL:BOOL=OFF -DIMQTT_BUILD_DOC:BOOL=OFF -DBUILD_SHARED_LIBS=OFF ..
cmake --build . -j ${NUMBER_OF_PROCESSORS}
cd ..