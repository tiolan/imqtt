/**
 * @file IMqttClientCallbacks.h
 * @author Timo Lange
 * @brief Callback definitions for an abstract MqttClient
 * @date 2020
 * @copyright    Copyright 2020 Timo Lange

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#pragma once

#include <map>
#include <string>

#include "IMqttMessage.h"

namespace i_mqtt_client {

/**
 * @brief Describes the abstract callback interface for an MqttClient to provide logs to the user. Inherit from this
 * class in order to obtain logs from the IMqtt implementation.
 */
class IMqttLogCallbacks {
private:
    static MqttLogInit_t mqttLibLogInitParams;

protected:
    IMqttLogCallbacks(void) = default;
    /**
     * @brief used internally
     *
     */
    static void LogMqttLib(LogLevelLib, std::string const&);
    /**
     * @brief Can be used in order to provide a callback function object, that is invoked to handover logs from the
     * underlying MQTT library to the user. Logs from IMqtt will not be provided through this callback.
     * This method can only be called one single time and has to be called before instantiating the first IMqtt object.
     *
     * @param initParam see MqttLogInit_t
     * @return Either what is set via argument, or in case of a second call, what has been set already (argument is
     * ignored in this case).
     */
    static MqttLogInit_t const& InitLogMqttLib(MqttLogInit_t const& initParam);

public:
    virtual ~IMqttLogCallbacks() noexcept = default;
    /**
     * @brief Override this method in order to obatain logs from the IMqtt implementation. Logs from the underlying MQTT
     * library will not be provided in this callback. If not overriden, a default empty function will be used.
     *
     * @param lvl the log level of the message
     * @param txt the message text
     */
    virtual void
    Log(LogLevel lvl, std::string const& txt) const
    {
        (void)lvl;
        (void)txt;
    };
};

/**
 * @brief Describes the abstract callback interface that is used in order to provide information about connection status
 * changes to the user. Inherit from this class in order to provide callbacks to the IMqtt implementation.
 *
 */
class IMqttConnectionCallbacks {
protected:
    IMqttConnectionCallbacks(void) = default;

public:
    virtual ~IMqttConnectionCallbacks() noexcept = default;
    /**
     * @brief Type of connection information (not the actual connection status)
     *
     */
    enum class ConnectionType {
        /**
         * @brief Callback is invoked because of information about the connection state.
         *
         */
        CONNECT,
        /**
         * @brief Callback is invoked because of information about the dis-connection state.
         *
         */
        DISCONNECT
    };

    /**
     * @brief Override this method in order to obatain information about the status of the connection or connection
     * changes. If not overriden, a default empty method will be used.
     *
     * @param type indicates whether the information are about a connect or a disconnect (see ConnectionType)
     * @param mqttRc MQTTv5 reason code provided by the MQTT library
     */
    virtual void
    OnConnectionStatusChanged(ConnectionType type, Mqtt5ReasonCode mqttRc) const
    {
        (void)type;
        (void)mqttRc;
    };
};

/**
 * @brief Describes the abstract interface that is used in order to provides MQTT messages received by the underlying
 * MQTT libary to the user. Inherit from this class in order to provide a callback method.
 *
 */
class IMqttMessageCallbacks {
protected:
    IMqttMessageCallbacks(void) = default;

public:
    virtual ~IMqttMessageCallbacks() noexcept = default;

    /**
     * @brief Has the be overriden by the user and is invoked when an MQTT message was received and is to be delivered
     * to the user. While the callback is active, the underlying MQTT library is blocked from doing any other tasks.
     * Hence, the time spent in this callback should be short. An ::IDispatchQueue can be used in order to decouple
     * callbacks and message processing. If not inherited and overriden a default callback will be invoked, that
     * generates a warning log message.
     *
     * @param mqttMessage the message, received by the underlying MQTT library
     */
    virtual void OnMqttMessage(upMqttMessage_t mqttMessage) const = 0;
};

/**
 * @brief Describes the abstract interface that is used in order to handover information about command such as Publish,
 * Subscribe and Unsubscribe to the user. Inherit from this class in order to provide a callback method.
 *
 */
class IMqttCommandCallbacks {
protected:
    IMqttCommandCallbacks(void) = default;
    using token_t               = int;

public:
    virtual ~IMqttCommandCallbacks() noexcept = default;

    /**
     * @brief Can be overriden by the user in order to obtain information about a preceeded call to
     * IMqttClient::SubscribeAsync. Currently is invoked, once the underlying MQTT library finished subscribing.
     * If not overriden, a default empty callback will be used.
     * @todo This method should provide the MQTT v5 reason codes to the user.
     *
     * @param token a value indicating which preceeding call to IMqttClient::SubscribeAsync this callback belongs to.
     * @warning For Paho AND QOS0 operations, the token is always 0 (fire and forget strategy).
     */
    virtual void
    OnSubscribe(token_t token) const
    {
        (void)token;
    }

    /**
     * @brief Can be overriden by the user in order to obtain information about a preceeded call to
     * IMqttClient::UnSubscribeAsync. Currently is invoked, once the underlying MQTT library finished unsubscribing.
     * If not overriden, a default empty callback will be used.
     * @todo This method should provide the MQTT v5 reason codes to the user.
     *
     * @param token a value indicating which preceeding call to IMqttClient::UnSubscribeAsync this callback belongs to.
     * @warning For Paho AND QOS0 operations, the token is always 0 (fire and forget strategy).
     */
    virtual void
    OnUnSubscribe(token_t token) const
    {
        (void)token;
        /*by default, do nothing*/
    }

    /**
     * @brief Can be overriden by the user in order to obtain information about a preceeded call to
     * IMqttClient::PublishAsync. Is invoked, once the underlying MQTT library finished publishing.
     * If not overriden, a default empty callback will be used.
     *
     * @param token a value indicating which preceeding call to IMqttClient::PublishAsync this callback belongs to.
     * @param mqttRc the MQTTv5 reason code, provided by the broker
     * @warning For Paho AND QOS0 operations, the token is always 0 (fire and forget strategy)
     */
    virtual void
    OnPublish(token_t token, Mqtt5ReasonCode mqttRc) const
    {
        (void)token;
        (void)mqttRc;
    }
};

/**
 * @brief In case the user wants one class to inherit from each callback class, this wrapper allows to
 * inherit all of them combined in one class. It does not provide any functionality.
 *
 */
class IMqttClientCallbacks : public IMqttLogCallbacks,
                             public IMqttMessageCallbacks,
                             public IMqttConnectionCallbacks,
                             public IMqttCommandCallbacks {
protected:
    IMqttClientCallbacks(void) = default;

public:
    virtual ~IMqttClientCallbacks() noexcept = default;
};
}  // namespace i_mqtt_client