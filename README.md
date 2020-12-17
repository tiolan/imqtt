# IMqtt
IMqtt is a (by far not fully complete) C++ Wrapper around MQTT libraries, written in C.

The idea is to provide an abstract C++ interface to make the underlaying C MQTT library exchangeable quite easy.
Target for now is to support [Paho](https://github.com/eclipse/paho.mqtt.c) and [Mosquitto](https://github.com/eclipse/mosquitto).